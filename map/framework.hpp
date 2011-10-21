#pragma once

#include "events.hpp"
#include "drawer_yg.hpp"
#include "tile_renderer.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"

#include "../defines.hpp"

#include "../indexer/mercator.hpp"
#include "../indexer/data_header.hpp"
#include "../indexer/scales.hpp"

#include "../platform/platform.hpp"
#include "../platform/location.hpp"

#include "../yg/defines.hpp"
#include "../yg/screen.hpp"
#include "../yg/color.hpp"
#include "../yg/render_state.hpp"
#include "../yg/skin.hpp"
#include "../yg/resource_manager.hpp"
#include "../yg/info_layer.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../geometry/rect2d.hpp"
#include "../geometry/screenbase.hpp"

#include "../base/logging.hpp"
#include "../base/mutex.hpp"
#include "../base/timer.hpp"

#include "../std/bind.hpp"
#include "../std/function.hpp"
#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/target_os.hpp"

#include "../search/search_engine.hpp"

//#define DRAW_TOUCH_POINTS

namespace search { class Engine; }

struct BenchmarkRectProvider;

namespace search { class Result; }
typedef function<void (search::Result const &)> SearchCallbackT;

typedef function<void (location::TLocationStatus)> LocationRetrievedCallbackT;

class DrawerYG;
class RenderPolicy;

struct PathAppender
{
  string const & m_path;
  PathAppender(string const & path) : m_path(path) {}
  void operator()(string & elem)
  {
    elem.insert(elem.begin(), m_path.begin(), m_path.end());
  }
};

class ReadersAdder
{
protected:

  typedef vector<string> maps_list_t;

private:
  Platform & m_pl;
  maps_list_t & m_lst;

public:
  ReadersAdder(Platform & pl, maps_list_t & lst) : m_pl(pl), m_lst(lst) {}

  void operator() (string const & name)
  {
    m_lst.push_back(name);
  }
};

template
<
  class TModel
>
class Framework
{
protected:
  typedef TModel model_t;

  typedef Framework<model_t> this_type;

  scoped_ptr<search::Engine> m_pSearchEngine;
  model_t m_model;
  Navigator m_navigator;

  shared_ptr<WindowHandle> m_windowHandle;
  shared_ptr<RenderPolicy> m_renderPolicy;

  InformationDisplay m_informationDisplay;

  double const m_metresMinWidth;
  double const m_metresMaxWidth;
  int const m_minRulerWidth;

  enum TGpsCenteringMode
  {
    EDoNothing,
    ECenterAndScale,
    ECenterOnly
  };

  TGpsCenteringMode m_centeringMode;
  LocationRetrievedCallbackT m_locationObserver;
  location::State m_locationState;

  mutable threads::Mutex m_modelSyn;

//  int m_tileSize;

  my::Timer m_timer;

  typedef typename TModel::ReaderT ReaderT;

  void AddMap(string const & file);
  void RemoveMap(string const & datFile);

public:
  void OnGpsUpdate(location::GpsInfo const & info);

  void OnCompassUpdate(location::CompassInfo const & info);

  Framework(shared_ptr<WindowHandle> windowHandle,
            size_t bottomShift);

  virtual ~Framework();

  void SetRenderPolicy(shared_ptr<RenderPolicy> const & rp);

  void InitializeGL(shared_ptr<yg::gl::RenderContext> const & primaryContext,
                    shared_ptr<yg::ResourceManager> const & resourceManager);

  model_t & get_model();

  typedef vector<string> maps_list_t;
  virtual void EnumLocalMaps(maps_list_t & filesList);

  /// Initialization.
  template <class TStorage>
  void InitStorage(TStorage & storage)
  {
    m_model.InitClassificator();

    typename TStorage::TEnumMapsFunction enumMapsFn;

    enumMapsFn = bind(&Framework::EnumLocalMaps, this, _1);

    LOG(LINFO, ("Initializing storage"));
    // initializes model with locally downloaded maps
    storage.Init(bind(&Framework::AddMap, this, _1),
                 bind(&Framework::RemoveMap, this, _1),
                 bind(&Framework::InvalidateRect, this, _1),
                 enumMapsFn);
    LOG(LINFO, ("Storage initialized"));
  }

  void StartLocationService(LocationRetrievedCallbackT observer);
  void StopLocationService();

  bool IsEmptyModel();

  // Cleanup.
  void Clean();

  void PrepareToShutdown();

public:

  void SetupMeasurementSystem();

  void DrawModel(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 m2::RectD const & clipRect,
                 int scaleLevel,
                 bool isTiling);

  void Search(string const & text, SearchCallbackT callback);
  search::Engine * GetSearchEngine();

  void SetMaxWorldRect();

  void Invalidate();
  void InvalidateRect(m2::RectD const & rect);

  void SaveState();
  bool LoadState();

  /// Resize event from window.
  virtual void OnSize(int w, int h);

  bool SetUpdatesEnabled(bool doEnable);

  /// respond to device orientation changes
  void SetOrientation(EOrientation orientation);

  double GetCurrentScale() const;

  m2::PointD GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);

  bool NeedRedraw() const;
  void SetNeedRedraw(bool flag);

  virtual void BeginPaint(shared_ptr<PaintEvent> const & e);
  /// Function for calling from platform dependent-paint function.
  virtual void DoPaint(shared_ptr<PaintEvent> const & e);

  virtual void EndPaint(shared_ptr<PaintEvent> const & e);

  void ShowRect(m2::RectD rect);

  void MemoryWarning();

  void EnterBackground();

  void EnterForeground();

  /// @TODO refactor to accept point and min visible length
  void CenterAndScaleViewport();

  /// Show all model by it's world rect.
  void ShowAll();

  /// @name Drag implementation.
  //@{
  void StartDrag(DragEvent const & e);
  void DoDrag(DragEvent const & e);
  void StopDrag(DragEvent const & e);
  void Move(double azDir, double factor);
  //@}

  /// @name Rotation implementation
  //@{
  void StartRotate(RotateEvent const & e);
  void DoRotate(RotateEvent const & e);
  void StopRotate(RotateEvent const & e);
  //@}

  /// @name Scaling.
  //@{
  void ScaleToPoint(ScaleToPointEvent const & e);
  void ScaleDefault(bool enlarge);
  void Scale(double scale);
  void StartScale(ScaleEvent const & e);
  void DoScale(ScaleEvent const & e);
  void StopScale(ScaleEvent const & e);
  //@}
};

