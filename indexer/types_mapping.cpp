#include "indexer/types_mapping.hpp"
#include "indexer/classificator.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

using namespace std;

void IndexAndTypeMapping::Clear()
{
  m_types.clear();
  m_map.clear();
}

void IndexAndTypeMapping::Load(istream & s)
{
  Classificator const & c = classif();

  string v;
  vector<string> path;

  uint32_t ind = 0;
  while (s.good())
  {
    v.clear();
    s >> v;

    if (!v.empty())
    {
      path.clear();

      // Types can be deprecated with replacement, for deprecated type we have replacing type
      // in types.txt. We should use only main type for type index to ensure stable indices.
      // Main type description starts with '*' in types.txt.
      auto const isMainTypeDescription = v[0] == '*';
      if (isMainTypeDescription)
      {
        v = v.substr(1);
        CHECK(!v.empty(), (ind));
      }
      strings::Tokenize(v, "|", base::MakeBackInsertFunctor(path));

      Add(ind++, c.GetTypeByPath(path), isMainTypeDescription);
    }
  }
}

void IndexAndTypeMapping::Add(uint32_t ind, uint32_t type, bool isMainTypeDescription)
{
  ASSERT_EQUAL ( ind, m_types.size(), () );

  m_types.push_back(type);
  if (isMainTypeDescription)
  {
    auto const res = m_map.insert(make_pair(type, ind));
    CHECK(res.second, ("Type can have only one main description.", ind, m_map[ind]));
  }
}

uint32_t IndexAndTypeMapping::GetIndex(uint32_t t) const
{
  Map::const_iterator i = m_map.find(t);
  CHECK ( i != m_map.end(), (t, classif().GetFullObjectName(t)) );
  return i->second;
}
