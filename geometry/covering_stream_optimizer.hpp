#pragma once
#include "covering.hpp"
#include "../base/assert.hpp"
#include "../base/logging.hpp"
#include "../std/algorithm.hpp"
#include "../std/deque.hpp"
#include "../std/iterator.hpp"
#include "../std/map.hpp"
#include "../std/utility.hpp"

namespace covering
{

template <class CellIdT, typename ValueT, typename SinkT>
class CoveringStreamOptimizer
{
public:
  typedef CellIdT CellId;

  CoveringStreamOptimizer(SinkT & sink, uint32_t windowSize, uint32_t maxDuplicates)
    : m_Sink(sink), m_WindowSize(windowSize), m_MaxDuplicates(maxDuplicates),
    m_LastPopped(0), m_LastPushed(0)
  {
    CHECK_GREATER_OR_EQUAL(windowSize, maxDuplicates, ());
  }

  ~CoveringStreamOptimizer()
  {
    Flush();
  }

  void Add(int64_t id, ValueT value)
  {
    CHECK_LESS_OR_EQUAL(m_LastPushed, id, (value));
    m_LastPushed = id;
    m_Buffer.push_back(make_pair(id, value));
    if (++m_ValueCounts[value] >= m_MaxDuplicates && m_Buffer.size() >= m_WindowSize)
      ProcessWindow(value);
    while (m_Buffer.size() >= m_WindowSize)
      PopFront();
  }

  void Flush()
  {
    while (!m_Buffer.empty())
      PopFront();
  }

  static void Optimize(vector<int64_t> & ids, int64_t minId)
  {
    CHECK_GREATER(ids.size(), 2, (minId, ids));
    CHECK(IsSortedAndUnique(ids.begin(), ids.end()), (minId, ids));
    CHECK_GREATER_OR_EQUAL(ids[0], minId, (minId, ids));

    vector<CellId> cells(ids.size(), CellId(""));
    for (size_t i = 0; i < ids.size(); ++i)
      cells[i] = CellId::FromInt64(ids[i]);
    covering::Covering<CellId> covering(cells, minId);
    covering.Simplify(minId);

    vector<int64_t> res;
    covering.OutputToVector(res);
    sort(res.begin(), res.end());

    CHECK(IsSortedAndUnique(res.begin(), res.end()), (minId, ids, res));
    CHECK_GREATER_OR_EQUAL(res[0], minId, (minId, ids, res));

    ids.swap(res);
  }

private:
  void PopFront()
  {
    CHECK_LESS_OR_EQUAL(m_LastPopped, m_Buffer.front().first, ());
    m_LastPopped = m_Buffer.front().first;
    m_Sink(m_Buffer.front().first, m_Buffer.front().second);
    typename map<ValueT, uint32_t>::iterator it = m_ValueCounts.find(m_Buffer.front().second);
    CHECK(it != m_ValueCounts.end(), (m_Buffer.front().second))
    CHECK_GREATER(it->second, 0, (m_Buffer.front().second));
    if (--(it->second) == 0)
      m_ValueCounts.erase(it);
    m_Buffer.pop_front();
  }

  void ProcessWindow(ValueT value)
  {
    CHECK(IsSortedAndUnique(m_Buffer.begin(), m_Buffer.end()), ());
    vector<pair<int64_t, ValueT> > others;
    vector<int64_t> ids;
    for (typename BufferType::const_iterator it = m_Buffer.begin(); it != m_Buffer.end(); ++it)
    {
      if (it->second == value)
        ids.push_back(it->first);
      else
        others.push_back(*it);
    }
    CHECK_GREATER_OR_EQUAL(ids.size(), m_MaxDuplicates, ());
    Optimize(ids, m_LastPopped);
    CHECK(IsSortedAndUnique(ids.begin(), ids.end()), (ids));
    vector<pair<int64_t, ValueT> > optimized(ids.size(), make_pair(0LL, value));
    for (size_t i = 0; i < ids.size(); ++i)
      optimized[i].first = ids[i];
    CHECK(IsSortedAndUnique(others.begin(), others.end()), (others));
    CHECK(IsSortedAndUnique(optimized.begin(), optimized.end()), (optimized));
    m_Buffer.clear();
    set_union(others.begin(), others.end(),
              optimized.begin(), optimized.end(),
              back_inserter(m_Buffer));
    m_ValueCounts[value] = optimized.size();
  }

  SinkT & m_Sink;
  uint32_t const m_WindowSize;
  uint32_t const m_MaxDuplicates;
  typedef deque<pair<int64_t, ValueT> > BufferType;
  BufferType m_Buffer;
  map<ValueT, uint32_t> m_ValueCounts;
  int64_t m_LastPopped, m_LastPushed;
};

}
