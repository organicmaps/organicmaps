#pragma once

#include "generator/intermediate_elements.hpp"
#include "generator/routing_helpers.hpp"

#include "base/assert.hpp"
#include "base/cache.hpp"
#include "base/control_flow.hpp"

#include <cstdint>
#include <string>
#include <utility>

struct OsmElement;

namespace generator
{
/// Generated features should include parent relation tags to make
/// full types matching and storing any additional info.
class RelationTagsBase
{
public:
  RelationTagsBase();
  virtual ~RelationTagsBase() = default;

  void Reset(uint64_t fID, OsmElement * p);

  template <class Reader>
  base::ControlFlow operator() (uint64_t id, Reader & reader)
  {
    bool exists = false;
    RelationElement & e = m_cache.Find(id, exists);
    if (!exists)
      CHECK(reader.Read(id, e), (id));

    Process(e);
    return base::ControlFlow::Continue;
  }

protected:
  static bool IsSkipRelation(std::string const & type);
  bool IsKeyTagExists(std::string const & key) const;
  void AddCustomTag(std::pair<std::string, std::string> const & p);
  virtual void Process(RelationElement const & e) = 0;

  uint64_t m_featureID = 0;
  OsmElement * m_current = nullptr;

private:
  base::Cache<uint64_t, RelationElement> m_cache;
};

class RelationTagsNode : public RelationTagsBase
{
protected:
  void Process(RelationElement const & e) override;

private:
    using Base = RelationTagsBase;
};

class RelationTagsWay : public RelationTagsBase
{
private:
  using Base = RelationTagsBase;
  using NameKeys = std::unordered_set<std::string>;

  bool IsAcceptBoundary(RelationElement const & e) const;

protected:
  void Process(RelationElement const & e) override;
};
}  // namespace generator
