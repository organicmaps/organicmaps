#pragma once

#include "osrm2feature_map.hpp"
#include "router.hpp"
#include "osrm_data_facade.hpp"

#include "indexer/index.hpp"

#include "3party/osrm/osrm-backend/data_structures/query_edge.hpp"
//#include "../3party/osrm/osrm-backend/DataStructures/RawRouteData.h"

namespace routing
{
typedef OsrmDataFacade<QueryEdge::EdgeData> TDataFacade;

/// Datamapping and facade for single MWM and MWM.routing file
struct RoutingMapping
{
  TDataFacade m_dataFacade;
  OsrmFtSegMapping m_segMapping;
  CrossRoutingContextReader m_crossContext;

  ///@param fName: mwm file path
  RoutingMapping(string const & fName, Index const * pIndex);

  ~RoutingMapping();

  void Map();

  void Unmap();

  void LoadFacade();

  void FreeFacade();

  void LoadCrossContext();

  void FreeCrossContext();

  bool IsValid() const {return m_isValid;}

  IRouter::ResultCode GetError() const {return m_error;}

  string const & GetName() const { return m_baseName; }

  Index::MwmId const & GetMwmId() const { return m_mwmId; }

private:
  size_t m_mapCounter;
  size_t m_facadeCounter;
  bool m_crossContextLoaded;
  string m_baseName;
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
}
