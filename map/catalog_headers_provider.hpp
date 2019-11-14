#pragma once

#include "map/position_provider.hpp"

#include "storage/storage.hpp"

#include "web_api/request_headers.hpp"

#include "platform/http_client.hpp"

class CatalogHeadersProvider
{
public:
  CatalogHeadersProvider(PositionProvider const & positionProvider,
                         storage::Storage const & storage);

  platform::HttpClient::Headers GetHeaders();

private:
  PositionProvider const & m_positionProvider;
  storage::Storage const & m_storage;
};
