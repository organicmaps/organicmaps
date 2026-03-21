#include "search/bookmarks/data.hpp"

#include <sstream>

namespace search
{
namespace bookmarks
{
std::string DebugPrint(Data const & data)
{
  std::ostringstream os;
  os << "Data [";
  os << "names: " << ::DebugPrint(data.GetNames()) << ", ";
  os << "description: " << data.GetDescription() << "]";
  return os.str();
}
}  // namespace bookmarks
}  // namespace search
