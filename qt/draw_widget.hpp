#pragma once

#include "map/framework.hpp"
#include "map/navigator.hpp"
#include "map/qgl_render_context.hpp"

#include "render/window_handle.hpp"

#include "base/scheduled_task.hpp"

#include "platform/video_timer.hpp"

#include "std/unique_ptr.hpp"

#include <QtCore/QTimer>
#include <QtOpenGL/qgl.h>


namespace qt
{
  class QScaleSlider;

  class DrawWidget;

  class QtVideoTimer : public QObject, public ::VideoTimer
  {
    Q_OBJECT
  private:
    QTimer * m_timer;

  public:
    QtVideoTimer(::VideoTimer::TFrameFn frameFn);

    void resume();
    void pause();

    void start();
    void stop();

  protected:
    Q_SLOT void TimerElapsed();
  };

  class DrawWidget : public QGLWidget
  {
    bool m_isInitialized;
    bool m_isTimerStarted;

    unique_ptr<Framework> m_framework;
    unique_ptr<VideoTimer> m_videoTimer;

    bool m_isDrag;
    bool m_isRotate;

    //QTimer * m_timer;
    //QTimer * m_animTimer;
    //size_t m_redrawInterval;

    qreal m_ratio;

    inline int L2D(int px) const { return px * m_ratio; }
    inline m2::PointD GetDevicePoint(QMouseEvent * e) const;
    DragEvent GetDragEvent(QMouseEvent * e) const;
    RotateEvent GetRotateEvent(QPoint const & pt) const;

    Q_OBJECT

  public Q_SLOTS:
    void MoveLeft();
    void MoveRight();
    void MoveUp();
    void MoveDown();

    void ScalePlus();
    void ScaleMinus();
    void ScalePlusLight();
    void ScaleMinusLight();

    void ShowAll();
    void Repaint();
    void ScaleChanged(int action);
    //void ScaleTimerElapsed();

    void QueryMaxScaleMode();

  public:
    DrawWidget(QWidget * pParent);
    ~DrawWidget();

    void SetScaleControl(QScaleSlider * pScale);

    bool Search(search::SearchParams params);
    string GetDistance(search::Result const & res) const;
    void ShowSearchResult(search::Result const & res);
    void CloseSearch();

    void OnLocationUpdate(location::GpsInfo const & info);

    void SaveState();
    void LoadState();

    void UpdateNow();
    void UpdateAfterSettingsChanged();

    void PrepareShutdown();

    Framework & GetFramework() { return *m_framework.get(); }

    void SetMapStyle(MapStyle mapStyle);

  protected:
    VideoTimer * CreateVideoTimer();

  protected:
    void StartPressTask(m2::PointD const & pt, unsigned ms);
    bool KillPressTask();
    void OnPressTaskEvent(m2::PointD const & pt, unsigned ms);
    void OnActivateMark(unique_ptr<UserMarkCopy> pCopy);

  protected:
    /// @name Overriden from base_type.
    //@{
    virtual void initializeGL();
    virtual void resizeGL(int w, int h);
    virtual void paintGL();
    //@}

    void DrawFrame();

    /// @name Overriden from QWidget.
    //@{
    virtual void mousePressEvent(QMouseEvent * e);
    virtual void mouseDoubleClickEvent(QMouseEvent * e);
    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void mouseReleaseEvent(QMouseEvent * e);
    virtual void wheelEvent(QWheelEvent * e);
    virtual void keyReleaseEvent(QKeyEvent * e);
    //@}

  private:
    void UpdateScaleControl();
    void StopDragging(QMouseEvent * e);
    void StopRotating(QMouseEvent * e);
    void StopRotating(QKeyEvent * e);

    QScaleSlider * m_pScale;

    unique_ptr<ScheduledTask> m_scheduledTask;
    m2::PointD m_taskPoint;
    bool m_wasLongClick, m_isCleanSingleClick;

    bool m_emulatingLocation;

    PinClickManager & GetBalloonManager() { return m_framework->GetBalloonManager(); }

    void InitRenderPolicy();
  };
}
