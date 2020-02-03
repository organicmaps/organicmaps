#pragma once

#include <string>
#include <utility>
#include <vector>

namespace osm
{
using AllCuisines = std::vector<std::pair<std::string, std::string>>;

class Cuisines
{
public:
  static Cuisines & Instance();

  std::string Translate(std::string const & singleCuisine);
  AllCuisines AllSupportedCuisines();

private:
  Cuisines();
  AllCuisines m_allCuisines;
};
}  // namespace osm
