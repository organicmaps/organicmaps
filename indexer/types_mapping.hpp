#pragma once
#include "../base/assert.hpp"

#include "../std/vector.hpp"
#include "../std/map.hpp"
#include "../std/fstream.hpp"


class BaseTypeMapper
{
protected:
  virtual void Add(uint32_t ind, uint32_t type) = 0;

public:
  void Load(istream & s);
};

class Index2Type : public BaseTypeMapper
{
  vector<uint32_t> m_types;

protected:
  virtual void Add(uint32_t ind, uint32_t type);

public:
  uint32_t GetType(uint32_t ind) const
  {
    ASSERT_LESS ( ind, m_types.size(), () );
    return m_types[ind];
  }
};

class Type2Index : public BaseTypeMapper
{
  typedef map<uint32_t, uint32_t> MapT;
  MapT m_map;

protected:
  virtual void Add(uint32_t ind, uint32_t type);

public:
  uint32_t GetIndex(uint32_t t) const
  {
    MapT::const_iterator i = m_map.find(t);
    ASSERT ( i != m_map.end(), () );
    return i->second;
  }
};

void PrintTypesDefault(string const & fPath);
