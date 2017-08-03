#include "openlr/openlr_match_quality/openlr_assessment_tool/traffic_mode.hpp"

#include "openlr/openlr_model_xml.hpp"

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "3party/pugixml/src/utils.hpp"

#include <QItemSelection>
#include <QMessageBox>

#include <tuple>

namespace
{
std::vector<m2::PointD> GetPoints(openlr::LinearSegment const & segment)
{
  std::vector<m2::PointD> result;
  for (auto const & lrp : segment.m_locationReference.m_points)
    result.push_back(MercatorBounds::FromLatLon(lrp.m_latLon));
  return result;
}

// TODO(mgsergio): Optimize this.
std::vector<m2::PointD> GetReachablePoints(m2::PointD const & srcPoint,
                                           std::vector<m2::PointD> const path,
                                           PointsControllerDelegateBase const & pointsDelegate,
                                           size_t const lookBackIndex)
{
  auto reachablePoints = pointsDelegate.GetReachablePoints(srcPoint);
  if (lookBackIndex < path.size())
  {
    auto const & toBeRemoved = path[path.size() - lookBackIndex - 1];
    reachablePoints.erase(
        remove_if(begin(reachablePoints), end(reachablePoints),
                  [&toBeRemoved](m2::PointD const & p) {
                    return p.EqualDxDy(toBeRemoved, 1e-6);
                  }),
        end(reachablePoints));
  }
  return reachablePoints;
}
}

namespace impl
{
// static
size_t const RoadPointCandidate::kInvalidId = std::numeric_limits<size_t>::max();;

/// This class denotes a "non-deterministic" feature point.
/// I.e. it is a set of all pairs <FeatureID, point index>
/// located at a specified coordinate.
/// Only one point at a time is considered active.
RoadPointCandidate::RoadPointCandidate(std::vector<FeaturePoint> const & points,
                                       m2::PointD const & coord)
  : m_coord(coord)
  , m_candidates(points)
{
  LOG(LDEBUG, ("Candidate points:", points));
}

void RoadPointCandidate::ActivatePoint(RoadPointCandidate const & rpc)
{
  for (auto const & fp1 : m_candidates)
  {
    for (auto const & fp2 : rpc.m_candidates)
    {
      if (fp1.first == fp2.first)
      {
        SetActivePoint(fp1.first);
        return;
      }
    }
  }
  CHECK(false, ("One mutual featrue id should exist."));
}

FeaturePoint const & RoadPointCandidate::GetPoint() const
{
  CHECK_NOT_EQUAL(m_activePointIndex, kInvalidId, ("No point is active."));
  return m_candidates[m_activePointIndex];
}

m2::PointD const & RoadPointCandidate::GetCoordinate() const
{
  return m_coord;
}

void RoadPointCandidate::SetActivePoint(FeatureID const & fid)
{
  for (size_t i = 0; i < m_candidates.size(); ++i)
  {
    if (m_candidates[i].first == fid)
    {
      m_activePointIndex = i;
      return;
    }
  }
  CHECK(false, ("One point should match."));
}
}  // namespace impl

