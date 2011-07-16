#pragma once

#include "qt_window_handle.hpp"
#include "widgets.hpp"

#include "../map/feature_vec_model.hpp"
#include "../map/framework.hpp"
#include "../map/navigator.hpp"

#include "../std/auto_ptr.hpp"

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

  class DrawWidget : public widget_type
  {
    typedef widget_type base_type;

    shared_ptr<qt::WindowHandle> m_handle;

    typedef model::FeaturesFetcher model_t;

    auto_ptr<Framework<model_t> > m_framework;

    bool m_isDrag;

    QTimer * m_timer;
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

  public:
    DrawWidget(QWidget * pParent, storage::Storage & storage);

    void SetScaleControl(QScaleSlider * pScale);

    void OnEnableMyPosition(LocationRetrievedCallbackT observer);
    void OnDisableMyPosition();

    void Search(string const & text, SearchCallbackT callback);
    void ShowFeature(m2::RectD const & rect);

    void SaveState();
    /// @return false if can't load previously saved values
    bool LoadState();

    void UpdateNow();

    void PrepareShutdown();

  protected:
    static const uint32_t ini_file_version = 0;

  protected:
    /// @name Overriden from base_type.
    //@{
    virtual void initializeGL();
    virtual void DoDraw(shared_ptr<drawer_t> p);
    virtual void DoResize(int w, int h);
    //@}

    /// @name Overriden from QWidget.
    //@{
    virtual void mousePressEvent(QMouseEvent * e);
    virtual void mouseDoubleClickEvent(QMouseEvent * e);
    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void mouseReleaseEvent(QMouseEvent * e);
    virtual void wheelEvent(QWheelEvent * e);
    //@}

  private:
    void UpdateScaleControl();
    void StopDragging(QMouseEvent * e);

    QScaleSlider * m_pScale;
  };
}
