#pragma once

#include "indexer/index.hpp"

#include "generator/sponsored_dataset.hpp"

#include "search/reverse_geocoder.hpp"

#include "base/newtype.hpp"

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "std/function.hpp"
#include "std/map.hpp"
#include "std/string.hpp"

class FeatureBuilder1;

namespace generator
{
class BookingDataset : public SponsoredDatasetBase
{
public:

  // class AddressMatcher
  // {
  //   Index m_index;
  //   unique_ptr<search::ReverseGeocoder> m_coder;

  // public:
  //   AddressMatcher();
  //   void operator()(Hotel & hotel);
  // };

  explicit BookingDataset(string const & dataPath, string const & addressReferencePath = string())
    : SponsoredDatasetBase(dataPath, addressReferencePath)
  {
  }


  explicit BookingDataset(istream & dataSource, string const & addressReferencePath = string())
    : SponsoredDatasetBase(dataSource, addressReferencePath)
  {
  }

  bool NecessaryMatchingConditionHolds(FeatureBuilder1 const & fb) const override;

protected:
  void BuildObject(Object const & hotel, function<void(FeatureBuilder1 &)> const & fn) const override;

  ObjectId FindMatchingObjectIdImpl(FeatureBuilder1 const & e) const override;
};
}  // namespace generator
