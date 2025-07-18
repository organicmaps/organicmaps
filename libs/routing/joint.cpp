#include "routing/joint.hpp"

#include <sstream>

namespace routing
{
// static
Joint::Id constexpr Joint::kInvalidId;

std::string DebugPrint(Joint const & joint)
{
  std::ostringstream oss;
  oss << "Joint [";
  for (size_t i = 0; i < joint.GetSize(); ++i)
  {
    if (i > 0)
      oss << ", ";
    oss << DebugPrint(joint.GetEntry(i));
  }
  oss << "]";
  return oss.str();
}
}  // namespace routing
