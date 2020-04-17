#include "generator/final_processor_interface.hpp"

namespace generator
{
FinalProcessorIntermediateMwmInterface::FinalProcessorIntermediateMwmInterface(
    FinalProcessorPriority priority)
  : m_priority(priority)
{
}

bool FinalProcessorIntermediateMwmInterface::operator<(
    FinalProcessorIntermediateMwmInterface const & other) const
{
  return m_priority < other.m_priority;
}

bool FinalProcessorIntermediateMwmInterface::operator==(
    FinalProcessorIntermediateMwmInterface const & other) const
{
  return m_priority == other.m_priority;
}

bool FinalProcessorIntermediateMwmInterface::operator!=(
    FinalProcessorIntermediateMwmInterface const & other) const
{
  return m_priority != other.m_priority;
}
}  // namespace generator
