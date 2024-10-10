#pragma once

#include "platform/location.hpp"

#include <deque>
#include <limits>
#include <utility>
#include <vector>

class GpsTrackCollection final
{
public:
  static size_t const kInvalidId; // = numeric_limits<size_t>::max();

  using TItem = location::GpsInfo;

  /// Constructor
  GpsTrackCollection();

  /// Adds new point in the collection.
  /// @param item - item to be added.
  /// @returns the item unique identifier or kInvalidId if point has incorrect time.
  size_t Add(TItem const & item);

  /// Adds set of new points in the collection.
  /// @param items - set of items to be added.
  /// @returns range of identifiers of added items or pair(kInvalidId,kInvalidId) if nothing was added
  /// @note items which does not conform to timestamp sequence, is not added.
  std::pair<size_t, size_t> Add(std::vector<TItem> const & items);

  /// Removes all points from the collection.
  /// @param resetIds - if it is set to true, then new identifiers will start from 0,
  /// otherwise new identifiers will continue from returned value res.second + 1
  /// @return range of item identifiers, which were removed or
  /// pair(kInvalidId,kInvalidId) if nothing was removed
  std::pair<size_t, size_t> Clear(bool resetIds = true);

  /// Returns true if collection is empty, otherwise returns false.
  bool IsEmpty() const;

  /// Returns number of items in the collection
  size_t GetSize() const;

  /// Enumerates items in the collection.
  /// @param f - callable object, which is called with params - item and item id,
  /// if f returns false, then enumeration is stopped.
  /// @param pos - position index to start enumeration
  template <typename F>
  void ForEach(F && f, size_t pos = 0) const
  {
    if (pos >= m_items.size())
      return;
    auto i = m_items.cbegin() + pos, iend = m_items.cend();
    size_t id = m_lastId - m_items.size() + pos;
    for (; i != iend; ++i, ++id)
    {
      TItem const & item = *i;
      size_t const itemId = id;
      if (!f(item, itemId))
        break;
    }
  }

private:
  // Removes items in range [m_items.begin(), i) and returnd
  // range of identifiers of removed items
  std::pair<size_t, size_t> RemoveUntil(std::deque<TItem>::iterator i);

  // Removes items extra by timestamp
  std::pair<size_t, size_t> RemoveExtraItems();

  std::deque<TItem> m_items;  // asc. sorted by timestamp

  size_t m_lastId;
};
