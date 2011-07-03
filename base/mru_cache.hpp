#pragma once

#include "../std/unordered_map.hpp"
#include "assert.hpp"

namespace my
{
  template <typename TValue>
  struct MRUCacheValueTraits
  {
    static void Evict(TValue & val){}
  };

  template <typename KeyT, typename ValueT, typename ValueTraitsT = MRUCacheValueTraits<ValueT> >
  class MRUCache
  {
  public:
    typedef MRUCache<KeyT, ValueT> this_type;
  private:
    MRUCache(this_type const & c);
    this_type & operator= (this_type const &);

    typedef list<KeyT> list_t;

    struct MapEntry
    {
      ValueT m_value;
      size_t m_weight;
      typename list_t::iterator m_it;
    };

    typedef unordered_map<KeyT, MapEntry> map_t;

    map_t m_map;
    list_t m_list;
    int m_curWeight;
    int m_maxWeight;

  public:
    explicit MRUCache(size_t maxWeight)
      : m_curWeight(0), m_maxWeight(maxWeight)
    {}

    bool HasElem(KeyT const & key)
    {
      return m_map.find(key) != m_map.end();
    }

    ValueT const & Find(KeyT const & key, bool DoTouch = true)
    {
      typename map_t::iterator it = m_map.find(key);
      CHECK(it != m_map.end(), ());
      if (DoTouch)
      {
        typename list_t::iterator listIt = it->second.m_it;
        KeyT k = *listIt;
        m_list.erase(listIt);
        m_list.push_front(k);
        it->second.m_it = m_list.begin();
      }
      return it->second.m_value;
    }

    void Touch(KeyT const & key)
    {
      if (!HasElem(key))
        return;
      typename map_t::iterator it = m_map.find(key);
      CHECK(it != m_map.end(), ());

      typename list_t::iterator listIt = it->second.m_it;
      KeyT k = *listIt;
      m_list.erase(listIt);
      m_list.push_front(k);
      it->second.m_it = m_list.begin();
    }

    void Remove(KeyT const & key)
    {
      typename map_t::iterator it = m_map.find(key);

      if (it != m_map.end())
      {
        m_curWeight -= it->second.m_weight;
        m_list.erase(it->second.m_it);
        ValueTraitsT::Evict(it->second.m_value);
        m_map.erase(it);
      }
    }

    void Add(KeyT const & key, ValueT const & val, size_t weight)
    {
      if (HasElem(key))
        Remove(key);

      while (m_curWeight + weight > m_maxWeight)
      {
        if (m_list.empty())
          return;
        KeyT k = m_list.back();
        m_list.pop_back();
        m_curWeight -= m_map[k].m_weight;
        ValueTraitsT::Evict(m_map[k].m_value);
        m_map.erase(k);
      }

      m_list.push_front(key);
      m_map[key].m_weight = weight;
      m_map[key].m_value = val;
      m_map[key].m_it = m_list.begin();
      m_curWeight += weight;
    }

    void Clear()
    {
      for (typename map_t::iterator it = m_map.begin(); it != m_map.end(); ++it)
        ValueTraitsT::Evict(it->second);

      m_map.clear();
      m_list.clear();
      m_curWeight = 0;
    }
  };
}
