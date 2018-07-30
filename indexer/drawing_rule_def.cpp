#include "indexer/drawing_rule_def.hpp"

#include <algorithm>
#include <iterator> 

using namespace std;

namespace drule
{
namespace
{
struct less_key
{
  bool operator() (drule::Key const & r1, drule::Key const & r2) const
  {
    // assume that unique algo leaves the first element (with max priority), others - go away
    if (r1.m_type == r2.m_type)
      return (r1.m_priority > r2.m_priority);
    else
      return (r1.m_type < r2.m_type);
   }
};

struct equal_key
{
  bool operator() (drule::Key const & r1, drule::Key const & r2) const
  {
    // many line rules - is ok, other rules - one is enough
    if (r1.m_type == drule::line)
      return (r1 == r2);
    else
      return (r1.m_type == r2.m_type);
  }
};
}  // namespace

void MakeUnique(KeysT & keys)
{
  sort(keys.begin(), keys.end(), less_key());
  keys.resize(distance(keys.begin(), unique(keys.begin(), keys.end(), equal_key())));
}
}
