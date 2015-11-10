#include "indexer/feature_meta.hpp"

#include "std/algorithm.hpp"

namespace feature
{

string Metadata::GetWikiURL() const
{
  string v = this->Get(FMD_WIKIPEDIA);
  if (v.empty())
    return v;

  auto const colon = v.find(':');
  if (colon == string::npos)
    return v;

  // Spaces and % sign should be replaced in urls.
  replace(v.begin() + colon, v.end(), ' ', '_');
  string::size_type percent, pos = colon;
  string const escapedPercent("%25");
  while ((percent = v.find('%', pos)) != string::npos)
  {
    v.replace(percent, 1, escapedPercent);
    pos = percent + escapedPercent.size();
  }
  // Trying to avoid redirects by constructing the right link.
  // TODO: Wikipedia article could be opened it a user's language, but need
  // generator level support to check for available article languages first.
  return "https://" + v.substr(0, colon) + ".m.wikipedia.org/wiki/" + v.substr(colon + 1);
}

}  // namespace feature
