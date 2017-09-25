#pragma once

#include "platform/platform.hpp"
#include "platform/safe_callback.hpp"

#include <string>

#define STAGE_LOCALS_SERVER

namespace locals
{
class RawApi
{
public:
  static bool Get(double lat, double lon, std::string const & lang,
                  size_t resultsOnPage, size_t pageNumber, std::string & result);
};

struct LocalExpert
{
  size_t m_id;
  std::string m_name;
  std::string m_country;
  std::string m_city;
  double m_rating;
  size_t m_reviewCount;
  double m_pricePerHour;
  std::string m_currency;
  std::string m_motto;
  std::string m_aboutExpert;
  std::string m_offerDescription;
  std::string m_pageUrl;
  std::string m_photoUrl;
};

using LocalsSuccessCallback = platform::SafeCallback<void(uint64_t id, std::vector<LocalExpert> const & locals,
                                                          size_t pageNumber, size_t countPerPage,
                                                          bool hasPreviousPage, bool hasNextPage)>;
using LocalsErrorCallback = platform::SafeCallback<void(uint64_t id, int errorCode,
                                                        std::string const & errorMessage)>;

class Api
{
public:
  static int constexpr kUnknownErrorCode = 0;
  static std::string GetLocalsPageUrl();
  uint64_t GetLocals(double lat, double lon, std::string const & lang,
                     size_t resultsOnPage, size_t pageNumber,
                     LocalsSuccessCallback const & successFn,
                     LocalsErrorCallback const & errorFn);
private:
  // Id for currently processed request.
  uint64_t m_requestId = 0;
};

std::string DebugPrint(LocalExpert const & localExpert);
}  // namespace locals
