#include "generator/osm_element_helpers.hpp"

#include "base/logging.hpp"
#include "base/string_utils.hpp"

namespace generator
{
namespace osm_element
{
uint64_t GetPopulation(std::string const & str)
{
  std::string number;
  for (auto const c : str)
    if (isdigit(c))
      number += c;
    else if (c == '.' || c == ',' || c == ' ')
      continue;
    else
      break;

  if (number.empty())
    return 0;

  uint64_t result = 0;
  if (!strings::to_uint64(number, result))
    LOG(LWARNING, ("Failed to get population from", number, str));

  return result;
}

uint64_t GetPopulation(OsmElement const & elem)
{
  return GetPopulation(elem.GetTag("population"));
}

std::vector<uint64_t> GetPlaceNodeFromMembers(OsmElement const & elem)
{
  std::vector<uint64_t> res;
  int adminIdx = -1;
  for (auto const & member : elem.m_members)
  {
    if (member.m_type == OsmElement::EntityType::Node)
    {
      if (member.m_role == "admin_centre")
      {
        if (adminIdx == -1)
          adminIdx = res.size();
        res.push_back(member.m_ref);
      }
      else if (member.m_role == "label")
        res.push_back(member.m_ref);
    }
  }

  if (adminIdx > 0)
    std::swap(res[0], res[adminIdx]);
  return res;
}

uint8_t GetAdminLevel(OsmElement const & elem)
{
  auto const str = elem.GetTag("admin_level");
  if (!str.empty())
  {
    int res = 0;
    if (strings::to_int(str, res) && res > 0 && res < 13)
      return static_cast<uint8_t>(res);

    // There are too many custom admin_level value.
    // LOG(LWARNING, ("Failed to get admin_level from", str, elem.m_id));
  }
  return 0;
}

}  // namespace osm_element
}  // namespace generator
