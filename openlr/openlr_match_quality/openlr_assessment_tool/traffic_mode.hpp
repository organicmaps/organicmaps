#pragma once

#include "indexer/feature.hpp"

#include "openlr/decoded_path.hpp"
#include "openlr/openlr_model.hpp"

#include "base/exception.hpp"

#include "3party/pugixml/src/pugixml.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

#include <QAbstractTableModel>
#include <Qt>

class Index;
class QItemSelection;
class Selection;

DECLARE_EXCEPTION(TrafficModeError, RootException);

using FeaturePoint = std::pair<FeatureID, size_t>;

namespace impl
{
/// This class denotes a "non-deterministic" feature point.
/// I.e. it is a set of all pairs <FeatureID, point index>
/// located at a specified coordinate.
/// Only one point at a time is considered active.
class RoadPointCandidate
{
public:
  RoadPointCandidate(std::vector<FeaturePoint> const & points,
                     m2::PointD const & coord);

  void ActivateCommonPoint(RoadPointCandidate const & rpc);
  FeaturePoint const & GetPoint() const;
  m2::PointD const & GetCoordinate() const;

private:
  static size_t const kInvalidId;

  void SetActivePoint(FeatureID const & fid);

  m2::PointD m_coord = m2::PointD::Zero();
  std::vector<FeaturePoint> m_points;

  size_t m_activePointIndex = kInvalidId;
};
}  // namespace impl

class SegmentCorrespondence
{
public:
  enum class Status
  {
    Untouched,
    Assessed,
    Ignored
  };

  SegmentCorrespondence(SegmentCorrespondence const & sc)
  {
    m_partnerSegment = sc.m_partnerSegment;

    m_matchedPath = sc.m_matchedPath;
    m_fakePath = sc.m_fakePath;
    m_goldenPath = sc.m_goldenPath;

    m_partnerXMLDoc.reset(sc.m_partnerXMLDoc);
    m_partnerXMLSegment = m_partnerXMLDoc.child("reportSegments");

    m_status = sc.m_status;
  }

  SegmentCorrespondence(openlr::LinearSegment const & segment,
                        openlr::Path const & matchedPath,
                        openlr::Path const & fakePath,
                        openlr::Path const & goldenPath,
                        pugi::xml_node const & partnerSegmentXML)    : m_partnerSegment(segment)
    , m_matchedPath(matchedPath)
    , m_fakePath(fakePath)
  {
    SetGoldenPath(goldenPath);

    m_partnerXMLDoc.append_copy(partnerSegmentXML);
    m_partnerXMLSegment = m_partnerXMLDoc.child("reportSegments");
    CHECK(m_partnerXMLSegment, ("Node should contain <reportSegments> part"));
  }

  openlr::Path const & GetMatchedPath() const { return m_matchedPath; }
  openlr::Path const & GetFakePath() const { return m_fakePath; }

  openlr::Path const & GetGoldenPath() const { return m_goldenPath; }
  void SetGoldenPath(openlr::Path const & p) {
    m_goldenPath = p;
    m_status = p.empty() ? Status::Untouched : Status::Assessed;
  }

  openlr::LinearSegment const & GetPartnerSegment() const { return m_partnerSegment; }

  uint32_t GetPartnerSegmentId() const { return m_partnerSegment.m_segmentId; }

  pugi::xml_document const & GetPartnerXML() const { return m_partnerXMLDoc; }
  pugi::xml_node const & GetPartnerXMLSegment() const { return m_partnerXMLSegment; }

  Status GetStatus() const { return m_status; }

  void Ignore()
  {
    m_status = Status::Ignored;
    m_goldenPath.clear();
  }

private:
  openlr::LinearSegment m_partnerSegment;

  openlr::Path m_matchedPath;
  openlr::Path m_fakePath;
  openlr::Path m_goldenPath;

  // A dirty hack to save back SegmentCorrespondence.
  // TODO(mgsergio): Consider unifying xml serialization with one used in openlr_stat.
  pugi::xml_document m_partnerXMLDoc;
  // This is used by GetPartnerXMLSegment shortcut to return const ref. pugi::xml_node is
  // just a wrapper so returning by value won't guarantee constness.
  pugi::xml_node m_partnerXMLSegment;

  Status m_status = Status::Untouched;
};

/// This class is used to delegate segments drawing to the DrapeEngine.
class TrafficDrawerDelegateBase
{
public:
  virtual ~TrafficDrawerDelegateBase() = default;

  virtual void SetViewportCenter(m2::PointD const & center) = 0;

  virtual void DrawDecodedSegments(std::vector<m2::PointD> const & points) = 0;
  virtual void DrawEncodedSegment(std::vector<m2::PointD> const & points) = 0;
  virtual void DrawGoldenPath(std::vector<m2::PointD> const & points) = 0;

  virtual void ClearGoldenPath() = 0;
  virtual void ClearAllPaths() = 0;

  virtual void VisualizePoints(std::vector<m2::PointD> const & points) = 0;
  virtual void ClearAllVisualizedPoints() = 0;
};

class BookmarkManager;

/// This class is responsible for collecting junction points and
/// checking user's clicks.
class PointsControllerDelegateBase
{
public:
  enum class ClickType
  {
    Miss,
    Add,
    Remove
  };

  virtual std::vector<m2::PointD> GetAllJunctionPointsInViewport() const = 0;
  /// Returns all junction points at a given location in the form of feature id and
  /// point index in the feature.
  virtual std::pair<std::vector<FeaturePoint>, m2::PointD> GetCandidatePoints(
      m2::PointD const & p) const = 0;
  virtual std::vector<m2::PointD> GetReachablePoints(m2::PointD const & p) const = 0;

  virtual ClickType CheckClick(m2::PointD const & clickPoint,
                       m2::PointD const & lastClickedPoint,
                       std::vector<m2::PointD> const & reachablePoints) const = 0;
};

/// This class is used to map sample ids to real data
/// and change sample evaluations.
class TrafficMode : public QAbstractTableModel
{
  Q_OBJECT

public:
  // TODO(mgsergio): Check we are on the right mwm. I.e. right mwm version and everything.
  TrafficMode(std::string const & dataFileName,
              Index const & index,
              std::unique_ptr<TrafficDrawerDelegateBase> drawerDelegate,
              std::unique_ptr<PointsControllerDelegateBase> pointsDelegate,
              QObject * parent = Q_NULLPTR);

  bool SaveSampleAs(std::string const & fileName) const;

  int rowCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;
  int columnCount(const QModelIndex & parent = QModelIndex()) const Q_DECL_OVERRIDE;

  QVariant data(const QModelIndex & index, int role) const Q_DECL_OVERRIDE;
  // QVariant headerData(int section, Qt::Orientation orientation,
  //                     int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;

  Qt::ItemFlags flags(QModelIndex const & index) const Q_DECL_OVERRIDE;

  bool IsBuildingPath() const { return m_buildingPath; }
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

private:
  void HandlePoint(m2::PointD clickPoint, Qt::MouseButton const button);

  Index const & m_index;
  std::vector<SegmentCorrespondence> m_segments;
  // Non-owning pointer to an element of m_segments.
  SegmentCorrespondence * m_currentSegment = nullptr;

  std::unique_ptr<TrafficDrawerDelegateBase> m_drawerDelegate;
  std::unique_ptr<PointsControllerDelegateBase> m_pointsDelegate;

  bool m_buildingPath = false;
  std::vector<impl::RoadPointCandidate> m_goldenPath;
};
