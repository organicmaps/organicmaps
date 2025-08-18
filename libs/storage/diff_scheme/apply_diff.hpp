#pragma once

#include "mwm_diff/diff.hpp"

#include "storage/storage_defines.hpp"

#include <functional>

namespace base
{
class Cancellable;
}

namespace storage
{
namespace diffs
{
struct ApplyDiffParams
{
  std::string m_diffReadyPath;
  LocalFilePtr m_diffFile;
  LocalFilePtr m_oldMwmFile;
};

using OnDiffApplicationFinished = std::function<void(generator::mwm_diff::DiffApplicationResult)>;

void ApplyDiff(ApplyDiffParams && p, base::Cancellable const & cancellable, OnDiffApplicationFinished const & task);
}  // namespace diffs
}  // namespace storage
