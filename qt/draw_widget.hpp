#pragma once

#include "widgets.hpp"

#include "../map/window_handle.hpp"
#include "../map/feature_vec_model.hpp"
#include "../map/framework.hpp"
#include "../map/navigator.hpp"

#include "../platform/video_timer.hpp"

#include "../std/scoped_ptr.hpp"

#include <QtCore/QTimer>

//class FileReader;
//template <class> class ReaderSource;
//class FileWriter;

namespace storage { class Storage; }

namespace qt
{
  class QScaleSlider;

  /// Replace this to set a draw widget kernel.
  typedef GLDrawWidget widget_type;

  class DrawWidget;

  class QtVideoTimer : public ::VideoTimer
  {
  private:

    QTimer * m_timer;
    DrawWidget * m_widget;

  public:

    QtVideoTimer(DrawWidget * w, ::VideoTimer::TFrameFn frameFn);

    void resume();
    void pause();

    void start();
    void stop();
  };

  class DrawWidget : public widget_type
  {
    typedef widget_type base_type;

    shared_ptr<WindowHandle> m_handle;

    shared_ptr<drawer_t> m_drawer;
    bool m_isInitialized;
    bool m_isTimerStarted;

    typedef model::FeaturesFetcher model_t;

    scoped_ptr<Framework<model_t> > m_framework;

    shared_ptr<VideoTimer> m_videoTimer;

    bool m_isDrag;
    bool m_isRotate;

    QTimer * m_timer;
    QTimer * m_animTimer;
    size_t m_redrawInterval;

    Q_OBJECT

  signals:
    void ViewportChanged();

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
    void ScaleTimerElapsed();
    void AnimTimerElapsed();

  public:
    DrawWidget(QWidget * pParent, storage::Storage & storage);

    void SetScaleControl(QScaleSlider * pScale);

    void Search(string const & text, SearchCallbackT callback);
    void ShowFeature(m2::RectD const & rect);

    void SaveState();
    /// @return false if can't load previously saved values
    bool LoadState();

    void UpdateNow();
    void UpdateAfterSettingsChanged();

    void PrepareShutdown();

    Framework<model_t> & GetFramework() { return *m_framework.get(); }

  protected:
    static const uint32_t ini_file_version = 0;

  protected:
    /// @name Overriden from base_type.
    //@{
    virtual void initializeGL();
    virtual void DoDraw(shared_ptr<screen_t> p);
    virtual void DoResize(int w, int h);
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
  };
}
