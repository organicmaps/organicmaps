#include "routing_mapping.hpp"

#include "routing/cross_routing_context.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"

#include "platform/country_file.hpp"
#include "platform/local_country_file.hpp"
#include "platform/mwm_version.hpp"
#include "platform/platform.hpp"

#include "coding/reader_wrapper.hpp"

#include "base/logging.hpp"


using platform::CountryFile;
using platform::LocalCountryFile;

namespace
{
/*!
 * \brief CheckMwmConsistency checks versions of mwm and routing files.
 * \param localFile reference to country file we need to check.
 * \return true if files has same versions.
 * \warning Function assumes that the file lock was already taken.
 */
bool CheckMwmConsistency(LocalCountryFile const & localFile)
{
  ModelReaderPtr r1 = FilesContainerR(localFile.GetPath(MapOptions::CarRouting))
      .GetReader(VERSION_FILE_TAG);
  ReaderSrc src1(r1.GetPtr());
  ModelReaderPtr r2 = FilesContainerR(localFile.GetPath(MapOptions::Map))
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

RoutingMapping::RoutingMapping(string const & countryFile, MwmSet & index)
    : m_mapCounter(0),
      m_facadeCounter(0),
      m_crossContextLoaded(0),
      m_countryFile(countryFile),
      m_error(IRouter::ResultCode::RouteFileNotExist),
      m_pIndex(&index)
{
  m_handle = index.GetMwmHandleByCountryFile(CountryFile(countryFile));
  if (!m_handle.IsAlive())
    return;

  LocalCountryFile const & localFile = m_handle.GetInfo()->GetLocalFile();
  if (!HasOptions(localFile.GetFiles(), MapOptions::MapWithCarRouting))
  {
    m_error = IRouter::ResultCode::RouteFileNotExist;
    m_handle = MwmSet::MwmHandle();
    return;
  }

  m_container.Open(localFile.GetPath(MapOptions::CarRouting));
  if (!CheckMwmConsistency(localFile))
  {
    m_error = IRouter::ResultCode::InconsistentMWMandRoute;
    m_container.Close();
    m_handle = MwmSet::MwmHandle();
    return;
  }

  m_mwmId = m_handle.GetId();
  m_error = IRouter::ResultCode::NoError;
}

void RoutingMapping::LoadFileIfNeeded()
{
  ASSERT(m_pIndex != nullptr, ());
  if (!m_handle.IsAlive() && m_mwmId.IsAlive())
  {
    m_handle = m_pIndex->GetMwmHandleById(m_mwmId);
    m_container.Open(m_mwmId.GetInfo()->GetLocalFile().GetPath(MapOptions::CarRouting));
  }
}

void RoutingMapping::FreeFileIfPossible()
{
  if (m_mapCounter == 0 && m_facadeCounter == 0 && m_handle.IsAlive())
  {
    m_handle = MwmSet::MwmHandle();
    m_container.Close();
  }
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
  LoadFileIfNeeded();
  if (!m_handle.IsAlive())
    return;
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
  FreeFileIfPossible();
}

void RoutingMapping::LoadFacade()
{
  if (!m_facadeCounter)
  {
    LoadFileIfNeeded();
    if (!m_handle.IsAlive())
      return;
    m_dataFacade.Load(m_container);
  }
  ++m_facadeCounter;
}

void RoutingMapping::FreeFacade()
{
  --m_facadeCounter;
  if (!m_facadeCounter)
  {
    FreeFileIfPossible();
    m_dataFacade.Clear();
  }
}

void RoutingMapping::LoadCrossContext()
{
  if (m_crossContextLoaded)
    return;

  LoadFileIfNeeded();

  if (!m_handle.IsAlive())
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
  FreeFileIfPossible();
}

TRoutingMappingPtr RoutingIndexManager::GetMappingByPoint(m2::PointD const & point)
{
  string const name = m_countryFileFn(point);
  if (name.empty())
    return TRoutingMappingPtr(new RoutingMapping());

  return GetMappingByName(name);
}

TRoutingMappingPtr RoutingIndexManager::GetMappingByName(string const & mapName)
{
  // Check if we have already loaded this file.
  auto mapIter = m_mapping.find(mapName);
  if (mapIter != m_mapping.end())
    return mapIter->second;

  // Or load and check file.
  TRoutingMappingPtr newMapping(new RoutingMapping(mapName, m_index));
  m_mapping[mapName] = newMapping;
  return newMapping;
}

TRoutingMappingPtr RoutingIndexManager::GetMappingById(Index::MwmId const & id)
{
  if (!id.IsAlive())
    return TRoutingMappingPtr(new RoutingMapping());
  return GetMappingByName(id.GetInfo()->GetCountryName());
}

}  // namespace routing
