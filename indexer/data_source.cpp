#include "indexer/data_source.hpp"
#include "indexer/scale_index.hpp"
#include "indexer/unique_index.hpp"

#include "platform/mwm_version.hpp"

#include <algorithm>
#include <limits>

using platform::CountryFile;
using platform::LocalCountryFile;

namespace
{
class ReadMWMFunctor
{
public:
  using Fn = std::function<void(uint32_t, FeatureSource & src)>;

  ReadMWMFunctor(FeatureSourceFactory const & factory, Fn const & fn) : m_factory(factory), m_fn(fn)
  {
    m_stop = []() { return false; };
  }

  ReadMWMFunctor(FeatureSourceFactory const & factory, Fn const & fn,
                 DataSource::StopSearchCallback const & stop)
    : m_factory(factory), m_fn(fn), m_stop(stop)
  {
  }

  // Reads features visible at |scale| covered by |cov| from mwm and applies |m_fn| to them.
  // Feature reading process consists of two steps: untouched (original) features reading and
  // touched (created, edited etc.) features reading.
  void operator()(MwmSet::MwmHandle const & handle, covering::CoveringGetter & cov, int scale) const
  {
    auto src = m_factory(handle);

    MwmValue const * mwmValue = handle.GetValue();
    if (mwmValue)
    {
      // Untouched (original) features reading. Applies covering |cov| to geometry index, gets
      // feature ids from it, gets untouched features by ids from |src| and applies |m_fn| by
      // ProcessElement.
      feature::DataHeader const & header = mwmValue->GetHeader();
      CheckUniqueIndexes checkUnique;

      // In case of WorldCoasts we should pass correct scale in ForEachInIntervalAndScale.
      auto const lastScale = header.GetLastScale();
      if (scale > lastScale)
        scale = lastScale;

      // Use last coding scale for covering (see index_builder.cpp).
      covering::Intervals const & intervals = cov.Get<RectId::DEPTH_LEVELS>(lastScale);
      ScaleIndex<ModelReaderPtr> index(mwmValue->m_cont.GetReader(INDEX_FILE_TAG), mwmValue->m_factory);

      // iterate through intervals
      for (auto const & i : intervals)
      {
        index.ForEachInIntervalAndScale(i.first, i.second, scale, [&](uint64_t /* key */, uint32_t value)
        {
          if (checkUnique(value))
            m_fn(value, *src);
        });
        if (m_stop())
          break;
      }
    }

    // Check created features container.
    // Need to do it on a per-mwm basis, because Drape relies on features in a sorted order.
    // Touched (created, edited) features reading.
    auto f = [&](uint32_t i) { m_fn(i, *src); };
    src->ForEachAdditionalFeature(cov.GetRect(), scale, f);
  }

private:
  FeatureSourceFactory const & m_factory;
  Fn m_fn;
  DataSource::StopSearchCallback m_stop;
};

void ReadFeatureType(std::function<void(FeatureType &)> const & fn, FeatureSource & src, uint32_t index)
{
  std::unique_ptr<FeatureType> ft;
  switch (src.GetFeatureStatus(index))
  {
  case FeatureStatus::Deleted:
  case FeatureStatus::Obsolete: return;
  case FeatureStatus::Created:
  case FeatureStatus::Modified:
  {
    ft = src.GetModifiedFeature(index);
    break;
  }
  case FeatureStatus::Untouched:
  {
    ft = src.GetOriginalFeature(index);
    break;
  }
  }
  CHECK(ft, ());
  fn(*ft);
}
}  //  namespace

// FeaturesLoaderGuard ---------------------------------------------------------------------
std::string FeaturesLoaderGuard::GetCountryFileName() const
{
  if (!m_handle.IsAlive())
    return {};

  return m_handle.GetValue()->GetCountryFileName();
}

bool FeaturesLoaderGuard::IsWorld() const
{
  if (!m_handle.IsAlive())
    return false;

  return m_handle.GetValue()->GetHeader().GetType() ==
         feature::DataHeader::MapType::World;
}

std::unique_ptr<FeatureType> FeaturesLoaderGuard::GetOriginalOrEditedFeatureByIndex(uint32_t index) const
{
  if (!m_handle.IsAlive())
    return {};

  ASSERT_NOT_EQUAL(m_source->GetFeatureStatus(index), FeatureStatus::Created, ());
  return GetFeatureByIndex(index);
}

std::unique_ptr<FeatureType> FeaturesLoaderGuard::GetFeatureByIndex(uint32_t index) const
{
  if (!m_handle.IsAlive())
    return {};

  ASSERT_NOT_EQUAL(FeatureStatus::Deleted, m_source->GetFeatureStatus(index),
                   ("Deleted feature was cached. It should not be here. Please review your code."));

  auto ft = m_source->GetModifiedFeature(index);
  if (ft)
    return ft;

  return GetOriginalFeatureByIndex(index);
}

std::unique_ptr<FeatureType> FeaturesLoaderGuard::GetOriginalFeatureByIndex(uint32_t index) const
{
  return m_handle.IsAlive() ? m_source->GetOriginalFeature(index) : nullptr;
}

// DataSource ----------------------------------------------------------------------------------
std::unique_ptr<MwmInfo> DataSource::CreateInfo(platform::LocalCountryFile const & localFile) const
{
  MwmValue value(localFile);

  feature::DataHeader const & h = value.GetHeader();

  auto info = std::make_unique<MwmInfoEx>();
  info->m_bordersRect = h.GetBounds();

  auto const scaleR = h.GetScaleRange();
  info->m_minScale = static_cast<uint8_t>(scaleR.first);
  info->m_maxScale = static_cast<uint8_t>(scaleR.second);
  info->m_version = value.GetMwmVersion();
  value.m_factory.MoveRegionData(info->m_data);

  return info;
}

