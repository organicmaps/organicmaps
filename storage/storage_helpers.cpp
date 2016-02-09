#include "storage/storage_helpers.hpp"

#include "storage/country_info_getter.hpp"
#include "storage/storage.hpp"

namespace
{
using namespace storage;

class CalcLimitRectAccumulator
{
  m2::RectD m_boundBox;
 CountryInfoGetter const & m_countryInfoGetter;
public:
  CalcLimitRectAccumulator(CountryInfoGetter const & countryInfoGetter)
    : m_countryInfoGetter(countryInfoGetter) {}

  m2::RectD GetBoundBox() { return m_boundBox; }
  void operator()(TCountryId const & descendantCountryId, bool expandableNode)
  {
    if (!expandableNode)
      m_boundBox.Add(m_countryInfoGetter.CalcLimitRectForLeaf(descendantCountryId));
  }
};
} // namespace

namespace storage
{
bool IsPointCoveredByDownloadedMaps(m2::PointD const & position,
                                    Storage const & storage,
                                    CountryInfoGetter const & countryInfoGetter)
{
  return storage.IsNodeDownloaded(countryInfoGetter.GetRegionCountryId(position));
}

bool IsDownloadFailed(Status status)
{
  return status == Status::EDownloadFailed || status == Status::EOutOfMemFailed ||
         status == Status::EUnknown;
}

m2::RectD CalcLimitRect(TCountryId countryId,
                        Storage const & storage,
                        CountryInfoGetter const & countryInfoGetter)
{
  CalcLimitRectAccumulator accumulater(countryInfoGetter);
  storage.ForEachInSubtree(countryId, accumulater);
  m2::RectD const boundBox = accumulater.GetBoundBox();

  ASSERT(boundBox.IsValid(), ());

  return accumulater.GetBoundBox();
}
} // namespace storage
