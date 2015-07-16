#include "routing/routing_mapping.h"

#include "routing/cross_routing_context.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"

#include "base/logging.hpp"

#include "coding/reader_wrapper.hpp"

#include "indexer/mwm_version.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/platform.hpp"

using platform::CountryFile;
using platform::LocalCountryFile;

namespace {
/*!
 * \brief CheckMwmConsistency checks versions of mwm and routing files.
 * \param localFile reference to country file we need to check.
 * \return true if files has same versions.
 * \warning Function assumes that the file lock was already taken.
 */
bool CheckMwmConsistency(platform::LocalCountryFile const & localFile)
{
  ModelReaderPtr r1 = FilesContainerR(localFile.GetPath(TMapOptions::ECarRouting))
      .GetReader(VERSION_FILE_TAG);
  ReaderSrc src1(r1.GetPtr());
  ModelReaderPtr r2 = FilesContainerR(localFile.GetPath(TMapOptions::EMap))
      .GetReader(VERSION_FILE_TAG);
  ReaderSrc src2(r2.GetPtr());

  version::MwmVersion version1;
  version::ReadVersion(src1, version1);

  version::MwmVersion version2;
  version::ReadVersion(src2, version2);

  return version1.timestamp == version2.timestamp;
}
} //  namespace

namespace routing
{
RoutingMapping::RoutingMapping(CountryFile const & countryFile)
    : m_mapCounter(0),
      m_facadeCounter(0),
      m_crossContextLoaded(0),
      m_countryFile(countryFile),
      m_error(IRouter::ResultCode::RouteFileNotExist),
      m_handle()
{
}

RoutingMapping::RoutingMapping(CountryFile const & countryFile, MwmSet * pIndex)
    : m_mapCounter(0),
      m_facadeCounter(0),
      m_crossContextLoaded(0),
      m_countryFile(countryFile),
      m_error(IRouter::ResultCode::RouteFileNotExist),
      m_handle(pIndex->GetMwmHandleByCountryFile(countryFile))
{
  if (!m_handle.IsAlive())
    return;
  LocalCountryFile const & localFile = m_handle.GetInfo()->GetLocalFile();
  if (!HasOptions(localFile.GetFiles(), TMapOptions::EMapWithCarRouting))
  {
    m_handle = MwmSet::MwmHandle();
    return;
  }

  m_container.Open(localFile.GetPath(TMapOptions::ECarRouting));
  if (!CheckMwmConsistency(localFile))
  {
    m_error = IRouter::ResultCode::InconsistentMWMandRoute;
    m_container.Close();
    m_handle = MwmSet::MwmHandle();
    return;
  }

  m_error = IRouter::ResultCode::NoError;
}

RoutingMapping::~RoutingMapping()
{
  // Clear data while m_container is valid.
  m_dataFacade.Clear();
  m_segMapping.Clear();
  m_container.Close();
}

void RoutingMapping::Map()
{
  ++m_mapCounter;
  if (!m_segMapping.IsMapped())
  {
    m_segMapping.Load(m_container, m_handle.GetInfo()->GetLocalFile());
    m_segMapping.Map(m_container);
  }
}

void RoutingMapping::Unmap()
{
  --m_mapCounter;
  if (m_mapCounter < 1 && m_segMapping.IsMapped())
    m_segMapping.Unmap();
}

void RoutingMapping::LoadFacade()
{
  if (!m_facadeCounter)
  {
    m_dataFacade.Load(m_container);
  }
  ++m_facadeCounter;
}

void RoutingMapping::FreeFacade()
{
  --m_facadeCounter;
  if (!m_facadeCounter)
    m_dataFacade.Clear();
}

void RoutingMapping::LoadCrossContext()
{
  if (m_crossContextLoaded)
    return;

  if (m_container.IsExist(ROUTING_CROSS_CONTEXT_TAG))
  {
    m_crossContext.Load(m_container.GetReader(ROUTING_CROSS_CONTEXT_TAG));
    m_crossContextLoaded = true;
  }
}

void RoutingMapping::FreeCrossContext()
{
  m_crossContextLoaded = false;
  m_crossContext = CrossRoutingContextReader();
}

// static
shared_ptr<RoutingMapping> RoutingMapping::MakeInvalid(platform::CountryFile const & countryFile)
{
  return shared_ptr<RoutingMapping>(new RoutingMapping(countryFile));
}

TRoutingMappingPtr RoutingIndexManager::GetMappingByPoint(m2::PointD const & point)
{
  return GetMappingByName(m_countryFileFn(point));
}

TRoutingMappingPtr RoutingIndexManager::GetMappingByName(string const & mapName)
{
  // Check if we have already loaded this file.
  auto mapIter = m_mapping.find(mapName);
  if (mapIter != m_mapping.end())
    return mapIter->second;

  // Or load and check file.
  TRoutingMappingPtr newMapping =
      make_shared<RoutingMapping>(platform::CountryFile(mapName), m_index);
  m_mapping.insert(make_pair(mapName, newMapping));
  return newMapping;
}

}  // namespace routing
