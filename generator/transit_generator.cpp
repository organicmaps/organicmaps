#include "generator/transit_generator.hpp"

#include "base/logging.hpp"
#include "base/macros.hpp"

namespace routing
{
namespace transit
{
void BuildTransit(std::string const & mwmPath, std::string const & transitDir)
{
  LOG(LINFO, ("mwm path:", mwmPath, ", directory with transit:", transitDir));
  NOTIMPLEMENTED();
}
}  // namespace transit
}  // namespace routing