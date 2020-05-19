#include "map/guides_on_map_delegate.hpp"

#include "web_api/request_headers.hpp"

GuidesOnMapDelegate::GuidesOnMapDelegate(
    std::shared_ptr<CatalogHeadersProvider> const & headersProvider)
  : m_headersProvider(headersProvider)
{
}

platform::HttpClient::Headers GuidesOnMapDelegate::GetHeaders()
{
  if (!m_headersProvider)
    return {};

  auto const position = m_headersProvider->GetPositionHeader();
  if (!position)
    return {};

  auto headers = web_api::GetDefaultCatalogHeaders();
  headers.emplace(position->m_name, position->m_value);

  return headers;
}
