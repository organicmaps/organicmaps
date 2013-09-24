#include <gmock/internal/gmock-internal-utils.h>

#include "../../testing/testing.hpp"

namespace testing
{
  namespace internal
  {
    class MwmFailureReporter : public FailureReporterInterface
    {
      bool m_throwed;
    public:
      MwmFailureReporter()
        : m_throwed(false) {}

      virtual void ReportFailure(FailureType type,
                                 const char* file,
                                 int line,
                                 const string& message)
      {
        if (!m_throwed)
        {
          m_throwed = true;
          my::OnTestFailed(my::SrcPoint(file == NULL ? "" : file,
                                        line, ""), message);
        }
      }
    };

    FailureReporterInterface * GetFailureReporter()
    {
      static MwmFailureReporter reporter;
      return &reporter;
    }
  }
}
