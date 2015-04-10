#include "storage/data_header.hpp"

#include "base/string_utils.hpp"

#include "platform/platform.hpp"

#include "coding/file_writer.hpp"

#include "indexer/cell_id.hpp"

#include "base/start_mem_debug.hpp"

namespace feature
{

  DataHeader::DataHeader()
  {
    Reset();
  }

  namespace
  {
    struct do_reset
    {
      void operator() (string & t, int) { t.clear(); }
      void operator() (uint64_t & t, int) { t = 0; }
      void operator() (pair<int64_t, int64_t> &, int) {}
    };
  }

  void DataHeader::Reset()
  {
    do_reset doReset;
    for_each_tuple(m_params, doReset);
  }

  m2::RectD const DataHeader::Bounds() const
  {
    return Int64ToRect(Get<EBoundary>());
  }

  void DataHeader::SetBounds(m2::RectD const & r)
  {
    Set<EBoundary>(RectToInt64(r));
  }

}
