#include "generator/osm_decl.hpp"

#include "indexer/classificator.hpp"

#include "base/macros.hpp"

#include "std/target_os.hpp"


string RelationElement::GetType() const
{
  map<string, string>::const_iterator i = tags.find("type");
  return ((i != tags.end()) ? i->second : string());
}

namespace
{
  bool FindRoleImpl(vector<pair<uint64_t, string> > const & cnt,
                    uint64_t id, string & role)
  {
    for (size_t i = 0; i < cnt.size(); ++i)
      if (cnt[i].first == id)
      {
        role = cnt[i].second;
        return true;
      }
    return false;
  }
}

bool RelationElement::FindWay(uint64_t id, string & role) const
{
  return FindRoleImpl(ways, id, role);
}

bool RelationElement::FindNode(uint64_t id, string & role) const
{
  return FindRoleImpl(nodes, id, role);
}
