#include "partners_api/locals_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/string_utf8_multilang.hpp"
#include "coding/url_encode.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "defines.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

namespace
{
using namespace locals;

void ParseError(std::string const & src, int & errorCode, std::string & message)
{
  message.clear();
  errorCode = 0;
  base::Json root(src.c_str());
  FromJSONObject(root.get(), "code", errorCode);
  FromJSONObject(root.get(), "message", message);
}

void ParseLocals(std::string const & src, std::vector<LocalExpert> & locals,
                 bool & hasPrevious, bool & hasNext)
{
  locals.clear();
  base::Json root(src.c_str());
  auto previousField = base::GetJSONOptionalField(root.get(), "previous");
  auto nextField = base::GetJSONOptionalField(root.get(), "next");
  hasPrevious = json_is_number(previousField);
  hasNext = json_is_number(nextField);
  auto const results = json_object_get(root.get(), "results");
  auto const dataSize = json_array_size(results);
  for (size_t i = 0; i < dataSize; ++i)
  {
    LocalExpert local;
    auto const item = json_array_get(results, i);
    FromJSONObject(item, "id", local.m_id);
    FromJSONObject(item, "motto", local.m_motto);
    FromJSONObject(item, "link", local.m_pageUrl);
    FromJSONObject(item, "photo", local.m_photoUrl);
    FromJSONObject(item, "name", local.m_name);
    FromJSONObject(item, "country", local.m_country);
    FromJSONObject(item, "city", local.m_city);
    FromJSONObject(item, "rating", local.m_rating);
    FromJSONObject(item, "reviews", local.m_reviewCount);
    FromJSONObject(item, "price_per_hour", local.m_pricePerHour);
    FromJSONObject(item, "currency", local.m_currency);
    FromJSONObject(item, "about_me", local.m_aboutExpert);
    FromJSONObject(item, "i_will_show_you", local.m_offerDescription);

    // Rescale rating to (0.0; 10.0] range. Rating 0.0 is invalid.
    if (local.m_rating != kInvalidRatingValue)
      local.m_rating *= 2.0;

    locals.push_back(std::move(local));
  }
}
}  // namespace

namespace locals
{
bool RawApi::Get(double lat, double lon, std::string const & lang, size_t resultsOnPage, size_t pageNumber,
                 std::string & result)
{
  result.clear();
  std::ostringstream ostream;
  ostream << LOCALS_API_URL << "/search?api_key=" << LOCALS_API_KEY
          << "&lat=" << lat << "&lon=" << lon
          << "&limit=" << resultsOnPage << "&page=" << pageNumber;
  if (!lang.empty())
    ostream << "&lang=" << lang;

  platform::HttpClient request(ostream.str());
  request.SetHttpMethod("GET");
  request.SetRawHeader("User-Agent", GetPlatform().GetAppUserAgent());
  if (request.RunHttpRequest())
  {
    result = request.ServerResponse();
    if (request.ErrorCode() == 200)
      return true;
  }
  return false;
}

std::string Api::GetLocalsPageUrl()
{
  return LOCALS_PAGE_URL;
}

uint64_t Api::GetLocals(double lat, double lon, std::string const & lang,
                        size_t resultsOnPage, size_t pageNumber,
                        LocalsSuccessCallback const & successFn,
                        LocalsErrorCallback const & errorFn)
{
  uint64_t id = ++m_requestId;
  GetPlatform().RunTask(Platform::Thread::Network,
                        [id, lat, lon, lang, resultsOnPage, pageNumber, successFn, errorFn]()
  {
    std::string result;
    if (!RawApi::Get(lat, lon, lang, resultsOnPage, pageNumber, result))
    {
      try
      {
        int errorCode;
        std::string errorMessage;
        ParseError(result, errorCode, errorMessage);
        LOG(LWARNING, ("Locals request failed:", errorCode, errorMessage));
        return errorFn(id, errorCode, errorMessage);
      }
      catch (base::Json::Exception const & e)
      {
        LOG(LWARNING, ("Unknown error:", e.Msg(), ", response:", result));
        return errorFn(id, kUnknownErrorCode, "Unknown error: " + e.Msg());
      }
    }

    std::vector<LocalExpert> locals;
    bool hasPreviousPage = false;
    bool hasNextPage = false;
    try
    {
      ParseLocals(result, locals, hasPreviousPage, hasNextPage);
    }
    catch (base::Json::Exception const & e)
    {
      LOG(LWARNING, ("Locals response parsing failed:", e.Msg(), ", response:", result));
      errorFn(id, kUnknownErrorCode, "Response parsing failed: " + e.Msg());
    }

    successFn(id, locals, pageNumber, resultsOnPage, hasPreviousPage, hasNextPage);
  });
  return id;
}

std::string DebugPrint(LocalExpert const & localExpert)
{
  std::ostringstream out;
  out << "id: " << localExpert.m_id << std::endl
      << "name: " << localExpert.m_name << std::endl
      << "about: " << localExpert.m_aboutExpert << std::endl
      << "city: " << localExpert.m_city << std::endl
      << "country: " << localExpert.m_country << std::endl
      << "currency: " << localExpert.m_currency << std::endl
      << "pageUrl: " << localExpert.m_pageUrl << std::endl
      << "photoUrl: " << localExpert.m_photoUrl << std::endl
      << "description: " << localExpert.m_offerDescription << std::endl
      << "motto: " << localExpert.m_motto << std::endl;
  return out.str();
}
}  // namespace locals
