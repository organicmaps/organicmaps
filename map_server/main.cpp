#include "main.hpp"
#include "render_context.hpp"

#include "../graphics/resource_manager.hpp"

#include "../indexer/mercator.hpp"

#include "../map/render_policy.hpp"
#include "../map/simple_render_policy.hpp"
#include "../map/framework.hpp"

#include "../gui/controller.hpp"

#include "../platform/platform.hpp"
#include "../platform/settings.hpp"

#include "../std/shared_ptr.hpp"

#include "qjsonrpcservice.h"

#include <QGLPixelBuffer>
#include <QtCore/QBuffer>
#include <QtGui/QApplication>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QLocalServer>
#include <QFile>
#include <QDir>

#define TEXTURE_SIZE 2560


MwmRpcService::MwmRpcService(QObject * parent) : m_pixelBuffer(new QGLPixelBuffer(TEXTURE_SIZE, TEXTURE_SIZE))
{
  LOG(LINFO, ("MwmRpcService started"));

  m_pixelBuffer->makeCurrent();
  shared_ptr<srv::RenderContext> primaryRC(new srv::RenderContext());
  graphics::ResourceManager::Params rmParams;
  rmParams.m_rtFormat = graphics::Data8Bpp;
  rmParams.m_texFormat = graphics::Data8Bpp;
  rmParams.m_texRtFormat = graphics::Data4Bpp;
  rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

  m_videoTimer = new EmptyVideoTimer();

  m_rpParams.m_videoTimer = m_videoTimer;
  m_rpParams.m_useDefaultFB = true;
  m_rpParams.m_rmParams = rmParams;
  m_rpParams.m_primaryRC = primaryRC;
  m_rpParams.m_density = graphics::EDensityXHDPI;
  m_rpParams.m_skinName = "basic.skn";
  m_rpParams.m_screenWidth = TEXTURE_SIZE;
  m_rpParams.m_screenHeight = TEXTURE_SIZE;

  try
  {
    m_framework.SetRenderPolicy(new SimpleRenderPolicy(m_rpParams));
  }
  catch (graphics::gl::platform_unsupported const & e)
  {
    LOG(LCRITICAL, ("OpenGL platform is unsupported, reason: ", e.what()));
  }

}

QString MwmRpcService::RenderBox(
    const QVariant bbox,
    int width,
    int height,
    QString const & density,
    QString const & language,
    bool maxScaleMode
    )
{
  LOG(LINFO, ("Render box started", width, height, maxScaleMode));

  // @todo: set language from parameter
  // Settings::SetCurrentLanguage(string(language.toAscii()));

  graphics::EDensity requestDensity;
  graphics::convert(density.toAscii(), requestDensity);
  if (m_framework.GetRenderPolicy()->Density() != requestDensity)
  {
    m_rpParams.m_density = requestDensity;
    m_framework.SetRenderPolicy(0);
    m_framework.SetRenderPolicy(new SimpleRenderPolicy(m_rpParams));
  };

  m_framework.OnSize(width, height);
  m_framework.SetQueryMaxScaleMode(maxScaleMode);

  QVariantList box(bbox.value<QVariantList>());
  m2::AnyRectD requestBox(m2::RectD(
                            MercatorBounds::LonToX(box[0].toDouble()),
                            MercatorBounds::LatToY(box[1].toDouble()),
                            MercatorBounds::LonToX(box[2].toDouble()),
                            MercatorBounds::LatToY(box[3].toDouble())
                         ));
  m_framework.GetNavigator().SetFromRect(requestBox);

  shared_ptr<PaintEvent> pe(new PaintEvent(m_framework.GetRenderPolicy()->GetDrawer().get()));

  m_framework.BeginPaint(pe);
  m_framework.DoPaint(pe);
  m_framework.EndPaint(pe);

  QByteArray ba;
  QBuffer b(&ba);
  b.open(QIODevice::WriteOnly);

  m_pixelBuffer->toImage().copy(0, TEXTURE_SIZE-height, width, height).save(&b, "PNG");

  LOG(LINFO, ("Render box finished"));
  return QString(ba.toBase64());
}

bool MwmRpcService::Ping()
{
  return true;
}

void MwmRpcService::Exit()
{
  qApp->exit(0);
}

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);

  string socketPath;

  if (!Settings::Get("ServerSocketPath", socketPath))
  {
    socketPath = "/tmp/mwm-render-socket";
    Settings::Set("ServerSocketPath", socketPath);
  }

  QString qSocketPath(socketPath.c_str());

  if (QFile::exists(qSocketPath))
  {
    if (!QFile::remove(qSocketPath))
    {
      qDebug() << "couldn't delete temporary service";
      return -1;
    }
  }

  MwmRpcService service;
  QJsonRpcLocalServer rpcServer;
  rpcServer.addService(&service);
  if (!rpcServer.listen(qSocketPath))
  {
    qDebug() << "could not start server: " << rpcServer.errorString();
    return -1;
  }

  return app.exec();
}
