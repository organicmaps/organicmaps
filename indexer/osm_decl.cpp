#include "osm_decl.hpp"
#include "classificator.hpp"

#include "../base/macros.hpp"

#include "../std/target_os.hpp"
#include "../std/iostream.hpp"

#include "../base/start_mem_debug.hpp"


namespace feature
{
  char const * arrUnite[1][2] = { { "natural", "coastline" } };

  bool NeedUnite(string const & k, string const & v)
  {
    for (size_t i = 0; i < ARRAY_SIZE(arrUnite); ++i)
      if (k == arrUnite[i][0] && v == arrUnite[i][1])
        return true;

    return false;
  }

  bool NeedUnite(uint32_t type)
  {
    static uint32_t arrTypes[1] = { 0 };

    if (arrTypes[0] == 0)
    {
      // initialize static array
      for (size_t i = 0; i < ARRAY_SIZE(arrUnite); ++i)
      {
        size_t const count = ARRAY_SIZE(arrUnite[i]);
        vector<string> path(count);
        for (size_t j = 0; j < count; ++j)
          path[j] = arrUnite[i][j];

        arrTypes[i] = classif().GetTypeByPath(path);
      }
    }

    for (size_t i = 0; i < ARRAY_SIZE(arrTypes); ++i)
      if (arrTypes[i] == type)
        return true;

    return false;
  }
}

void progress_policy::Begin(string const & /*name*/, size_t factor)
{
  m_count = 0;
  m_factor = factor;
//#ifndef OMIM_OS_BADA
//  cout << "Progress of " << name << " started" << endl;
//#endif
}

void progress_policy::Inc(size_t i /* = 1 */)
{
  m_count += i;
//#ifndef OMIM_OS_BADA
//  if (m_count % m_factor == 0)
//    cout << '.';
//#endif
}

void progress_policy::End()
{
}

string RelationElement::GetType() const
{
  map<string, string>::const_iterator i = tags.find("type");
  return ((i != tags.end()) ? i->second : string());
}

bool RelationElement::FindWay(uint64_t id, string & role) const
{
  for (size_t i = 0; i < ways.size(); ++i)
    if (ways[i].first == id)
    {
      role = ways[i].second;
      return true;
    }
    return false;
}
