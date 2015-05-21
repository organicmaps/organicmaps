#include "map_server/main.hpp"
#include "map_server/render_context.hpp"

#include "graphics/resource_manager.hpp"

#include "indexer/mercator.hpp"

#include "map/framework.hpp"

#include "render/render_policy.hpp"
#include "render/simple_render_policy.hpp"

#include "gui/controller.hpp"

#include "platform/platform.hpp"

#include "std/shared_ptr.hpp"

#include <qjsonrpcservice.h>

#include "3party/gflags/src/gflags/gflags.h"

#include <QtOpenGL/QGLPixelBuffer>
#include <QtCore/QBuffer>
#include <QtCore/QFile>

#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
  #include <QtGui/QApplication>
#else
  #include <QtWidgets/QApplication>
#endif


DEFINE_uint64(texture_size, 2560, "Texture size");
DEFINE_string(listen, "/tmp/mwm-render-socket",
                 "Path to socket to be listened");

MwmRpcService::MwmRpcService(QObject * parent) : m_pixelBuffer(new QGLPixelBuffer(FLAGS_texture_size, FLAGS_texture_size))
{
  LOG(LINFO, ("MwmRpcService started"));

  m_pixelBuffer->makeCurrent();
#ifndef USE_DRAPE
  shared_ptr<srv::RenderContext> primaryRC(new srv::RenderContext());
  graphics::ResourceManager::Params rmParams;
  rmParams.m_texFormat = graphics::Data8Bpp;
  rmParams.m_texRtFormat = graphics::Data4Bpp;
  rmParams.m_videoMemoryLimit = GetPlatform().VideoMemoryLimit();

  m_rpParams.m_videoTimer = &m_videoTimer;
  m_rpParams.m_useDefaultFB = true;
  m_rpParams.m_rmParams = rmParams;
  m_rpParams.m_primaryRC = primaryRC;
  m_rpParams.m_density = graphics::EDensityXHDPI;
  m_rpParams.m_skinName = "basic.skn";
  m_rpParams.m_screenWidth = FLAGS_texture_size;
  m_rpParams.m_screenHeight = FLAGS_texture_size;

  try
  {
    m_framework.SetRenderPolicy(new SimpleRenderPolicy(m_rpParams));
  }
  catch (graphics::gl::platform_unsupported const & e)
  {
    LOG(LCRITICAL, ("OpenGL platform is unsupported, reason: ", e.what()));
  }
#endif // USE_DRAPE
}

MwmRpcService::~MwmRpcService()
{
  m_framework.PrepareToShutdown();
}

QString MwmRpcService::RenderBox(
    QVariant const & bbox,
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
#ifndef USE_DRAPE
  graphics::EDensity requestDensity;
  graphics::convert(density.toUtf8(), requestDensity);
  if (m_framework.GetRenderPolicy()->Density() != requestDensity)
  {
    m_rpParams.m_density = requestDensity;
    m_framework.SetRenderPolicy(0);
    m_framework.SetRenderPolicy(new SimpleRenderPolicy(m_rpParams));
  };

  m_framework.OnSize(width, height);
  m_framework.SetQueryMaxScaleMode(maxScaleMode);

  QVariantList box(bbox.value<QVariantList>());
  m2::AnyRectD requestBox(m2::RectD(MercatorBounds::FromLatLon(box[1].toDouble(), box[0].toDouble()),
                                    MercatorBounds::FromLatLon(box[3].toDouble(), box[2].toDouble())));
  m_framework.GetNavigator().SetFromRect(requestBox);

  shared_ptr<PaintEvent> pe(new PaintEvent(m_framework.GetRenderPolicy()->GetDrawer().get()));

  m_framework.BeginPaint(pe);
  m_framework.DoPaint(pe);
  m_framework.EndPaint(pe);

  QByteArray ba;
  QBuffer b(&ba);
  b.open(QIODevice::WriteOnly);

  m_pixelBuffer->toImage().copy(0, FLAGS_texture_size-height, width, height).save(&b, "PNG");

  LOG(LINFO, ("Render box finished"));
  return QString(ba.toBase64());
#else
  return QString();
#endif // USE_DRAPE
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
  google::SetUsageMessage("Usage: MapsWithMe-server [-listen SOCKET]");
  google::ParseCommandLineFlags(&argc, &argv, true);

  QApplication app(argc, argv);

  QString socketPath(FLAGS_listen.c_str());

  if (QFile::exists(socketPath))
  {
    if (!QFile::remove(socketPath))
    {
      qDebug() << "couldn't delete temporary service";
      return -1;
    }
  }

  MwmRpcService service;
  QJsonRpcLocalServer rpcServer;
  rpcServer.addService(&service);
  if (!rpcServer.listen(socketPath))
  {
    qDebug() << "could not start server: " << rpcServer.errorString();
    return -1;
  }

  return app.exec();
}
