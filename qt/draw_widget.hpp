#pragma once

#include "qt_window_handle.hpp"
#include "widgets.hpp"

#include "../map/feature_vec_model.hpp"
#include "../map/framework.hpp"
#include "../map/navigator.hpp"

#include <QtCore/QTimer>

class FileReader;
template <class> class ReaderSource;
class FileWriter;

namespace storage { class Storage; }

class QSlider;

namespace qt
{
  /// Replace this to set a draw widget kernel.
  typedef GLDrawWidget widget_type;

  class DrawWidget : public widget_type
  {
    typedef widget_type base_type;

    typedef qt::WindowHandle handle_t;
    shared_ptr<handle_t> m_handle;

    typedef model::FeaturesFetcher model_t;

    FrameWork<model_t, Navigator, handle_t> m_framework;

    bool m_isDrag;

    QTimer * m_timer;
    size_t m_redrawInterval;

    Q_OBJECT

  public Q_SLOTS:
    void MoveLeft();
    void MoveRight();
    void MoveUp();
    void MoveDown();
    void ScalePlus();
    void ScaleMinus();
    void ShowAll();
    void Repaint();
    void ScaleChanged(int action);
    void ScaleTimerElapsed();

  public:
    DrawWidget(QWidget * pParent, storage::Storage & storage);

    void SetScaleControl(QSlider * pScale);

    //model_t * GetModel() { return &(m_framework.get_model()); }

    //void ShowFeature(Feature const & p);

    void SaveState(FileWriter & writer);
    void LoadState(ReaderSource<FileReader> & reader);

  protected:
    string GetIniFile();
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
    QSlider * m_pScale;
  };
}