// TrafficMode -------------------------------------------------------------------------------------
TrafficMode::TrafficMode(std::string const & dataFileName,
                         Index const & index,
                         std::unique_ptr<TrafficDrawerDelegateBase> drawerDelegate,
                         std::unique_ptr<PointsControllerDelegateBase> pointsDelegate,
                         QObject * parent)
  : QAbstractTableModel(parent)
  , m_index(index)
  , m_drawerDelegate(move(drawerDelegate))
  , m_pointsDelegate(move(pointsDelegate))
{
  // TODO(mgsergio): Collect stat how many segments of each kind were parsed.
  pugi::xml_document doc;
  if (!doc.load_file(dataFileName.data()))
    MYTHROW(TrafficModeError, ("Can't load file:", strerror(errno)));

  // Select all Segment elements that are direct children of the context node.
  auto const segments = doc.select_nodes("./Segment");

  try
  {
    for (auto const xpathNode : segments)
    {
      m_segments.emplace_back();

      auto const xmlSegment = xpathNode.node();
      auto & segment = m_segments.back();

      {
        // TODO(mgsergio): Unify error handling interface of openlr_xml_mode and decoded_path parsers.
        auto const partnerSegment = xmlSegment.child("reportSegments");
        if (!openlr::SegmentFromXML(partnerSegment, segment.GetPartnerSegment()))
          MYTHROW(TrafficModeError, ("An error occured while parsing: can't parse segment"));

        segment.SetPartnerXML(partnerSegment);
      }

      if (auto const route = xmlSegment.child("Route"))
      {
        auto & path = segment.GetMatchedPath();
        path.emplace();
        openlr::PathFromXML(route, m_index, *path);
      }
      if (auto const route = xmlSegment.child("FakeRoute"))
      {
        auto & path = segment.GetFakePath();
        path.emplace();
        openlr::PathFromXML(route, m_index, *path);
      }
      if (auto const route = xmlSegment.child("GoldenRoute"))
      {
        auto & path = segment.GetGoldenPath();
        path.emplace();
        openlr::PathFromXML(route, m_index, *path);
      }
    }
  }
  catch (openlr::DecodedPathLoadError const & e)
  {
    MYTHROW(TrafficModeError, ("An exception occured while parsing", dataFileName, e.Msg()));
  }

  // TODO(mgsergio): LOG(LINFO, (xxx, "segments are loaded"));
}

// TODO(mgsergio): Check if a path was commited, or commit it.
bool TrafficMode::SaveSampleAs(std::string const & fileName) const
{
  CHECK(!fileName.empty(), ("Can't save to an empty file."));

  pugi::xml_document result;

  for (auto const & sc : m_segments)
  {
    auto segment = result.append_child("Segment");
    segment.append_copy(sc.GetPartnerXMLSegment());

    if (auto const & path = sc.GetMatchedPath())
    {
      auto node = segment.append_child("Route");
      openlr::PathToXML(*path, node);
    }
    if (auto const & path = sc.GetFakePath())
    {
      auto node = segment.append_child("FakeRoute");
      openlr::PathToXML(*path, node);
    }
    if (auto const & path = sc.GetGoldenPath())
    {
      auto node = segment.append_child("GoldenRoute");
      openlr::PathToXML(*path, node);
    }
  }

  result.save_file(fileName.data(), "  " /* indent */);
  return true;
}

int TrafficMode::rowCount(const QModelIndex & parent) const
{
  return static_cast<int>(m_segments.size());
}

int TrafficMode::columnCount(const QModelIndex & parent) const { return 1; }

QVariant TrafficMode::data(const QModelIndex & index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.row() >= rowCount())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

  if (index.column() == 0)
    return m_segments[index.row()].GetPartnerSegmentId();

  return QVariant();
}

void TrafficMode::OnItemSelected(QItemSelection const & selected, QItemSelection const &)
{
  CHECK(!selected.empty(), ("The selection should not be empty. RTFM for qt5."));
  CHECK(!m_segments.empty(), ("No segments are loaded, can't select."));

  auto const row = selected.front().top();

  // TODO(mgsergio): Use algo for center calculation.
  // Now viewport is set to the first point of the first segment.
  CHECK_LESS(row, m_segments.size(), ());
  m_currentSegment = &m_segments[row];
  auto const partnerSegmentId = m_currentSegment->GetPartnerSegmentId();

  // TODO(mgsergio): Maybe we shold show empty paths.
  CHECK(m_currentSegment->GetMatchedPath(), ("Empty mwm segments for partner id", partnerSegmentId));

  auto const & path = *m_currentSegment->GetMatchedPath();
  auto const & firstEdge = path.front();

  m_drawerDelegate->ClearAllPaths();
  m_drawerDelegate->SetViewportCenter(GetStart(firstEdge));
  m_drawerDelegate->DrawEncodedSegment(GetPoints(m_currentSegment->GetPartnerSegment()));
  m_drawerDelegate->DrawDecodedSegments(GetPoints(path));
  if (auto const & path = m_currentSegment->GetGoldenPath())
    m_drawerDelegate->DrawGoldenPath(GetPoints(*path));
}

