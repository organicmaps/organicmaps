#pragma once

#include <string>

namespace generator
{
namespace mwm_diff
{
// Makes a diff that, when applied to the mwm at |oldMwmPath|, will
// result in the mwm at |newMwmPath|. The diff is stored at |diffPath|.
// It is assumed that the files at |oldMwmPath| and |newMwmPath| are valid mwms.
// Returns true on success and false on failure.
bool MakeDiff(std::string const & oldMwmPath, std::string const & newMwmPath,
              std::string const & diffPath);

// Applies the diff at |diffPath| to the mwm at |oldMwmPath|. The resulting
// mwm is stored at |newMwmPath|.
// It is assumed that the file at |oldMwmPath| is a valid mwm and the file
// at |diffPath| is a valid mwmdiff.
// Returns true on success and false on failure.
bool ApplyDiff(std::string const & oldMwmPath, std::string const & newMwmPath,
               std::string const & diffPath);
}  // namespace mwm_diff
}  // namespace generator
