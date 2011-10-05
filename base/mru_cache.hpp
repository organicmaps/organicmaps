#pragma once

#include "../std/unordered_map.hpp"
#include "../std/list.hpp"
#include "assert.hpp"
#include "logging.hpp"

namespace my
{
  template <typename TValue>
  struct MRUCacheValueTraits
  {
    static void Evict(TValue &){}
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
      size_t m_lockCount;
      typename list_t::iterator m_it;
    };

    typedef map<KeyT, MapEntry> map_t;

    map_t m_map;
    list_t m_list;
    int m_curWeight;
    int m_maxWeight;

  public:
    explicit MRUCache(int maxWeight)
      : m_curWeight(0), m_maxWeight(maxWeight)
    {}

    set<KeyT> const Keys() const
    {
      set<KeyT> keys;
      for (typename map_t::const_iterator it = m_map.begin(); it != m_map.end(); ++it)
        keys.insert(it->first);
      return keys;
    }

    bool HasElem(KeyT const & key)
    {
      return m_map.find(key) != m_map.end();
    }

    void LockElem(KeyT const & key)
    {
      ASSERT(HasElem(key), ());
      m_map[key].m_lockCount += 1;
    }

    size_t LockCount(KeyT const & key)
    {
      ASSERT(HasElem(key), ());
      return m_map[key].m_lockCount;
    }

    void UnlockElem(KeyT const & key)
    {
      ASSERT(HasElem(key), ());
      ASSERT(m_map[key].m_lockCount > 0, ());
      m_map[key].m_lockCount -= 1;
    }

    ValueT const & Find(KeyT const & key, bool DoTouch = true)
    {
      typename map_t::iterator it = m_map.find(key);

      ASSERT(it != m_map.end(), ());

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

      ASSERT(it != m_map.end(), ());

      typename list_t::iterator listIt = it->second.m_it;
      KeyT k = *listIt;
      m_list.erase(listIt);
      m_list.push_front(k);
      it->second.m_it = m_list.begin();
    }

    void Remove(KeyT const & key)
    {
      typename map_t::iterator it = m_map.find(key);

      ASSERT(it->second.m_lockCount == 0, ("removing locked element"));

      if (it != m_map.end() && it->second.m_lockCount == 0)
      {
        m_curWeight -= it->second.m_weight;
        m_list.erase(it->second.m_it);
        ValueTraitsT::Evict(it->second.m_value);
//        LOG(LINFO, ("removing : ", m_curWeight));
        m_map.erase(it);
      }
    }

    void Add(KeyT const & key, ValueT const & val, size_t weight)
    {
      if (HasElem(key))
        Remove(key);

      if (!m_list.empty())
      {
        typename list<KeyT>::iterator it = (++m_list.rbegin()).base();

        bool needToFree = false;

        while (m_curWeight + weight > m_maxWeight)
        {
/*          if (!needToFree)
            LOG(LINFO, ("freeing elements from ", m_curWeight));*/
          needToFree = true;
          KeyT k = *it;

          /// erasing only unlocked elements
          if (m_map[k].m_lockCount == 0)
          {
            m_curWeight -= m_map[k].m_weight;
            ValueTraitsT::Evict(m_map[k].m_value);
//            LOG(LINFO, ("freeing lru elem : ", m_map[k].m_weight, m_curWeight));
            m_map.erase(k);

            typename list<KeyT>::iterator nextIt = it;
            if (nextIt != m_list.begin())
            {
              --nextIt;
              m_list.erase(it);
              it = nextIt;
            }
            else
            {
              m_list.erase(it);
              break;
            }
          }
          else
          {
            if (it == m_list.begin())
              break;

            --it;
          }
        }
      }

      ASSERT(m_curWeight + weight <= m_maxWeight, ());

      m_list.push_front(key);
      m_map[key].m_weight = weight;
      m_map[key].m_value = val;
      m_map[key].m_lockCount = 0;
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
