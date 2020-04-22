#include "map/guides_on_map_delegate.hpp"

GuidesOnMapDelegate::GuidesOnMapDelegate(
    std::shared_ptr<CatalogHeadersProvider> const & headersProvider)
  : m_headersProvider(headersProvider)
{
}

platform::HttpClient::Headers GuidesOnMapDelegate::GetHeaders()
{
  if (!m_headersProvider)
    return {};

  return m_headersProvider->GetHeaders();
}
