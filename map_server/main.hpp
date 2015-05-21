#include "map_server/render_context.hpp"

#include "map/framework.hpp"

#include "render/render_policy.hpp"

#include "platform/video_timer.hpp"

#include <qjsonrpcservice.h>
#include <QGLPixelBuffer>

class MwmRpcService : public QJsonRpcService
{
    Q_OBJECT
    Q_CLASSINFO("serviceName", "MapsWithMe")
private:
    Framework m_framework;
    QGLPixelBuffer * m_pixelBuffer;
    RenderPolicy::Params m_rpParams;
    EmptyVideoTimer m_videoTimer;
public:
    MwmRpcService(QObject * parent = 0);
    ~MwmRpcService();

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

