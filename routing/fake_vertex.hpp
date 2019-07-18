#pragma once

#include "routing/road_graph.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/point2d.hpp"

#include "base/visitor.hpp"

#include <sstream>
#include <string>
#include <tuple>
#include <utility>

namespace routing
{
class FakeVertex final
{
public:
  enum class Type
  {
    PureFake,
    PartOfReal,
  };

  FakeVertex(NumMwmId numMwmId, Junction const & from, Junction const & to, Type type)
    : m_numMwmId(numMwmId), m_from(from), m_to(to), m_type(type)
  {
  }

  FakeVertex() = default;

  bool operator==(FakeVertex const & rhs) const
  {
    return std::tie(m_numMwmId, m_from, m_to, m_type) ==
           std::tie(rhs.m_numMwmId, rhs.m_from, rhs.m_to, rhs.m_type);
  }

  bool operator<(FakeVertex const & rhs) const
  {
    return std::tie(m_numMwmId, m_from, m_to, m_type) <
           std::tie(rhs.m_numMwmId, rhs.m_from, rhs.m_to, rhs.m_type);
  }

  Junction const & GetJunctionFrom() const { return m_from; }
  m2::PointD const & GetPointFrom() const { return m_from.GetPoint(); }
  Junction const & GetJunctionTo() const { return m_to; }
  m2::PointD const & GetPointTo() const { return m_to.GetPoint(); }
  Type GetType() const { return m_type; }

  DECLARE_VISITOR(visitor(m_numMwmId, "m_numMwmId"), visitor(m_from, "m_from"),
                  visitor(m_to, "m_to"), visitor(m_type, "m_type"))
  DECLARE_DEBUG_PRINT(FakeVertex)

private:
  // Note. It's important to know which mwm contains the FakeVertex if it is located
  // near an mwm borders along road features which are duplicated.
  NumMwmId m_numMwmId = kFakeNumMwmId;
  Junction m_from;
  Junction m_to;
  Type m_type = Type::PureFake;
};

inline std::string DebugPrint(FakeVertex::Type type)
{
  switch (type)
  {
  case FakeVertex::Type::PureFake: return "PureFake";
  case FakeVertex::Type::PartOfReal: return "PartOfReal";
  }
}
}  // namespace routing
