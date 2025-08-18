#pragma once

#include "indexer/centers_table.hpp"

#include "coding/files_container.hpp"

#include "geometry/point2d.hpp"

#include <cstdint>
#include <memory>

class MwmValue;

namespace search
{
class LazyCentersTable
{
public:
  enum State
  {
    STATE_NOT_LOADED,
    STATE_LOADED,
    STATE_FAILED
  };

  explicit LazyCentersTable(MwmValue const & value);

  inline State GetState() const { return m_state; }

  void EnsureTableLoaded();

  [[nodiscard]] bool Get(uint32_t id, m2::PointD & center);

private:
  MwmValue const & m_value;
  State m_state;

  FilesContainerR::TReader m_reader;
  std::unique_ptr<CentersTable> m_table;
};
}  // namespace search
