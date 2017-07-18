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
}

// TrafficMode -------------------------------------------------------------------------------------
TrafficMode::TrafficMode(std::string const & dataFileName, Index const & index,
                         std::unique_ptr<TrafficDrawerDelegateBase> drawerDelagate,
                         QObject * parent)
  : QAbstractTableModel(parent), m_index(index), m_drawerDelegate(move(drawerDelagate))
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

  m_drawerDelegate->Clear();
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
