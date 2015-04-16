#pragma once

#include "qt/qtoglcontextfactory.hpp"

#include "map/framework.hpp"
#include "map/navigator.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "base/deferred_task.hpp"

#include "std/unique_ptr.hpp"

#include <QtGui/QWindow>


namespace qt
{
  class QScaleSlider;

  class DrawWidget : public QWindow
  {
    typedef QWindow TBase;

    drape_ptr<dp::OGLContextFactory> m_contextFactory;
    unique_ptr<Framework> m_framework;

    bool m_isDrag;
    bool m_isRotate;

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
    void ScaleChanged(int action);

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

    void UpdateAfterSettingsChanged();

    void PrepareShutdown();

    Framework & GetFramework() { return *m_framework.get(); }

    void SetMapStyle(MapStyle mapStyle);

    void SetRouter(routing::RouterType routerType);

  protected:
    void StartPressTask(m2::PointD const & pt, unsigned ms);
    void KillPressTask();
    void OnPressTaskEvent(m2::PointD const & pt, unsigned ms);
    void OnActivateMark(unique_ptr<UserMarkCopy> pCopy);

    void CreateEngine();

  protected:
    /// @name Overriden from QWidget.
    //@{
    virtual void exposeEvent(QExposeEvent * e);
    virtual void mousePressEvent(QMouseEvent * e);
    virtual void mouseDoubleClickEvent(QMouseEvent * e);
    virtual void mouseMoveEvent(QMouseEvent * e);
    virtual void mouseReleaseEvent(QMouseEvent * e);
    virtual void wheelEvent(QWheelEvent * e);
    virtual void keyReleaseEvent(QKeyEvent * e);
    //@}

    Q_SLOT void sizeChanged(int);

  private:
    void UpdateScaleControl();
    void StopDragging(QMouseEvent * e);
    void StopRotating(QMouseEvent * e);
    void StopRotating(QKeyEvent * e);

    QScaleSlider * m_pScale;

    unique_ptr<DeferredTask> m_deferredTask;
    m2::PointD m_taskPoint;
    bool m_wasLongClick, m_isCleanSingleClick;

    bool m_emulatingLocation;

    PinClickManager & GetBalloonManager() { return m_framework->GetBalloonManager(); }

    void InitRenderPolicy();
  };
}
