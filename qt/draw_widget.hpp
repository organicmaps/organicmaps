#pragma once

#include "qt/qtoglcontextfactory.hpp"

#include "map/framework.hpp"

#include "search/everywhere_search_params.hpp"

#include "drape_frontend/gui/skin.hpp"

#include "drape_frontend/drape_engine.hpp"

#include "std/condition_variable.hpp"
#include "std/mutex.hpp"
#include "std/unique_ptr.hpp"

#include <QtWidgets/QOpenGLWidget>
#include <QtWidgets/QRubberBand>

class QQuickWindow;

namespace qt
{
  class QScaleSlider;

  class DrawWidget : public QOpenGLWidget
  {
    using TBase = QOpenGLWidget;

    drape_ptr<QtOGLContextFactory> m_contextFactory;
    unique_ptr<Framework> m_framework;

    qreal m_ratio;

    Q_OBJECT

  public Q_SLOTS:
    void ScalePlus();
    void ScaleMinus();
    void ScalePlusLight();
    void ScaleMinusLight();

    void ShowAll();
    void ScaleChanged(int action);
    void SliderPressed();
    void SliderReleased();

    void ChoosePositionModeEnable();
    void ChoosePositionModeDisable();
    void OnUpdateCountryStatusByTimer();

  public:
    DrawWidget(QWidget * parent);
    ~DrawWidget();

    void SetScaleControl(QScaleSlider * pScale);

    bool Search(search::EverywhereSearchParams const & params);
    string GetDistance(search::Result const & res) const;
    void ShowSearchResult(search::Result const & res);

    void CreateFeature();

    void OnLocationUpdate(location::GpsInfo const & info);

    void UpdateAfterSettingsChanged();

    void PrepareShutdown();

    Framework & GetFramework() { return *m_framework.get(); }

    void SetMapStyle(MapStyle mapStyle);

    void SetRouter(routing::RouterType routerType);
    Q_SIGNAL void BeforeEngineCreation();

    void CreateEngine();

    using TCurrentCountryChanged = function<void(storage::TCountryId const &, string const &,
                                                 storage::Status, uint64_t, uint8_t)>;
    void SetCurrentCountryChangedListener(TCurrentCountryChanged const & listener);

    void DownloadCountry(storage::TCountryId const & countryId);
    void RetryToDownloadCountry(storage::TCountryId const & countryId);

    void SetSelectionMode(bool mode);

  protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    /// @name Overriden from QOpenGLWindow.
    //@{
    void mousePressEvent(QMouseEvent * e) override;
    void mouseDoubleClickEvent(QMouseEvent * e) override;
    void mouseMoveEvent(QMouseEvent * e) override;
    void mouseReleaseEvent(QMouseEvent * e) override;
    void wheelEvent(QWheelEvent * e) override;
    void keyPressEvent(QKeyEvent * e) override;
    void keyReleaseEvent(QKeyEvent * e) override;
    //@}

  private:
    void SubmitFakeLocationPoint(m2::PointD const & pt);
    void SubmitRoutingPoint(m2::PointD const & pt);
    void ShowInfoPopup(QMouseEvent * e, m2::PointD const & pt);
    void ShowPlacePage(place_page::Info const & info);

    void OnViewportChanged(ScreenBase const & screen);
    void UpdateScaleControl();
    df::Touch GetTouch(QMouseEvent * e);
    df::Touch GetSymmetrical(const df::Touch & touch);
    df::TouchEvent GetTouchEvent(QMouseEvent * e, df::TouchEvent::ETouchType type);

    inline int L2D(int px) const { return px * m_ratio; }
    m2::PointD GetDevicePoint(QMouseEvent * e) const;

    void UpdateCountryStatus(storage::TCountryId const & countryId);

    QScaleSlider * m_pScale;
    QRubberBand * m_rubberBand;
    QPoint m_rubberBandOrigin;
    bool m_enableScaleUpdate;

    bool m_emulatingLocation;

    void InitRenderPolicy();

    unique_ptr<gui::Skin> m_skin;

    TCurrentCountryChanged m_currentCountryChanged;
    storage::TCountryId m_countryId;

    bool m_selectionMode = false;
  };
}
