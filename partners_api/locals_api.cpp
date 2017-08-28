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

namespace
{
using namespace locals;

bool CheckJsonArray(json_t const * data)
{
  if (data == nullptr)
    return false;
  return json_is_array(data) && json_array_size(data) > 0;
}

void ParseError(std::string const & src, ErrorCode & errorCode, std::string & message)
{
  message.clear();
  my::Json root(src.c_str());
  int code = 0;
  FromJSONObject(root.get(), "code", code);
  FromJSONObject(root.get(), "message", message);
  // TODO(darina): Process real error codes.
  errorCode = code > 0 ? ErrorCode::NoLocals : ErrorCode::RemoteError;
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
  if (!CheckJsonArray(results))
    return;
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
  if (request.RunHttpRequest() && request.ErrorCode() == 200)
  {
    result = request.ServerResponse();
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
  GetPlatform().RunOnNetworkThread([id, lat, lon, lang,
                                   resultsOnPage, pageNumber, successFn, errorFn]()
  {
    std::string result;
    if (!RawApi::Get(lat, lon, lang, resultsOnPage, pageNumber, result))
    {
      try
      {
        ErrorCode errorCode;
        std::string errorMessage;
        ParseError(result, errorCode, errorMessage);
        LOG(LWARNING, ("Locals request failed:", errorCode, errorMessage));
        return errorFn(id, errorCode, errorMessage);
      }
      catch (my::Json::Exception const & e)
      {
        LOG(LWARNING, ("Unknown error:", e.Msg()));
        return errorFn(id, ErrorCode::UnknownError, "Unknown error: " + e.Msg());
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
      LOG(LWARNING, ("Locals response parsing failed:", e.Msg()));
      errorFn(id, ErrorCode::UnknownError, "Response parsing failed: " + e.Msg());
    }

    successFn(id, locals, pageNumber, resultsOnPage, hasPreviousPage, hasNextPage);
  });
  return id;
}

std::string DebugPrint(ErrorCode code)
{
  switch (code)
  {
  case ErrorCode::NoLocals: return "NoLocals";
  case ErrorCode::RemoteError: return "RemoteError";
  case ErrorCode::UnknownError: return "UnknownError";
  }
  return "Unknown error code";
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