Qt::ItemFlags TrafficMode::flags(QModelIndex const & index) const
{
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractItemModel::flags(index);
}

void TrafficMode::StartBuildingPath()
{
  CHECK(m_currentSegment, ("A segment should be selected before path building is started."));

  if (m_buildingPath)
    MYTHROW(TrafficModeError, ("Path building already in progress."));

  if (m_currentSegment->GetGoldenPath())
  {
    auto const btn = QMessageBox::question(
        nullptr,
        "Override warning",
        "The selected segment already have a golden path. Do you want to override?");
    if (btn == QMessageBox::No)
      return;
  }

  m_currentSegment->GetGoldenPath() = boost::none;

  m_buildingPath = true;
  m_drawerDelegate->ClearGoldenPath();
  m_drawerDelegate->VisualizePoints(
      m_pointsDelegate->GetAllJunctionPointsInViewPort());
}

void TrafficMode::PushPoint(m2::PointD const & coord, std::vector<FeaturePoint> const & points)
{
  impl::RoadPointCandidate point(points, coord);
  if (!m_goldenPath.empty())
    m_goldenPath.back().ActivatePoint(point);
  m_goldenPath.push_back(point);
}

void TrafficMode::PopPoint()
{
  CHECK(!m_goldenPath.empty(), ("Attempt to pop point from an empty path."));
  m_goldenPath.pop_back();
}

void TrafficMode::CommitPath()
{
  // TODO(mgsergio): Make this situation impossible. Make the first segment selected by default
  // for example.
  CHECK(m_currentSegment, ("No segments selected"));

  if (!m_buildingPath)
    MYTHROW(TrafficModeError, ("Path building is not started"));

  m_buildingPath = false;
  m_drawerDelegate->ClearAllVisualizedPoints();

  if (m_goldenPath.empty())
  {
    LOG(LDEBUG, ("Golden path is empty :'("));
    emit EditingStopped();
    return;
  }

  CHECK_NOT_EQUAL(m_goldenPath.size(), 1, ("Path cannot consist of only one point"));

  // Activate last point. Since no more points will be availabe we link it to the same
  // feature as the previous one was linked to.
  m_goldenPath.back().ActivatePoint(m_goldenPath[GetPointsCount() - 2]);

  openlr::Path path;
  for (size_t i = 1; i < GetPointsCount(); ++i)
  {
    FeatureID fid, prevFid;
    size_t segId, prevSegId;

    auto const prevPoint = m_goldenPath[i - 1];
    auto point = m_goldenPath[i];

    // The start and the end of the edge should lie on the same feature.
    point.ActivatePoint(prevPoint);

    std::tie(prevFid, prevSegId) = prevPoint.GetPoint();
    std::tie(fid, segId) = point.GetPoint();

    path.emplace_back(
        fid,
        prevSegId < segId /* forward */,
        prevSegId,
        routing::Junction(prevPoint.GetCoordinate(), 0 /* altitude */),
        routing::Junction(point.GetCoordinate(), 0 /* altitude */)
    );
  }

  m_currentSegment->GetGoldenPath() = path;
  emit EditingStopped();
}

void TrafficMode::RollBackPath()
{
  CHECK(m_currentSegment, ("No segments selected"));

  // TODO(mgsergio): CHECK ?
  if (!m_buildingPath)
    MYTHROW(TrafficModeError, ("No path building is in progress."));

  m_buildingPath = false;

  // TODO(mgsergio): Add a method for common visual manipulations.
  m_drawerDelegate->ClearAllVisualizedPoints();
  m_drawerDelegate->ClearGoldenPath();
  if (auto const & path = m_currentSegment->GetGoldenPath())
    m_drawerDelegate->DrawGoldenPath(GetPoints(*path));

  emit EditingStopped();
}

size_t TrafficMode::GetPointsCount() const
{
  return m_goldenPath.size();
}

m2::PointD const & TrafficMode::GetPoint(size_t const index) const
{
  return m_goldenPath[index].GetCoordinate();
}

