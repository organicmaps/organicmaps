#include "pugixml.hpp"

#include <sstream>
#include <string>
#include <utility>

namespace pugi
{
template <typename... Args>
inline std::string XMLToString(pugi::xml_node const & n, Args &&... args)
{
  ostringstream sstr;
  n.print(sstr, std::forward<Args>(args)...);
  return sstr.str();
}

inline std::string DebugPrint(pugi::xml_node const & n)
{
  return XMLToString(n);
}

inline std::string DebugPrint(pugi::xml_document const & n)
{
  return DebugPrint(dynamic_cast<pugi::xml_node const &>(n));
}
}  // namespace pugi
