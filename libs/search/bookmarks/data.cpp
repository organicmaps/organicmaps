#include "search/bookmarks/data.hpp"

#include <sstream>

using namespace std;

namespace search
{
namespace bookmarks
{
string DebugPrint(Data const & data)
{
  ostringstream os;
  os << "Data [";
  os << "names: " << ::DebugPrint(data.GetNames()) << ", ";
  os << "description: " << data.GetDescription() << "]";
  return os.str();
}
}  // namespace bookmarks
}  // namespace search
