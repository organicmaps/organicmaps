#include "search/utils.hpp"

#include "indexer/data_source.hpp"

#include <cctype>

using namespace std;

namespace
{
vector<strings::UniString> const kAllowedMisprints = {
    strings::MakeUniString("ckq"),
    strings::MakeUniString("eyjiu"),
    strings::MakeUniString("gh"),
    strings::MakeUniString("pf"),
    strings::MakeUniString("vw"),
    strings::MakeUniString("ао"),
    strings::MakeUniString("еиэ"),
    strings::MakeUniString("шщ"),
};
}  // namespace

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
  // performance reasons, we limit prefix misprints to fixed set of substitutions defined in
  // kAllowedMisprints and skipped letters.
  return strings::LevenshteinDFA(s, 1 /* prefixSize */, kAllowedMisprints, GetMaxErrorsForToken(s));
}

MwmSet::MwmHandle FindWorld(DataSource const & dataSource,
                            vector<shared_ptr<MwmInfo>> const & infos)
{
  MwmSet::MwmHandle handle;
  for (auto const & info : infos)
  {
    if (info->GetType() == MwmInfo::WORLD)
    {
      handle = dataSource.GetMwmHandleById(MwmSet::MwmId(info));
      break;
    }
  }
  return handle;
}

MwmSet::MwmHandle FindWorld(DataSource const & dataSource)
{
  vector<shared_ptr<MwmInfo>> infos;
  dataSource.GetMwmsInfo(infos);
  return FindWorld(dataSource, infos);
}
}  // namespace search
