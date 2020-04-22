#pragma once

#include "map/catalog_headers_provider.hpp"

#include "partners_api/guides_on_map_api.hpp"

#include <memory>

class GuidesOnMapDelegate : public guides_on_map::Api::Delegate
{
public:
  GuidesOnMapDelegate(std::shared_ptr<CatalogHeadersProvider> const & headersProvider);

  platform::HttpClient::Headers GetHeaders() override;

private:
  std::shared_ptr<CatalogHeadersProvider> m_headersProvider;
};
