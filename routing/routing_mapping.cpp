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

namespace routing
{
RoutingMapping::RoutingMapping(CountryFile const & countryFile)
    : m_mapCounter(0),
      m_facadeCounter(0),
      m_crossContextLoaded(0),
      m_countryFileName(countryFile.GetNameWithoutExt()),
      m_isValid(false),
      m_error(IRouter::ResultCode::RouteFileNotExist)
{
}

RoutingMapping::RoutingMapping(LocalCountryFile const & localFile, Index const * pIndex)
    : m_mapCounter(0),
      m_facadeCounter(0),
      m_crossContextLoaded(0),
      m_countryFileName(localFile.GetCountryFile().GetNameWithoutExt()),
      m_isValid(true),
      m_error(IRouter::ResultCode::NoError)
{
  Platform & pl = GetPlatform();
  if (!HasOptions(localFile.GetFiles(), TMapOptions::EMapWithCarRouting))
  {
    m_isValid = false;
    m_error = IRouter::ResultCode::RouteFileNotExist;
    return;
  }
  string const routingFilePath = localFile.GetPath(TMapOptions::ECarRouting);
  // Open new container and check that mwm and routing have equal timestamp.
  LOG(LDEBUG, ("Load routing index for file:", routingFilePath));
  m_container.Open(routingFilePath);
  {
    FileReader r1 = m_container.GetReader(VERSION_FILE_TAG);
    ReaderSrc src1(r1);
    ModelReaderPtr r2 = FilesContainerR(pl.GetCountryReader(localFile, TMapOptions::EMap))
                            .GetReader(VERSION_FILE_TAG);
    ReaderSrc src2(r2.GetPtr());

    version::MwmVersion version1;
    version::ReadVersion(src1, version1);

    version::MwmVersion version2;
    version::ReadVersion(src2, version2);

    if (version1.timestamp != version2.timestamp)
    {
      m_container.Close();
      m_isValid = false;
      m_error = IRouter::ResultCode::InconsistentMWMandRoute;
      return;
    }
  }

  m_mwmId = pIndex->GetMwmIdByCountryFile(localFile.GetCountryFile());
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
    m_segMapping.Load(m_container);
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
    m_dataFacade.Load(m_container);
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
  if (!m_crossContextLoaded)
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
  shared_ptr<platform::LocalCountryFile> localFile = m_countryLocalFileFn(mapName);
  // Return invalid mapping when file does not exist.
  if (!localFile.get())
    return RoutingMapping::MakeInvalid(platform::CountryFile(mapName));

  // Check if we have already loaded this file.
  auto mapIter = m_mapping.find(mapName);
  if (mapIter != m_mapping.end())
    return mapIter->second;

  // Or load and check file.
  TRoutingMappingPtr newMapping = make_shared<RoutingMapping>(*localFile, m_index);
  m_mapping.insert(make_pair(mapName, newMapping));
  return newMapping;
}

}  // namespace routing
