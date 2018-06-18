#include "indexer/data_source.hpp"

#include "base/logging.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;
using namespace std;

//////////////////////////////////////////////////////////////////////////////////
// DataSourceBase implementation
//////////////////////////////////////////////////////////////////////////////////

unique_ptr<MwmInfo> DataSourceBase::CreateInfo(platform::LocalCountryFile const & localFile) const
{
  MwmValue value(localFile);

  feature::DataHeader const & h = value.GetHeader();
  if (!h.IsMWMSuitable())
    return nullptr;

  auto info = make_unique<MwmInfoEx>();
  info->m_limitRect = h.GetBounds();

  pair<int, int> const scaleR = h.GetScaleRange();
  info->m_minScale = static_cast<uint8_t>(scaleR.first);
  info->m_maxScale = static_cast<uint8_t>(scaleR.second);
  info->m_version = value.GetMwmVersion();
  // Copying to drop the const qualifier.
  feature::RegionData regionData(value.GetRegionData());
  info->m_data = regionData;

  return unique_ptr<MwmInfo>(move(info));
}

unique_ptr<MwmSet::MwmValueBase> DataSourceBase::CreateValue(MwmInfo & info) const
{
  // Create a section with rank table if it does not exist.
  platform::LocalCountryFile const & localFile = info.GetLocalFile();
  unique_ptr<MwmValue> p(new MwmValue(localFile));
  p->SetTable(dynamic_cast<MwmInfoEx &>(info));
  ASSERT(p->GetHeader().IsMWMSuitable(), ());
  return unique_ptr<MwmSet::MwmValueBase>(move(p));
}

pair<MwmSet::MwmId, MwmSet::RegResult> DataSourceBase::RegisterMap(LocalCountryFile const & localFile)
{
  return Register(localFile);
}

bool DataSourceBase::DeregisterMap(CountryFile const & countryFile) { return Deregister(countryFile); }

void DataSourceBase::ForEachInIntervals(ReaderCallback const & fn, covering::CoveringMode mode,
                               m2::RectD const & rect, int scale) const
{
  vector<shared_ptr<MwmInfo>> mwms;
  GetMwmsInfo(mwms);

  covering::CoveringGetter cov(rect, mode);

  MwmId worldID[2];

  for (shared_ptr<MwmInfo> const & info : mwms)
  {
    if (info->m_minScale <= scale && scale <= info->m_maxScale &&
        rect.IsIntersect(info->m_limitRect))
    {
      MwmId const mwmId(info);
      switch (info->GetType())
      {
      case MwmInfo::COUNTRY: fn(GetMwmHandleById(mwmId), cov, scale); break;
      case MwmInfo::COASTS: worldID[0] = mwmId; break;
      case MwmInfo::WORLD: worldID[1] = mwmId; break;
      }
    }
  }

  if (worldID[0].IsAlive())
    fn(GetMwmHandleById(worldID[0]), cov, scale);

  if (worldID[1].IsAlive())
    fn(GetMwmHandleById(worldID[1]), cov, scale);
}
