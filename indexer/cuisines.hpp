#pragma once

#include <string>
#include <utility>
#include <vector>

namespace osm
{
using AllCuisines = std::vector<std::pair<std::string, std::string>>;

// This class IS thread-safe.
class Cuisines
{
public:
  static Cuisines const & Instance();

  std::string const & Translate(std::string const & singleCuisine) const;
  AllCuisines const & AllSupportedCuisines() const;

private:
  Cuisines();
  AllCuisines m_allCuisines;
};
}  // namespace osm
