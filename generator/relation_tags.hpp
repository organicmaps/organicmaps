#pragma once

#include "generator/intermediate_elements.hpp"
#include "generator/routing_helpers.hpp"

#include "base/assert.hpp"
#include "base/cache.hpp"

#include <cstdint>
#include <string>

class OsmElement;

namespace generator
{
/// Generated features should include parent relation tags to make
/// full types matching and storing any additional info.
class RelationTagsBase
{
public:
  explicit RelationTagsBase(routing::TagsProcessor & tagsProcessor);

  virtual ~RelationTagsBase() {}

  void Reset(uint64_t fID, OsmElement * p);

  template <class TReader>
  bool operator() (uint64_t id, TReader & reader)
  {
    bool exists = false;
    RelationElement & e = m_cache.Find(id, exists);
    if (!exists)
      CHECK(reader.Read(id, e), (id));

    Process(e);
    return false;
  }

protected:
  static bool IsSkipRelation(std::string const & type);
  bool IsKeyTagExists(std::string const & key) const;
  void AddCustomTag(std::pair<std::string, std::string> const & p);
  virtual void Process(RelationElement const & e) = 0;

  uint64_t m_featureID;
  OsmElement * m_current;
  routing::TagsProcessor & m_routingTagsProcessor;

private:
  my::Cache<uint64_t, RelationElement> m_cache;
};

class RelationTagsNode : public RelationTagsBase
{
  using TBase = RelationTagsBase;

public:
  explicit RelationTagsNode(routing::TagsProcessor & tagsProcessor);

protected:
  void Process(RelationElement const & e) override;
};


class RelationTagsWay : public RelationTagsBase
{
public:
  explicit RelationTagsWay(routing::TagsProcessor & routingTagsProcessor);

private:
  using TBase = RelationTagsBase;
  using TNameKeys = std::unordered_set<std::string>;

  bool IsAcceptBoundary(RelationElement const & e) const;

protected:
  void Process(RelationElement const & e) override;
};
}  // namespace generator
