#include "eye/eye_tests_support/eye_for_testing.hpp"

#include "eye/eye.hpp"
#include "eye/eye_storage.hpp"

#include "coding/internal/file_data.hpp"

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
void EyeForTesting::AppendTip(Tips::Type type, Tips::Event event)
{
  Eye::Instance().AppendTip(type, event);
}

// static
void EyeForTesting::SetInfo(Info const & info)
{
  Eye::Instance().m_info.Set(make_shared<Info>(info));
}
}  // namespace eye
