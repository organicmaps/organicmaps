#pragma once

#include "events.hpp"
#include "drawer_yg.hpp"
#include "tile_renderer.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"

#include "../defines.hpp"

#include "../storage/storage.hpp"

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

class DrawerYG;
class RenderPolicy;

template <class TModel>
class Framework
{
protected:
  typedef TModel model_t;

  typedef Framework<model_t> this_type;

  scoped_ptr<search::Engine> m_pSearchEngine;
  model_t m_model;
  Navigator m_navigator;

  scoped_ptr<RenderPolicy> m_renderPolicy;
  bool m_hasPendingInvalidate;

  InformationDisplay m_informationDisplay;

  double const m_metresMinWidth;
  double const m_metresMaxWidth;
  int const m_minRulerWidth;

  int m_width;
  int m_height;

  enum TGpsCenteringMode
  {
    EDoNothing,
    ECenterAndScale,
    ECenterOnly
  };

  TGpsCenteringMode m_centeringMode;
  location::State m_locationState;

  mutable threads::Mutex m_modelSyn;

  storage::Storage m_storage;

  my::Timer m_timer;

  /// Stores lowest loaded map version
  /// Holds -1 if no maps were added
  /// @see feature::DataHeader::Version
  int m_lowestMapVersion;
//  typedef typename TModel::ReaderT ReaderT;

  void AddMap(string const & file);
  void RemoveMap(string const & datFile);
  /// Only file names
  void GetLocalMaps(vector<string> & outMaps);

public:

  Framework();
  virtual ~Framework();

  int GetLowestLoadedMapVersion() const { return m_lowestMapVersion; }

  storage::Storage & Storage() { return m_storage; }

  void OnLocationStatusChanged(location::TLocationStatus newStatus);
  void OnGpsUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);

  void SetRenderPolicy(RenderPolicy * renderPolicy);
  RenderPolicy * GetRenderPolicy() const;

  model_t & get_model();

  bool IsEmptyModel();

  // Cleanup.
  void Clean();

  void PrepareToShutdown();

  void SetupMeasurementSystem();

  RenderPolicy::TRenderFn DrawModelFn();

  void DrawModel(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 m2::RectD const & clipRect,
                 int scaleLevel);

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

