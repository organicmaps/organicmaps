#pragma once

#include "osrm2feature_map.hpp"
#include "osrm_data_facade.hpp"
#include "router.hpp"

#include "indexer/index.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"

#include "std/algorithm.hpp"
#include "std/unordered_map.hpp"

namespace platform
{
class CountryFile;
class LocalCountryFile;
}

namespace routing
{
using TDataFacade = OsrmDataFacade<QueryEdge::EdgeData>;
using TCountryFileFn = function<string(m2::PointD const &)>;
using TCountryLocalFileFn = function<shared_ptr<platform::LocalCountryFile>(string const &)>;

/// Datamapping and facade for single MWM and MWM.routing file
struct RoutingMapping
{
  TDataFacade m_dataFacade;
  OsrmFtSegMapping m_segMapping;
  CrossRoutingContextReader m_crossContext;

  ///@param fName: mwm file path
  RoutingMapping(platform::LocalCountryFile const & localFile, Index const * pIndex);

  ~RoutingMapping();

  void Map();

  void Unmap();

  void LoadFacade();

  void FreeFacade();

  void LoadCrossContext();

  void FreeCrossContext();

  bool IsValid() const {return m_isValid;}

  IRouter::ResultCode GetError() const {return m_error;}

  string const & GetName() const { return m_countryFileName; }

  Index::MwmId const & GetMwmId() const { return m_mwmId; }

  // static
  static shared_ptr<RoutingMapping> MakeInvalid(platform::CountryFile const & countryFile);

private:
  // Ctor for invalid mappings.
  RoutingMapping(platform::CountryFile const & countryFile);

  size_t m_mapCounter;
  size_t m_facadeCounter;
  bool m_crossContextLoaded;
  string m_countryFileName;
  FilesMappingContainer m_container;
  Index::MwmId m_mwmId;
  bool m_isValid;
  IRouter::ResultCode m_error;
};

typedef shared_ptr<RoutingMapping> TRoutingMappingPtr;

//! \brief The MappingGuard struct. Asks mapping to load all data on construction and free it on destruction
class MappingGuard
{
  TRoutingMappingPtr const m_mapping;

public:
  MappingGuard(TRoutingMappingPtr const mapping): m_mapping(mapping)
  {
    m_mapping->Map();
    m_mapping->LoadFacade();
  }

  ~MappingGuard()
  {
    m_mapping->Unmap();
    m_mapping->FreeFacade();
  }
};

/*! Manager for loading, cashing and building routing indexes.
 * Builds and shares special routing contexts.
*/
class RoutingIndexManager
{
public:
  RoutingIndexManager(TCountryFileFn const & countryFileFn,
                      TCountryLocalFileFn const & countryLocalFileFn, Index const * index)
      : m_countryFileFn(countryFileFn), m_countryLocalFileFn(countryLocalFileFn), m_index(index)
  {
    ASSERT(index, ());
  }

  TRoutingMappingPtr GetMappingByPoint(m2::PointD const & point);

  TRoutingMappingPtr GetMappingByName(string const & mapName);

  template <class TFunctor>
  void ForEachMapping(TFunctor toDo)
  {
    for_each(m_mapping.begin(), m_mapping.end(), toDo);
  }

  void Clear() { m_mapping.clear(); }

private:
  TCountryFileFn m_countryFileFn;
  TCountryLocalFileFn m_countryLocalFileFn;
  unordered_map<string, TRoutingMappingPtr> m_mapping;
  Index const * m_index;
};

}  // namespace routing
