#include "metrics/metrics_tests_support/eye_for_testing.hpp"

#include "metrics/eye.hpp"
#include "metrics/eye_storage.hpp"

#include "coding/internal/file_data.hpp"

#include <memory>

namespace eye
{
// static
void EyeForTesting::ResetEye()
{
  SetInfo({});

  auto const path = Storage::GetEyeFilePath();
  uint64_t unused;
  if (my::GetFileSize(path, unused))
    my::DeleteFileX(path);
}

// static
void EyeForTesting::AppendTip(Tip::Type type, Tip::Event event)
{
  Eye::Instance().AppendTip(type, event);
}

// static
void EyeForTesting::SetInfo(Info const & info)
{
  Eye::Instance().m_info.Set(std::make_shared<Info>(info));
}
}  // namespace eye
