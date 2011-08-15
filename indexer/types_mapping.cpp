#include "types_mapping.hpp"
#include "classificator.hpp"

#include "../base/string_utils.hpp"
#include "../base/stl_add.hpp"


void BaseTypeMapper::Load(string const & buffer)
{
  istringstream ss(buffer);
  Classificator & c = classif();

  string v;
  vector<string> path;

  uint32_t ind = 0;
  while (true)
  {
    ss >> v;

    if (ss.eof())
      break;

    path.clear();
    strings::Tokenize(v, "|", MakeBackInsertFunctor(path));

    Add(ind++, c.GetTypeByPath(path));
  }
}

void Index2Type::Add(uint32_t ind, uint32_t type)
{
  ASSERT_EQUAL ( ind, m_types.size(), () );
  m_types.push_back(type);
}

void Type2Index::Add(uint32_t ind, uint32_t type)
{
  VERIFY ( m_map.insert(make_pair(type, ind)).second, (type, ind) );
}
