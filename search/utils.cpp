#include "search/utils.hpp"

#include "indexer/index.hpp"

#include <cctype>

using namespace std;

namespace search
{
size_t GetMaxErrorsForToken(strings::UniString const & token)
{
  bool const digitsOnly = all_of(token.begin(), token.end(), ::isdigit);
  if (digitsOnly)
    return 0;
  if (token.size() < 4)
    return 0;
  if (token.size() < 8)
    return 1;
  return 2;
}

strings::LevenshteinDFA BuildLevenshteinDFA(strings::UniString const & s)
{
  // In search we use LevenshteinDFAs for fuzzy matching. But due to
  // performance reasons, we assume that the first letter is always
  // correct.
  return strings::LevenshteinDFA(s, 1 /* prefixCharsToKeep */, GetMaxErrorsForToken(s));
}

MwmSet::MwmHandle FindWorld(Index const & index, vector<shared_ptr<MwmInfo>> const & infos)
{
  MwmSet::MwmHandle handle;
  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::WORLD)
    {
      handle = index.GetMwmHandleById(MwmSet::MwmId(info));
      break;
    }
  }
  return handle;
}

MwmSet::MwmHandle FindWorld(Index const & index)
{
  vector<shared_ptr<MwmInfo>> infos;
  index.GetMwmsInfo(infos);
  return FindWorld(index, infos);
}
}  // namespace search
