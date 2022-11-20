#pragma once

#include "points_controller_delegate_base.hpp"
#include "segment_correspondence.hpp"
#include "traffic_drawer_delegate_base.hpp"

#include "openlr/decoded_path.hpp"

#include "indexer/data_source.hpp"

#include "base/exception.hpp"

#include "3party/pugixml/pugixml/src/pugixml.hpp"

#include <memory>
#include <string>
#include <vector>

#include <QAbstractTableModel>


class QItemSelection;
class Selection;

DECLARE_EXCEPTION(TrafficModeError, RootException);

namespace openlr
{
namespace impl
{
/// This class denotes a "non-deterministic" feature point.
/// I.e. it is a set of all pairs <FeatureID, point index>
/// located at a specified coordinate.
/// Only one point at a time is considered active.
class RoadPointCandidate
{
public:
  RoadPointCandidate(std::vector<openlr::FeaturePoint> const & points,
                     m2::PointD const & coord);

  void ActivateCommonPoint(RoadPointCandidate const & rpc);
  openlr::FeaturePoint const & GetPoint() const;
  m2::PointD const & GetCoordinate() const;

private:
  static size_t const kInvalidId;

  void SetActivePoint(FeatureID const & fid);

  m2::PointD m_coord = m2::PointD::Zero();
  std::vector<openlr::FeaturePoint> m_points;

  size_t m_activePointIndex = kInvalidId;
};
}  // namespace impl

/// This class is used to map sample ids to real data
/// and change sample evaluations.
class TrafficMode : public QAbstractTableModel
{
  Q_OBJECT

public:
  // TODO(mgsergio): Check we are on the right mwm. I.e. right mwm version and everything.
  TrafficMode(std::string const & dataFileName, DataSource const & dataSource,
              std::unique_ptr<TrafficDrawerDelegateBase> drawerDelegate,
              std::unique_ptr<PointsControllerDelegateBase> pointsDelegate,
              QObject * parent = Q_NULLPTR);

  bool SaveSampleAs(std::string const & fileName) const;

  int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  QVariant headerData(int section, Qt::Orientation orientation,
                      int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex & index, int role) const Q_DECL_OVERRIDE;

  Qt::ItemFlags flags(QModelIndex const & index) const Q_DECL_OVERRIDE;

  bool IsBuildingPath() const { return m_buildingPath; }
  void GoldifyMatchedPath();
  void StartBuildingPath();
  void PushPoint(m2::PointD const & coord,
                 std::vector<FeaturePoint> const & points);
  void PopPoint();
  void CommitPath();
  void RollBackPath();
  void IgnorePath();

  size_t GetPointsCount() const;
  m2::PointD const & GetPoint(size_t const index) const;
  m2::PointD const & GetLastPoint() const;
  std::vector<m2::PointD> GetGoldenPathPoints() const;

public slots:
  void OnItemSelected(QItemSelection const & selected, QItemSelection const &);
  void OnClick(m2::PointD const & clickPoint, Qt::MouseButton const button)
  {
    HandlePoint(clickPoint, button);
  }

signals:
  void EditingStopped();
  void SegmentSelected(int segmentId);

private:
  void HandlePoint(m2::PointD clickPoint, Qt::MouseButton const button);
  bool StartBuildingPathChecks() const;

  DataSource const & m_dataSource;
  std::vector<SegmentCorrespondence> m_segments;
  // Non-owning pointer to an element of m_segments.
  SegmentCorrespondence * m_currentSegment = nullptr;

  std::unique_ptr<TrafficDrawerDelegateBase> m_drawerDelegate;
  std::unique_ptr<PointsControllerDelegateBase> m_pointsDelegate;

  bool m_buildingPath = false;
  std::vector<impl::RoadPointCandidate> m_goldenPath;

  // Clone this document and add things to its clone when saving sample.
  pugi::xml_document m_template;
};
}  // namespace openlr
