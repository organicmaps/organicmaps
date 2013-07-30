#include "render_context.hpp"

#include "../graphics/resource_manager.hpp"

#include "../map/render_policy.hpp"
#include "../map/simple_render_policy.hpp"
#include "../map/framework.hpp"

#include "../gui/controller.hpp"

#include "../platform/platform.hpp"


#include "../std/shared_ptr.hpp"

#include <qjsonrpcservice.h>

#include <QGLPixelBuffer>
#include <QtCore/QBuffer>
#include <QtGui/QApplication>
#include <QDesktopServices>
#include <QLocalServer>
#include <QFile>
#include <QDir>

namespace
{
  void empty()
  {}

  class EmptyVideoTimer : public VideoTimer
  {
  public:
    EmptyVideoTimer()
      : VideoTimer(bind(&empty))
    {}

    ~EmptyVideoTimer()
    {
      stop();
    }

    void start()
    {
      if (m_state == EStopped)
        m_state = ERunning;
    }

    void resume()
    {
      if (m_state == EPaused)
      {
        m_state = EStopped;
        start();
      }
    }

    void pause()
    {
      stop();
      m_state = EPaused;
    }

    void stop()
    {
      if (m_state == ERunning)
        m_state = EStopped;
    }

    void perform()
    {}
  };
}

class MwmRpcService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "MapsWithMe")
private:
    Framework m_framework;
    QGLPixelBuffer * m_pixelBuffer;
    RenderPolicy::Params m_rpParams;
    VideoTimer * m_videoTimer;
public:
    MwmRpcService(QObject * parent = 0);

public Q_SLOTS:
    QString RenderBox(
        QVariant const & bbox,
        int width,
        int height,
        QString const & density,
        QString const & language,
        bool maxScaleMode
        );
    bool Ping();
    void Exit();
};

