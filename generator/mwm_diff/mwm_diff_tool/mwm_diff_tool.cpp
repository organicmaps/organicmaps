#include "generator/mwm_diff/diff.hpp"

#include "base/cancellable.hpp"

#include <iostream>
#include <cstring>

int main(int argc, char ** argv)
{
  if (argc < 5)
  {
    std::cout <<
        "Usage: " << argv[0] << " make|apply olderMWMDir newerMWMDir diffDir\n"
        "make\n"
        "  Creates the diff between newer and older MWM versions at `diffDir`\n"
        "apply\n"
        "  Applies the diff at `diffDir` to the mwm at `olderMWMDir` and stores result at `newerMWMDir`.\n"
        "WARNING: THERE IS NO MWM VALIDITY CHECK!\n";
    return -1;
  }
  char const * olderMWMDir{argv[2]}, * newerMWMDir{argv[3]}, * diffDir{argv[4]};
  if (0 == std::strcmp(argv[1], "make"))
    return generator::mwm_diff::MakeDiff(olderMWMDir, newerMWMDir, diffDir);

  // apply
  base::Cancellable cancellable;
  auto const res = generator::mwm_diff::ApplyDiff(olderMWMDir, newerMWMDir, diffDir, cancellable);
  if (res == generator::mwm_diff::DiffApplicationResult::Ok)
    return 0;

  return -1;  // Failed, Cancelled.
}
