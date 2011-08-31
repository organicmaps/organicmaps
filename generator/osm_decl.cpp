#include "osm_decl.hpp"

#include "../indexer/classificator.hpp"

#include "../base/macros.hpp"

#include "../std/target_os.hpp"


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
