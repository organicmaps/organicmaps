#pragma once

#include "base/assert.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/writer.hpp"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

struct WayElement
{
  uint64_t m_wayOsmId;
  std::vector<uint64_t> m_nodes;

  explicit WayElement(uint64_t osmId) : m_wayOsmId(osmId) {}
  WayElement(uint64_t osmId, std::vector<uint64_t> const & nodes) : m_wayOsmId(osmId), m_nodes(nodes) {}

  bool IsValid() const { return !m_nodes.empty(); }

  uint64_t GetOtherEndPoint(uint64_t id) const
  {
    if (id == m_nodes.front())
      return m_nodes.back();

    ASSERT(id == m_nodes.back(), ());
    return m_nodes.front();
  }

  template <class ToDo>
  void ForEachPoint(ToDo & toDo) const
  {
    std::for_each(m_nodes.begin(), m_nodes.end(), std::ref(toDo));
  }

  template <class ToDo>
  void ForEachPointOrdered(uint64_t start, ToDo && toDo)
  {
    ASSERT(!m_nodes.empty(), ());
    if (start == m_nodes.front())
      std::for_each(m_nodes.begin(), m_nodes.end(), std::ref(toDo));
    else
      std::for_each(m_nodes.rbegin(), m_nodes.rend(), std::ref(toDo));
  }

  template <class TWriter>
  void Write(TWriter & writer) const
  {
    uint64_t count = m_nodes.size();
    WriteVarUint(writer, count);
    for (uint64_t e : m_nodes)
      WriteVarUint(writer, e);
  }

  template <class TReader>
  void Read(TReader & reader)
  {
    ReaderSource<MemReader> r(reader);
    uint64_t count = ReadVarUint<uint64_t>(r);
    m_nodes.resize(count);
    for (uint64_t & e : m_nodes)
      e = ReadVarUint<uint64_t>(r);
  }
};

class RelationElement
{
public:
  using Member = std::pair<uint64_t, std::string>;

  std::vector<Member> m_nodes;
  std::vector<Member> m_ways;
  std::vector<Member> m_relations;
  // std::less<> is for transparent find using string_view.
  std::map<std::string, std::string, std::less<>> m_tags;

  bool IsValid() const { return !(m_nodes.empty() && m_ways.empty() && m_relations.empty()); }

  std::string_view GetTagValue(std::string_view key) const
  {
    auto const it = m_tags.find(key);
    return it == m_tags.end() ? std::string_view() : it->second;
  }

  std::string_view GetType() const { return GetTagValue("type"); }
  /// @returns (empty if absent) role for the way with given id.
  std::string_view GetWayRole(uint64_t id) const { return FindRoleImpl(m_ways, id); }
  /// @returns (empty if absent) role for the node with given id.
  std::string_view GetNodeRole(uint64_t id) const { return FindRoleImpl(m_nodes, id); }
  /// @returns (empty if absent) role for the relation with given id.
  std::string_view GetRelationRole(uint64_t id) const { return FindRoleImpl(m_relations, id); }

  template <class ToDo>
  void ForEachWay(ToDo & toDo) const
  {
    for (size_t i = 0; i < m_ways.size(); ++i)
      toDo(m_ways[i].first, m_ways[i].second);
  }

  void Swap(RelationElement & rhs)
  {
    m_nodes.swap(rhs.m_nodes);
    m_ways.swap(rhs.m_ways);
    m_relations.swap(rhs.m_relations);
    m_tags.swap(rhs.m_tags);
  }

  template <class TWriter>
  void Write(TWriter & writer) const
  {
    auto StringWriter = [&writer, this](std::string const & str)
    {
      CHECK_LESS(str.size(), std::numeric_limits<uint16_t>::max(),
                 ("Can't store std::string greater then 65535 bytes", *this));

      uint16_t sz = static_cast<uint16_t>(str.size());
      writer.Write(&sz, sizeof(sz));
      writer.Write(str.data(), sz);
    };

    auto MembersWriter = [&writer, &StringWriter](std::vector<Member> const & members)
    {
      uint64_t count = members.size();
      WriteVarUint(writer, count);
      for (auto const & e : members)
      {
        // write id
        WriteVarUint(writer, e.first);
        // write role
        StringWriter(e.second);
      }
    };

    MembersWriter(m_nodes);
    MembersWriter(m_ways);
    MembersWriter(m_relations);

    uint64_t count = m_tags.size();
    WriteVarUint(writer, count);
    for (auto const & e : m_tags)
    {
      // write key
      StringWriter(e.first);
      // write value
      StringWriter(e.second);
    }
  }

  template <class TReader>
  void Read(TReader & reader)
  {
    ReaderSource<TReader> r(reader);

    auto const StringReader = [&r](std::string & str)
    {
      uint16_t sz = 0;
      r.Read(&sz, sizeof(sz));
      str.resize(sz);
      r.Read(&str[0], sz);
    };

    auto const MembersReader = [&r, &StringReader](std::vector<Member> & members)
    {
      uint64_t count = ReadVarUint<uint64_t>(r);
      members.resize(count);
      for (auto & e : members)
      {
        // decode id
        e.first = ReadVarUint<uint64_t>(r);
        // decode role
        StringReader(e.second);
      }
    };

    MembersReader(m_nodes);
    MembersReader(m_ways);
    MembersReader(m_relations);

    // decode m_tags
    m_tags.clear();
    uint64_t const count = ReadVarUint<uint64_t>(r);
    for (uint64_t i = 0; i < count; ++i)
    {
      std::pair<std::string, std::string> kv;
      // decode key
      StringReader(kv.first);
      // decode value
      StringReader(kv.second);
      m_tags.emplace(kv);
    }
  }

  friend std::string DebugPrint(RelationElement const & element)
  {
    std::stringstream ss;
    for (auto const & e : element.m_tags)
      ss << e.first << "=" << e.second << std::endl;
    for (auto const & e : element.m_nodes)
      ss << "n{" << e.first << "," << e.second << "};";
    for (auto const & e : element.m_ways)
      ss << "w{" << e.first << "," << e.second << "};";
    for (auto const & e : element.m_tags)
      ss << "t{" << e.first << "," << e.second << "};";
    return ss.str();
  }

protected:
  static std::string_view FindRoleImpl(std::vector<Member> const & container, uint64_t id)
  {
    for (auto const & e : container)
      if (e.first == id)
        return e.second;
    return {};
  }
};
