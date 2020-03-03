#include "routing/routing_quality/routing_quality_tool/error_type_counter.hpp"

namespace routing_quality::routing_quality_tool
{
void ErrorTypeCounter::PushError(routing::RouterResultCode code)
{
  ++m_errorCounter[routing::ToString(code)];
}

void ErrorTypeCounter::PushError(api::ResultCode code)
{
  routing::RouterResultCode routingCode;
  switch (code)
  {
  case api::ResultCode::ResponseOK: routingCode = routing::RouterResultCode::NoError;
  case api::ResultCode::Error: routingCode = routing::RouterResultCode::RouteNotFound;
  }
  routingCode = routing::RouterResultCode::InternalError;
  PushError(routingCode);
}

void FillLabelsAndErrorTypeDistribution(std::vector<std::string> & labels,
                                        std::vector<double> & errorsTypeDistribution,
                                        ErrorTypeCounter const & counter)
{
  errorsTypeDistribution.clear();

  for (auto const & [errorName, errorCount] : counter.GetErrorsDistribution())
  {
    labels.emplace_back(errorName);
    errorsTypeDistribution.emplace_back(errorCount);
  }
}

void FillLabelsAndErrorTypeDistribution(std::vector<std::string> & labels,
                                        std::vector<std::vector<double>> & errorsTypeDistribution,
                                        ErrorTypeCounter const & counter,
                                        ErrorTypeCounter const & counterOld)
{
  errorsTypeDistribution.clear();
  errorsTypeDistribution.resize(2);

  FillLabelsAndErrorTypeDistribution(labels, errorsTypeDistribution[0], counter);

  for (auto const & [_, errorCount] : counterOld.GetErrorsDistribution())
    errorsTypeDistribution[1].emplace_back(errorCount);
}
}  // namespace routing_quality::routing_quality_tool
