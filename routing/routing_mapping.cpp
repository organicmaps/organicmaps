#include "routing/routing_mapping.h"

#include "routing/cross_routing_context.hpp"
#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"

#include "base/logging.hpp"

#include "coding/reader_wrapper.hpp"

#include "indexer/mwm_version.hpp"

#include "platform/platform.hpp"

namespace routing
{
RoutingMapping::RoutingMapping(string const & fName, Index const * pIndex)
    : m_mapCounter(0),
      m_facadeCounter(0),
      m_crossContextLoaded(0),
      m_baseName(fName),
      m_isValid(true),
      m_error(IRouter::ResultCode::NoError)
{
  Platform & pl = GetPlatform();
  string const mwmName = m_baseName + DATA_FILE_EXTENSION;
  string const fPath = pl.WritablePathForFile(mwmName + ROUTING_FILE_EXTENSION);
  if (!pl.IsFileExistsByFullPath(fPath))
  {
    m_isValid = false;
    m_error = IRouter::ResultCode::RouteFileNotExist;
    return;
  }
  // Open new container and check that mwm and routing have equal timestamp.
  LOG(LDEBUG, ("Load routing index for file:", fPath));
  m_container.Open(fPath);
  {
    FileReader r1 = m_container.GetReader(VERSION_FILE_TAG);
    ReaderSrc src1(r1);
    ModelReaderPtr r2 = FilesContainerR(pl.GetReader(mwmName)).GetReader(VERSION_FILE_TAG);
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

  m_mwmId = pIndex->GetMwmIdByFileName(mwmName);
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

TRoutingMappingPtr RoutingIndexManager::GetMappingByPoint(m2::PointD const & point)
{
  return GetMappingByName(m_countryFn(point));
}

TRoutingMappingPtr RoutingIndexManager::GetMappingByName(string const & fName)
{
  // Check if we have already loaded this file
  auto mapIter = m_mapping.find(fName);
  if (mapIter != m_mapping.end())
    return mapIter->second;

  // Or load and check file
  TRoutingMappingPtr new_mapping = make_shared<RoutingMapping>(fName, m_index);
  m_mapping.insert(make_pair(fName, new_mapping));
  return new_mapping;
}

}  // namespace routing