std::unique_ptr<MwmValue> DataSource::CreateValue(MwmInfo & info) const
{
  platform::LocalCountryFile const & localFile = info.GetLocalFile();
  auto p = std::make_unique<MwmValue>(localFile);

  p->SetTable(dynamic_cast<MwmInfoEx &>(info));

  p->m_metaDeserializer = indexer::MetadataDeserializer::Load(p->m_cont);
  CHECK(p->m_metaDeserializer, ());
  return p;
}

std::pair<MwmSet::MwmId, MwmSet::RegResult> DataSource::RegisterMap(LocalCountryFile const & localFile)
{
  return Register(localFile);
}

bool DataSource::DeregisterMap(CountryFile const & countryFile) { return Deregister(countryFile); }

void DataSource::ForEachInIntervals(ReaderCallback const & fn, covering::CoveringMode mode,
                                    m2::RectD const & rect, int scale) const
{
  std::vector<std::shared_ptr<MwmInfo>> mwms;
  GetMwmsInfo(mwms);

  covering::CoveringGetter cov(rect, mode);

  MwmId worldID[2];

  for (auto const & info : mwms)
  {
    if (info->m_minScale <= scale && scale <= info->m_maxScale &&
        rect.IsIntersect(info->m_bordersRect))
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

void DataSource::ForEachFeatureIDInRect(FeatureIdCallback const & f, m2::RectD const & rect,
                                        int scale) const
{
  auto readFeatureId = [&f](uint32_t index, FeatureSource & src)
  {
    if (src.GetFeatureStatus(index) != FeatureStatus::Deleted)
      f({ src.GetMwmId(), index });
  };

  ReadMWMFunctor readFunctor(*m_factory, readFeatureId);
  ForEachInIntervals(readFunctor, covering::LowLevelsOnly, rect, scale);
}

void DataSource::ForEachInRect(FeatureCallback const & f, m2::RectD const & rect, int scale) const
{
  auto readFeatureType = [&f](uint32_t index, FeatureSource & src) {
    ReadFeatureType(f, src, index);
  };

  ReadMWMFunctor readFunctor(*m_factory, readFeatureType);
  ForEachInIntervals(readFunctor, covering::ViewportWithLowLevels, rect, scale);
}

void DataSource::ForClosestToPoint(FeatureCallback const & f, StopSearchCallback const & stop,
                                   m2::PointD const & center, double sizeM, int scale) const
{
  auto const rect = mercator::RectByCenterXYAndSizeInMeters(center, sizeM);

  auto readFeatureType = [&f](uint32_t index, FeatureSource & src) {
    ReadFeatureType(f, src, index);
  };
  ReadMWMFunctor readFunctor(*m_factory, readFeatureType, stop);
  ForEachInIntervals(readFunctor, covering::CoveringMode::Spiral, rect, scale);
}

void DataSource::ForEachInScale(FeatureCallback const & f, int scale) const
{
  auto readFeatureType = [&f](uint32_t index, FeatureSource & src) {
    ReadFeatureType(f, src, index);
  };

  ReadMWMFunctor readFunctor(*m_factory, readFeatureType);
  ForEachInIntervals(readFunctor, covering::FullCover, m2::RectD::GetInfiniteRect(), scale);
}

void DataSource::ForEachInRectForMWM(FeatureCallback const & f, m2::RectD const & rect, int scale,
                                     MwmId const & id) const
{
  MwmHandle const handle = GetMwmHandleById(id);
  if (handle.IsAlive())
  {
    covering::CoveringGetter cov(rect, covering::ViewportWithLowLevels);
    auto readFeatureType = [&f](uint32_t index, FeatureSource & src) {
      ReadFeatureType(f, src, index);
    };

    ReadMWMFunctor readFunctor(*m_factory, readFeatureType);
    readFunctor(handle, cov, scale);
  }
}

void DataSource::ReadFeatures(FeatureCallback const & fn, std::vector<FeatureID> const & features) const
{
  ASSERT(is_sorted(features.begin(), features.end()), ());

  auto fidIter = features.begin();
  auto const endIter = features.end();
  while (fidIter != endIter)
  {
    MwmId const & id = fidIter->m_mwmId;
    MwmHandle const handle = GetMwmHandleById(id);
    if (handle.IsAlive())
    {
      // Prepare features reading.
      auto src = (*m_factory)(handle);
      do
      {
        auto const fts = src->GetFeatureStatus(fidIter->m_index);
        ASSERT_NOT_EQUAL(
            FeatureStatus::Deleted, fts,
            ("Deleted feature was cached. It should not be here. Please review your code."));
        std::unique_ptr<FeatureType> ft;
        if (fts == FeatureStatus::Modified || fts == FeatureStatus::Created)
          ft = src->GetModifiedFeature(fidIter->m_index);
        else
          ft = src->GetOriginalFeature(fidIter->m_index);

        CHECK(ft, ());
        fn(*ft);
      } while (++fidIter != endIter && id == fidIter->m_mwmId);
    }
    else
    {
      // Skip unregistered mwm files.
      while (++fidIter != endIter && id == fidIter->m_mwmId)
        ;
    }
  }
}
