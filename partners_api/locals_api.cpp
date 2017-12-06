#include "partners_api/locals_api.hpp"

#include "platform/http_client.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/url_encode.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

#include "3party/jansson/myjansson.hpp"

#include "private.h"

#define LOCALS_MOCKING_ENABLED

namespace
{
using namespace locals;

#ifdef LOCALS_MOCKING_ENABLED
std::string const kTempImageLink = "https://cdn.pixabay.com/photo/2017/08/01/08/29/people-2563491_1280.jpg";
std::string const kTempImageLink2 = "https://cdn.pixabay.com/photo/2017/06/26/02/47/people-2442565_1280.jpg";
std::vector<LocalExpert> const kLocalsMocking =
{
  {1, "Angelina", "Russian Federation", "Moscow", 10.0, 2, 100.0, "USD", "", "", "", "", kTempImageLink},
  {2, "Veronika", "Russian Federation", "Moscow", 8.0, 3, 89.99, "USD", "", "", "", "", ""},
  {3, "Snejana", "Russian Federation", "Moscow", 7.5, 4, 200.0, "USD", "", "", "", "", ""},
  {4, "Ella Petrovna", "Russian Federation", "Saint-Petersburg", 10.0, 1500, 0.0, "", "", "", "", "", ""},
  {5, "Boris", "Russian Federation", "Chelyabinsk", 9.0, 7, 1000.0, "RUB", "", "", "", "", kTempImageLink2}
};
#endif

void ParseError(std::string const & src, int & errorCode, std::string & message)
{
  message.clear();
  errorCode = 0;
  my::Json root(src.c_str());
  FromJSONObject(root.get(), "code", errorCode);
  FromJSONObject(root.get(), "message", message);
}

void ParseLocals(std::string const & src, std::vector<LocalExpert> & locals,
                 bool & hasPrevious, bool & hasNext)
{
  locals.clear();
  my::Json root(src.c_str());
  auto previousField = my::GetJSONOptionalField(root.get(), "previous");
  auto nextField = my::GetJSONOptionalField(root.get(), "next");
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

    // Rescale rating to [0.0; 10.0] range.
    local.m_rating *= 2;

    locals.push_back(move(local));
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
#ifndef LOCALS_MOCKING_ENABLED
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
      catch (my::Json::Exception const & e)
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
    catch (my::Json::Exception const & e)
    {
      LOG(LWARNING, ("Locals response parsing failed:", e.Msg(), ", response:", result));
      errorFn(id, kUnknownErrorCode, "Response parsing failed: " + e.Msg());
    }
#else
    std::vector<LocalExpert> locals = kLocalsMocking;
    bool hasPreviousPage = false;
    bool hasNextPage = false;
#endif

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
