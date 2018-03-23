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
  os << "name: " << data.m_name << ", ";
  os << "description: " << data.m_description << "]";
  return os.str();
}
}  // namespace bookmarks
}  // namespace search
