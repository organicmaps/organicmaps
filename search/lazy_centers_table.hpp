#pragma once

#include "indexer/centers_table.hpp"

#include "coding/file_container.hpp"

#include "geometry/point2d.hpp"

#include "std/unique_ptr.hpp"

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

  explicit LazyCentersTable(MwmValue & value);

  inline State GetState() const { return m_state; }

  void EnsureTableLoaded();

  WARN_UNUSED_RESULT bool Get(uint32_t id, m2::PointD & center);

private:
  MwmValue & m_value;
  State m_state;

  FilesContainerR::TReader m_reader;
  unique_ptr<CentersTable> m_table;
};
}  // namespace search