m2::PointD const & TrafficMode::GetLastPoint() const
{
  CHECK(!m_goldenPath.empty(), ("Attempt to get point from an empty path."));
  return m_goldenPath.back().GetCoordinate();
}

std::vector<m2::PointD> TrafficMode::GetCoordinates() const
{
  std::vector<m2::PointD> coordinates;
  for (auto const & roadPoint : m_goldenPath)
    coordinates.push_back(roadPoint.GetCoordinate());
  return coordinates;
}

// TODO(mgsergio): Draw the first point when the path size is 1.
void TrafficMode::HandlePoint(m2::PointD clickPoint, Qt::MouseButton const button)
{
  if (!m_buildingPath)
    return;

  auto const currentPathLength = GetPointsCount();
  auto const lastClickedPoint = currentPathLength != 0
      ? GetLastPoint()
      : m2::PointD::Zero();

  // Get candidates and also fix clickPoint to make it more accurate and sutable for
  // GetOutgoingEdges.
  auto const & candidatePoints = m_pointsDelegate->GetFeaturesPointsByPoint(clickPoint);
  if (candidatePoints.empty())
    return;

  auto reachablePoints = GetReachablePoints(clickPoint, GetCoordinates(), *m_pointsDelegate,
                                            0 /* lookBackIndex */);
  auto const & clickablePoints = currentPathLength != 0
      ? GetReachablePoints(lastClickedPoint, GetCoordinates(), *m_pointsDelegate,
                           1 /* lookBackIndex */)
      // TODO(mgsergio): This is not quite correct since view port can change
      // since first call to visualize points. But it's ok in general.
      : m_pointsDelegate->GetAllJunctionPointsInViewPort();

  using ClickType = PointsControllerDelegateBase::ClickType;
  switch(m_pointsDelegate->CheckClick(clickPoint, lastClickedPoint, clickablePoints))
  {
    case ClickType::Add:
      LOG(LDEBUG, ("Adding point", clickPoint));
      // TODO(mgsergio): Think of refactoring this with if (accumulator.empty)
      // instead of pushing point first ad then removing last selection.
      PushPoint(clickPoint, candidatePoints);

      if (currentPathLength > 0)
      {
        // TODO(mgsergio): Should I remove lastClickedPoint from clickablePoints
        // as well?
        reachablePoints.erase(
            remove_if(begin(reachablePoints), end(reachablePoints),
                      [&lastClickedPoint](m2::PointD const & p) {
                        return p.EqualDxDy(lastClickedPoint, 1e-6);
                      }),
            end(reachablePoints));
        m_drawerDelegate->DrawGoldenPath(GetCoordinates());
      }

      m_drawerDelegate->ClearAllVisualizedPoints();
      m_drawerDelegate->VisualizePoints(reachablePoints);
      m_drawerDelegate->VisualizePoints({clickPoint});
      break;
    case ClickType::Remove:  // TODO(mgsergio): Rename this case.
      if (button == Qt::MouseButton::LeftButton)  // RemovePoint
      {
        m_drawerDelegate->ClearAllVisualizedPoints();
        m_drawerDelegate->ClearGoldenPath();

        PopPoint();
        if (m_goldenPath.empty())
        {
          m_drawerDelegate->VisualizePoints(
              m_pointsDelegate->GetAllJunctionPointsInViewPort());
        }
        else
        {
          // TODO(mgsergioe): Warning unused! Check this staff.
          auto const & prevPoint = GetLastPoint();
          m_drawerDelegate->VisualizePoints(
              GetReachablePoints(GetLastPoint(), GetCoordinates(), *m_pointsDelegate,
                                 1 /* lookBackIndex */));
        }

        if (GetPointsCount() > 1)
          m_drawerDelegate->DrawGoldenPath(GetCoordinates());
      }
      else if (button == Qt::MouseButton::RightButton)
      {
        CommitPath();
      }
      break;
    case ClickType::Miss:
      // TODO(mgsergio): This situation should be handled by checking candidatePoitns.empty() above.
      // Not shure though if all cases are handled by that check.
      return;
  }
}
