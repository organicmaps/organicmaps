#include "indexer/types_mapping.hpp"
#include "indexer/classificator.hpp"

#include "base/string_utils.hpp"
#include "base/stl_add.hpp"

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
      strings::Tokenize(v, "|", MakeBackInsertFunctor(path));

      Add(ind++, c.GetTypeByPath(path));
    }
  }
}

void IndexAndTypeMapping::Add(uint32_t ind, uint32_t type)
{
  ASSERT_EQUAL ( ind, m_types.size(), () );

  m_types.push_back(type);
  m_map.insert(make_pair(type, ind));
}

uint32_t IndexAndTypeMapping::GetIndex(uint32_t t) const
{
  Map::const_iterator i = m_map.find(t);
  CHECK ( i != m_map.end(), (t, classif().GetFullObjectName(t)) );
  return i->second;
}
