#pragma once

#include "generator/regions/node.hpp"

#include <string>

namespace generator
{
namespace regions
{
class ToStringPolicyInterface
{
public:
  virtual ~ToStringPolicyInterface() = default;

  virtual std::string ToString(Node::PtrList const & nodePtrList) = 0;
};

class JsonPolicy : public ToStringPolicyInterface
{
public:
  JsonPolicy(bool extendedOutput = false) : m_extendedOutput(extendedOutput) {}

  std::string ToString(Node::PtrList const & nodePtrList) override;

private:
  bool m_extendedOutput;
};
}  // namespace regions
}  // namespace generator
