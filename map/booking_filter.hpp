#pragma once

#include "map/booking_filter_params.hpp"

#include "base/macros.hpp"

#include <memory>

struct FeatureID;
class DataSource;

namespace search
{
class Results;
}

namespace booking
{
class Api;

namespace filter
{
class FilterBase
{
public:
  class Delegate
  {
  public:
    virtual ~Delegate() = default;

    virtual DataSource const & GetDataSource() const = 0;
    virtual Api const & GetApi() const = 0;
  };

  explicit FilterBase(Delegate const & d) : m_delegate(d) {}
  virtual ~FilterBase() = default;

  virtual void ApplyFilter(search::Results const & results,
                           ParamsInternal const & filterParams) = 0;
  virtual void ApplyFilter(std::vector<FeatureID> const & featureIds,
                           ParamsRawInternal const & params) = 0;
  virtual void GetFeaturesFromCache(search::Results const & results,
                                    std::vector<FeatureID> & sortedResults,
                                    std::vector<Extras> & extras,
                                    std::vector<FeatureID> & filteredOut) = 0;
  virtual void UpdateParams(ParamsBase const & apiParams) = 0;

  Delegate const & GetDelegate() const { return m_delegate; }

private:
  Delegate const & m_delegate;
};

using FilterPtr = std::unique_ptr<FilterBase>;
}  // namespace filter
}  // namespace booking
