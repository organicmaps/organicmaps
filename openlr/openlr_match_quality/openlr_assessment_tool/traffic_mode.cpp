#include "openlr/openlr_match_quality/openlr_assessment_tool/traffic_mode.hpp"

#include "openlr/openlr_model_xml.hpp"

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "3party/pugixml/src/pugixml.hpp"

#include <QItemSelection>

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

      // TODO(mgsergio): Unify error handling interface of openlr_xml_mode and decoded_path parsers.
      if (!openlr::SegmentFromXML(xmlSegment.child("reportSegments"), segment.GetPartnerSegment()))
        MYTHROW(TrafficModeError, ("An error occured while parsing: can't parse segment"));

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
    }
  }
  catch (openlr::DecodedPathLoadError const & e)
  {
    MYTHROW(TrafficModeError, ("An exception occured while parsing", dataFileName, e.Msg()));
  }

  // TODO(mgsergio): LOG(LINFO, (xxx, "segments are loaded"));
}

bool TrafficMode::SaveSampleAs(std::string const & fileName) const
{
  // TODO(mgsergio): Remove #include "base/macros.hpp" when implemented;
  NOTIMPLEMENTED();
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
  auto & segment = m_segments[row];
  auto const partnerSegmentId = segment.GetPartnerSegmentId();

  // TODO(mgsergio): Maybe we shold show empty paths.
  CHECK(segment.GetMatchedPath(), ("Empty mwm segments for partner id", partnerSegmentId));

  auto const & path = *segment.GetMatchedPath();
  auto const & firstEdge = path.front();

  m_drawerDelegate->ClearAllPaths();
  m_drawerDelegate->SetViewportCenter(GetStart(firstEdge));
  m_drawerDelegate->DrawEncodedSegment(GetPoints(segment.GetPartnerSegment()));
  m_drawerDelegate->DrawDecodedSegments(GetPoints(path));
}

Qt::ItemFlags TrafficMode::flags(QModelIndex const & index) const
{
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  return QAbstractItemModel::flags(index);
}


void TrafficMode::StartBuildingPath()
{
  if (m_buildingPath)
    MYTHROW(TrafficModeError, ("Path building already in progress."));
  m_buildingPath = true;
  m_drawerDelegate->VisualizePoints(
      m_pointsDelegate->GetAllJunctionPointsInViewPort());
}

void TrafficMode::PushPoint(m2::PointD const & coord, std::vector<FeaturePoint> const & points)
{
  impl::RoadPointCandidate point(points, coord);
  if (!m_path.empty())
    m_path.back().ActivatePoint(point);
  m_path.push_back(point);
}

void TrafficMode::PopPoint()
{
  CHECK(!m_path.empty(), ("Attempt to pop point from an empty path."));
  m_path.pop_back();
}

void TrafficMode::CommitPath()
{
  if (!m_buildingPath)
    MYTHROW(TrafficModeError, ("Path building is not started"));
  m_buildingPath = false;
}

void TrafficMode::RollBackPath()
{
}

size_t TrafficMode::GetPointsCount() const
{
  return m_path.size();
}

m2::PointD const & TrafficMode::GetPoint(size_t const index) const
{
  return m_path[index].GetCoordinate();
}

m2::PointD const & TrafficMode::GetLastPoint() const
{
  CHECK(!m_path.empty(), ("Attempt to get point from an empty path."));
  return m_path.back().GetCoordinate();
}

std::vector<m2::PointD> TrafficMode::GetCoordinates() const
{
  std::vector<m2::PointD> coordinates;
  for (auto const & roadPoint : m_path)
    coordinates.push_back(roadPoint.GetCoordinate());
  return coordinates;
}

// TODO(mgsergio): Draw the first point when the path size is 1.
void TrafficMode::HandlePoint(m2::PointD clickPoint)
{
  auto const currentPathLength = GetPointsCount();
  auto const lastClickedPoint = currentPathLength != 0
      ? GetLastPoint()
      : m2::PointD::Zero();

  // Get candidates and also fix clickPoint to make it more accurate and sutable for
  // GetOutgoingEdges.
  auto const & candidatePoints = m_pointsDelegate->GetFeaturesPointsByPoint(clickPoint);
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
        m_drawerDelegate->VisualizeGoldenPath(GetCoordinates());
      }

      m_drawerDelegate->CleanAllVisualizedPoints();
      m_drawerDelegate->VisualizePoints(reachablePoints);
      m_drawerDelegate->VisualizePoints({clickPoint});
      break;
    case ClickType::Remove:
      // if (button == Qt::MouseButton::LeftButton)  // RemovePoint
      // {
      m_drawerDelegate->CleanAllVisualizedPoints();
      // TODO(mgsergio): Remove only golden path.
      m_drawerDelegate->ClearAllPaths();

      PopPoint();
      if (m_path.empty())
      {
        m_drawerDelegate->VisualizePoints(
            m_pointsDelegate->GetAllJunctionPointsInViewPort());
      }
      else
      {
        auto const & prevPoint = GetLastPoint();
        m_drawerDelegate->VisualizePoints(
            GetReachablePoints(GetLastPoint(), GetCoordinates(), *m_pointsDelegate,
                               1 /* lookBackIndex */));
      }

      if (GetPointsCount() > 1)
        m_drawerDelegate->VisualizeGoldenPath(GetCoordinates());
      // }
      // else if (botton == Qt::MouseButton::RightButton)
      // {
      //   // TODO(mgsergio): Implement comit path.
      // }
      break;
    case ClickType::Miss:
      LOG(LDEBUG, ("You miss"));
      return;
  }
}
