#pragma once

#include "map/discovery/discovery_search_params.hpp"
#include "map/search_product_info.hpp"

#include "indexer/feature_decl.hpp"
#include "indexer/mwm_set.hpp"
#include "indexer/rank_table.hpp"

#include "search/result.hpp"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <vector>

class DataSource;

namespace discovery
{
class SearchBase
{
public:
  SearchBase(DataSource const & dataSource, DiscoverySearchParams const & params,
             search::ProductInfo::Delegate const & productInfoDelegate);

  virtual ~SearchBase() = default;

  void Search();

  search::Results const & GetResults() const;
  std::vector<search::ProductInfo> const & GetProductInfo() const;

protected:
  DataSource const & GetDataSource() const;
  DiscoverySearchParams const & GetParams() const;
  void AppendResult(search::Result && result);

private:
  virtual void OnMwmChanged(MwmSet::MwmHandle const & handle);
  virtual void ProcessFeatureId(FeatureID const & id) = 0;
  virtual void ProcessAccumulated() = 0;

  DataSource const & m_dataSource;
  DiscoverySearchParams const m_params;
  search::ProductInfo::Delegate const & m_productInfoDelegate;

  search::Results m_results;
  std::vector<search::ProductInfo> m_productInfo;
};

struct SearchHotels : public SearchBase
{
public:
  SearchHotels(DataSource const & dataSource, DiscoverySearchParams const & params,
               search::ProductInfo::Delegate const & productInfoDelegate);

private:
  // SearchBase overrides:
  void ProcessFeatureId(FeatureID const & id) override;
  void ProcessAccumulated() override;

  std::vector<FeatureID> m_featureIds;
};

struct SearchPopularPlaces : public SearchBase
{
public:
  SearchPopularPlaces(DataSource const & dataSource, DiscoverySearchParams const & params,
                      search::ProductInfo::Delegate const & productInfoDelegate);

private:
  // SearchBase overrides:
  void OnMwmChanged(MwmSet::MwmHandle const & handle) override;
  void ProcessFeatureId(FeatureID const & id) override;
  void ProcessAccumulated() override;

  std::unique_ptr<search::RankTable> m_popularityRanks;
  std::multimap<uint8_t, FeatureID, std::greater<uint8_t>> m_accumulatedResults;
};

void ProcessSearchIntent(std::shared_ptr<SearchBase> intent);
}  // namespace discovery
