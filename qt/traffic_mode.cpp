#include "qt/traffic_mode.hpp"

#include "openlr/openlr_simple_parser.hpp"

#include "indexer/index.hpp"
#include "indexer/scales.hpp"

#include "base/stl_add.hpp"

#include "3party/pugixml/src/pugixml.hpp"

#include <QtCore/QItemSelection>

// DecodedSample -----------------------------------------------------------------------------------
DecodedSample::DecodedSample(Index const & index, openlr::SamplePool const & sample)
{
  for (auto const & item : sample)
  {
    m_decodedItems.push_back(item);
    for (auto const & mwmSegment : item.m_segments)
    {
      auto const & fid = mwmSegment.m_fid;
      Index::FeaturesLoaderGuard g(index, fid.m_mwmId);
      CHECK(fid.m_mwmId.IsAlive(), ("Mwm id is not alive."));
      if (m_points.find(fid) != end(m_points))
        continue;

      FeatureType ft;
      CHECK(g.GetFeatureByIndex(fid.m_index, ft), ("Can't read feature", fid));
      ft.ParseGeometry(FeatureType::BEST_GEOMETRY);

      auto & v = m_points[fid];
      v.reserve(ft.GetPointsCount());
      ft.ForEachPoint(MakeBackInsertFunctor(v), scales::GetUpperScale());
    }
  }
}

std::vector<m2::PointD> DecodedSample::GetPoints(size_t const index) const
{
  std::vector<m2::PointD> points;

  auto const pushPoint = [&points](m2::PointD const & p)
  {
    if (points.empty() || !(points.back() - p).IsAlmostZero())
      points.push_back(p);
  };

  auto const & item = m_decodedItems[index];
  for (auto const & seg : item.m_segments)
  {
    auto const ftIt = m_points.find(seg.m_fid);
    CHECK(ftIt != end(m_points), ("Can't find feature with id:", seg.m_fid));
    auto const & ftPoints = ftIt->second;
    CHECK_LESS(seg.m_segId + 1, ftPoints.size(), ());
    auto const firstP = ftPoints[seg.m_segId];
    auto const secondP = ftPoints[seg.m_segId + 1];
    if (seg.m_isForward)
    {
      pushPoint(firstP);
      pushPoint(secondP);
    }
    else
    {
      pushPoint(secondP);
      pushPoint(firstP);
    }
  }

  return points;
}

// TrafficMode -------------------------------------------------------------------------------------
TrafficMode::TrafficMode(std::string const & dataFileName, std::string const & sampleFileName,
                         Index const & index, std::unique_ptr<TrafficDrawerDelegateBase> drawerDelagate,
                         QObject * parent)
  : QAbstractTableModel(parent)
  , m_drawerDelegate(move(drawerDelagate))
{
  try
  {
    auto const & sample = openlr::LoadSamplePool(sampleFileName, index);
    m_decodedSample = make_unique<DecodedSample>(index, sample);
  }
  catch (openlr::SamplePoolLoadError const & e)
  {
    LOG(LERROR, (e.Msg()));
    return;
  }

  pugi::xml_document doc;
  if (!doc.load_file(dataFileName.data()))
  {
    LOG(LERROR, ("Can't load file:", dataFileName));
    return;
  }

  std::vector<openlr::LinearSegment> segments;
  if (!ParseOpenlr(doc, segments))
  {
    LOG(LERROR, ("Can't parse data:", dataFileName));
    return;
  }
  for (auto const & segment : segments)
  {
    CHECK(!segment.m_locationReference.m_points.empty(), ());
    m_partnerSegments[segment.m_segmentId] = segment;
  }

  m_valid = true;
}

bool TrafficMode::SaveSampleAs(std::string const & fileName) const
{
  try
  {
    auto const & samplePool = m_decodedSample->GetItems();
    openlr::SaveSamplePool(fileName, samplePool, true /* saveEvaluation */);
  }
  catch (openlr::SamplePoolSaveError const & e)
  {
    LOG(LERROR, (e.Msg()));
    return false;
  }
  return true;
}

int TrafficMode::rowCount(const QModelIndex & parent) const
{
  if (!m_decodedSample)
    return 0;
  return m_decodedSample->m_decodedItems.size();
}

int TrafficMode::columnCount(const QModelIndex & parent) const
{
  return 2;
}

QVariant TrafficMode::data(const QModelIndex & index, int role) const
{
  if (!index.isValid())
    return QVariant();

  if (index.row() >= rowCount())
    return QVariant();

  if (role != Qt::DisplayRole && role != Qt::EditRole)
    return QVariant();

  if (index.column() == 0)
    return ToString(m_decodedSample->m_decodedItems[index.row()].m_evaluation).data();

  if (index.column() == 1)
    return m_decodedSample->m_decodedItems[index.row()].m_partnerSegmentId.Get();

  return QVariant();
}

void TrafficMode::OnItemSelected(QItemSelection const & selected, QItemSelection const &)
{
  ASSERT(!selected.empty(), ("The selection should not be empty. RTFM for qt5."));
  auto const row = selected.front().top();

  // TODO(mgsergio): Use algo for center calculation.
  // Now viewport is set to the first point of the first segment.
  auto const partnerSegmentId = m_decodedSample->m_decodedItems[row].m_partnerSegmentId;

  if (m_decodedSample->m_decodedItems[row].m_segments.empty())
  {
    LOG(LERROR, ("Empty mwm segments for partner id", partnerSegmentId.Get()));
    return;
  }

  auto const & firstSegment = m_decodedSample->m_decodedItems[row].m_segments[0];
  auto const & firstSegmentFeatureId = firstSegment.m_fid;
  auto const & firstSegmentFeature = m_decodedSample->m_points.at(firstSegmentFeatureId);

  LOG(LDEBUG, ("PartnerSegmentId:", partnerSegmentId.Get(),
               "Segment points:", m_partnerSegments[partnerSegmentId.Get()].GetMercatorPoints(),
               "Featrue segment id", firstSegment.m_segId,
               "Feature segment points", firstSegmentFeature[firstSegment.m_segId],
                                         firstSegmentFeature[firstSegment.m_segId + 1]));

  m_drawerDelegate->Clear();
  m_drawerDelegate->SetViewportCenter(firstSegmentFeature[firstSegment.m_segId]);
  m_drawerDelegate->DrawEncodedSegment(m_partnerSegments.at(partnerSegmentId.Get()));
  m_drawerDelegate->DrawDecodedSegments(*m_decodedSample, row);
}

Qt::ItemFlags TrafficMode::flags(QModelIndex const & index) const
{
  if (!index.isValid())
    return Qt::ItemIsEnabled;

  if (index.column() != 0)
    QAbstractItemModel::flags(index);

  return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
}

bool TrafficMode::setData(QModelIndex const & index, QVariant const & value, int const role)
{
  if (!index.isValid() || role != Qt::EditRole)
    return false;

  auto const newValue = value.toString();
  auto evaluation = openlr::ParseItemEvaluation(newValue.toStdString());
  if (evaluation == openlr::ItemEvaluation::NotAValue)
    return false;

  m_decodedSample->m_decodedItems[index.row()].m_evaluation = evaluation;
  emit dataChanged(index, index);
  return true;
}
