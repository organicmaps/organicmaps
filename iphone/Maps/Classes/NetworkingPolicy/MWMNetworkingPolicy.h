#include "std/function.hpp"

namespace networking_policy
{
using MWMPartnersApiFn = function<void(BOOL canUseNetwork)>;

void CallPartnersApi(MWMPartnersApiFn const & fn);

enum class Stage
{
  Always,
  Session,
  Never
};

void SetStage(Stage state);
Stage GetStage();
}  // namespace networking_policy
