#pragma once

#include "osrm2feature_map.hpp"
#include "osrm_data_facade.hpp"
#include "router.hpp"

#include "indexer/index.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"

#include "std/algorithm.hpp"
#include "std/unordered_map.hpp"


namespace routing
{
using TDataFacade = OsrmDataFacade<QueryEdge::EdgeData>;

/// Datamapping and facade for single MWM and MWM.routing file
struct RoutingMapping
{
  TDataFacade m_dataFacade;
  OsrmFtSegMapping m_segMapping;
  CrossRoutingContextReader m_crossContext;

  /// Default constructor to create invalid instance for existing client code.
  /// @postcondition IsValid() == false.
  RoutingMapping() : m_pIndex(nullptr), m_error(IRouter::ResultCode::RouteFileNotExist) {}
  /// @param countryFile Country file name without extension.
  RoutingMapping(string const & countryFile, MwmSet & index);
  ~RoutingMapping();

  void Map();
  void Unmap();

  void LoadFacade();
  void FreeFacade();

  void LoadCrossContext();
  void FreeCrossContext();

  bool IsValid() const { return m_error == IRouter::ResultCode::NoError && m_mwmId.IsAlive(); }

  IRouter::ResultCode GetError() const { return m_error; }

  /*!
   * Returns mentioned country file. Works even the LocalCountryFile does not exist and
   * the lock was not taken.
   */
  string const & GetCountryName() const { return m_countryFile; }

  Index::MwmId const & GetMwmId() const { return m_mwmId; }

  // Free file handles if it is possible. Works only if there is no mapped sections. Cross section
  // will be valid after free.
  void FreeFileIfPossible();

private:
  void LoadFileIfNeeded();

  size_t m_mapCounter;
  size_t m_facadeCounter;
  bool m_crossContextLoaded;
  string m_countryFile;
  FilesMappingContainer m_container;
  IRouter::ResultCode m_error;
  MwmSet::MwmHandle m_handle;
  // We save a mwmId for possibility to unlock a mwm file by rewriting m_handle.
  Index::MwmId m_mwmId;
  MwmSet * m_pIndex;
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
  RoutingIndexManager(TCountryFileFn const & countryFileFn, MwmSet & index)
      : m_countryFileFn(countryFileFn), m_index(index)
  {
  }

  TRoutingMappingPtr GetMappingByPoint(m2::PointD const & point);

  TRoutingMappingPtr GetMappingByName(string const & mapName);

  TRoutingMappingPtr GetMappingById(Index::MwmId const & id);

  template <class TFunctor>
  void ForEachMapping(TFunctor toDo)
  {
    for_each(m_mapping.begin(), m_mapping.end(), toDo);
  }

  void Clear() { m_mapping.clear(); }

private:
  TCountryFileFn m_countryFileFn;
  // TODO (ldragunov) Rewrite to mwmId.
  unordered_map<string, TRoutingMappingPtr> m_mapping;
  MwmSet & m_index;
};

}  // namespace routing
