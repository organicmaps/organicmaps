#include "generator/mwm_diff/diff.hpp"

#include "base/cancellable.hpp"

#include <iostream>
#include <cstring>

void usage(char* executable) {
  std::cout <<
    "Usage: " << executable << " { make | apply } OLD.mwm NEW.mwm DIFF.mwmdiff\n"
    "make\n"
    "  Writes the diff between `OLD.mwm` and `NEW.mwm` files to `DIFF.mwmdiff`\n"
    "apply\n"
    "  Applies the diff at `DIFF.mwmdiff` to `OLD.mwm` and stores result at `NEW.mwm`.\n"
    "WARNING: THERE IS NO MWM VALIDITY CHECK!\n"
    "WARNING: EXISTING FILES WILL BE OVERWRITTEN!\n";
}

int main(int argc, char ** argv)
{
  if (argc != 5)
  {
    usage(argv[0]);
    return -1;
  }
  char const * olderMWM{argv[2]}, * newerMWM{argv[3]}, * diffPath{argv[4]};
  if (0 == std::strcmp(argv[1], "make")) {
    if (generator::mwm_diff::MakeDiff(olderMWM, newerMWM, diffPath))
      return 0;
  } else if (0 == std::strcmp(argv[1], "apply")) {
    base::Cancellable cancellable;
    auto const res = generator::mwm_diff::ApplyDiff(olderMWM, newerMWM, diffPath, cancellable);
    if (res == generator::mwm_diff::DiffApplicationResult::Ok)
      return 0;
  } else {
    usage(argv[0]);
  }

  return -1;  // Failed, Cancelled.
}
