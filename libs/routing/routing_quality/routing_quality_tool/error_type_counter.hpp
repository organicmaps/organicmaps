#pragma once

#include "routing/routing_quality/api/api.hpp"

#include "routing/routing_callbacks.hpp"

#include <cstddef>
#include <map>
#include <string>

namespace routing_quality::routing_quality_tool
{
class ErrorTypeCounter
{
public:
  void PushError(routing::RouterResultCode code);
  void PushError(api::ResultCode code);
  std::map<std::string, size_t> const & GetErrorsDistribution() const { return m_errorCounter; }

private:
  // string representation of RouterResultCode to number of such codes.
  std::map<std::string, size_t> m_errorCounter;
};

void FillLabelsAndErrorTypeDistribution(std::vector<std::string> & labels, std::vector<double> & errorsTypeDistribution,
                                        ErrorTypeCounter const & counter);

void FillLabelsAndErrorTypeDistribution(std::vector<std::string> & labels,
                                        std::vector<std::vector<double>> & errorsTypeDistribution,
                                        ErrorTypeCounter const & counter, ErrorTypeCounter const & counterOld);
}  // namespace routing_quality::routing_quality_tool
