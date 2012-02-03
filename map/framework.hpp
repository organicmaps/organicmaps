#pragma once

#include "events.hpp"
#include "drawer_yg.hpp"
#include "tile_renderer.hpp"
#include "information_display.hpp"
#include "window_handle.hpp"
#include "location_state.hpp"
#include "navigator.hpp"
#include "feature_vec_model.hpp"

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

#include "../std/vector.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/scoped_ptr.hpp"
#include "../std/target_os.hpp"

#include "../search/search_engine.hpp"


//#define DRAW_TOUCH_POINTS


class DrawerYG;
class RenderPolicy;

class Framework
{
protected:
  scoped_ptr<search::Engine> m_pSearchEngine;
  model::FeaturesFetcher m_model;
  Navigator m_navigator;

  threads::Mutex m_renderMutex;
  scoped_ptr<RenderPolicy> m_renderPolicy;
  bool m_hasPendingInvalidate, m_doForceUpdate, m_queryMaxScaleMode;

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

  void AddMap(string const & file);
  void RemoveMap(string const & datFile);
  /// Only file names
  void GetLocalMaps(vector<string> & outMaps);

public:
  Framework();
  virtual ~Framework();

  /// @name Used on iPhone for upgrade from April 1.0.1 version
  //@{
  /// @return true if client should display delete old maps dialog before using downloader
  bool NeedToDeleteOldMaps() const;
  void DeleteOldMaps();
  //@}

  void AddLocalMaps();
  void RemoveLocalMaps();

  storage::Storage & Storage() { return m_storage; }

  void OnLocationStatusChanged(location::TLocationStatus newStatus);
  void OnGpsUpdate(location::GpsInfo const & info);
  void OnCompassUpdate(location::CompassInfo const & info);

  void SetRenderPolicy(RenderPolicy * renderPolicy);
  RenderPolicy * GetRenderPolicy() const;

  InformationDisplay & GetInformationDisplay();

  void PrepareToShutdown();

  void SetupMeasurementSystem();

  RenderPolicy::TRenderFn DrawModelFn();

  void DrawModel(shared_ptr<PaintEvent> const & e,
                 ScreenBase const & screen,
                 m2::RectD const & selectRect,
                 m2::RectD const & clipRect,
                 int scaleLevel,
                 bool isTiling);

private:
  search::Engine * GetSearchEngine();
public:
  void Search(search::SearchParams const & params);
  bool GetCurrentPosition(double & lat, double & lon);

  void SetMaxWorldRect();

  void Invalidate(bool doForceUpdate = false);
  void InvalidateRect(m2::RectD const & rect, bool doForceUpdate = false);

  void SaveState();
  bool LoadState();

  /// Resize event from window.
  virtual void OnSize(int w, int h);

  bool SetUpdatesEnabled(bool doEnable);

  //double GetCurrentScale() const;
  int GetDrawScale() const;

  m2::PointD GetViewportCenter() const;
  void SetViewportCenter(m2::PointD const & pt);

  bool NeedRedraw() const;
  void SetNeedRedraw(bool flag);

  inline void XorQueryMaxScaleMode()
  {
    m_queryMaxScaleMode = !m_queryMaxScaleMode;
    Invalidate(true);
  }

  void GetFeatureTypes(m2::PointD pt, vector<string> & types) const;

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

private:
  //bool IsEmptyModel() const;
  bool IsEmptyModel(m2::PointD const & pt);
};
