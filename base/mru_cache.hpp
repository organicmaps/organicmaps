#pragma once

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "std/list.hpp"
#include "std/map.hpp"


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
    typedef set<KeyT> key_set_t;

    key_set_t m_keys;
    map_t m_map;
    list_t m_list;
    int m_curWeight;
    int m_maxWeight;

  public:

    MRUCache()
      : m_curWeight(0), m_maxWeight(0)
    {}

    explicit MRUCache(int maxWeight)
      : m_curWeight(0), m_maxWeight(maxWeight)
    {}

    set<KeyT> const & Keys() const
    {
      return m_keys;
    }

    bool HasRoom(int weight)
    {
      return m_curWeight + weight <= m_maxWeight;
    }

    /// how many elements in unlocked state do we have in cache
    int UnlockedWeight() const
    {
      int unlockedWeight = 0;

      for (typename map_t::const_iterator it = m_map.begin(); it != m_map.end(); ++it)
      {
        if (it->second.m_lockCount == 0)
          unlockedWeight += it->second.m_weight;
      }

      return unlockedWeight;
    }

    /// how many elements in locked state do we have in cache
    int LockedWeight() const
    {
      int lockedWeight = 0;

      for (typename map_t::const_iterator it = m_map.begin(); it != m_map.end(); ++it)
      {
        if (it->second.m_lockCount != 0)
          lockedWeight += it->second.m_weight;
      }

      return lockedWeight;
    }

    /// how much elements we can fit in this cache, considering unlocked
    /// elements, that could be popped out on request
    int CanFit() const
    {
      return m_maxWeight - LockedWeight();
    }

    int MaxWeight() const
    {
      return m_maxWeight;
    }

    void Resize(int maxWeight)
    {
      m_maxWeight = maxWeight;
      // in case of making cache smaller this
      // function pops out some unlocked elements
      FreeRoom(0);
    }

    void FreeRoom(int weight)
    {
      if (HasRoom(weight))
        return;

      if (!m_list.empty())
      {
        typename list<KeyT>::iterator it = (++m_list.rbegin()).base();

        while (m_curWeight + weight > m_maxWeight)
        {
          KeyT k = *it;

          MapEntry & e = m_map[k];

          /// erasing only unlocked elements
          if (e.m_lockCount == 0)
          {
            m_curWeight -= e.m_weight;
            ValueTraitsT::Evict(e.m_value);
            m_map.erase(k);
            m_keys.erase(k);

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
    }

    bool HasElem(KeyT const & key)
    {
      return m_keys.find(key) != m_keys.end();
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
      ASSERT(m_map[key].m_lockCount > 0, (m_map[key].m_lockCount));
      m_map[key].m_lockCount -= 1;
    }

    ValueT const & Find(KeyT const & key, bool DoTouch = true)
    {
      typename map_t::iterator it = m_map.find(key);

      ASSERT(it != m_map.end(), ());

      if (DoTouch)
        Touch(key);

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
        m_map.erase(it);
        m_keys.erase(key);
      }
    }

    void Add(KeyT const & key, ValueT const & val, size_t weight)
    {
      if (HasElem(key))
        Remove(key);

      FreeRoom(weight);

      m_list.push_front(key);
      m_map[key].m_weight = weight;
      m_map[key].m_value = val;
      m_map[key].m_lockCount = 0;
      m_map[key].m_it = m_list.begin();
      m_keys.insert(key);

      m_curWeight += weight;
    }

    void Clear()
    {
      for (typename map_t::iterator it = m_map.begin(); it != m_map.end(); ++it)
        ValueTraitsT::Evict(it->second);

      m_map.clear();
      m_keys.clear();
      m_list.clear();
      m_curWeight = 0;
    }
  };
}
