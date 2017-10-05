#include "ugc/storage.hpp"

#include "indexer/feature_decl.hpp"

namespace ugc
{
Storage::Storage(std::string const & filename)
  : m_filename(filename)
{
  Load();
}

void Storage::GetUGCUpdate(FeatureID const & id, UGCUpdate & ugc) const
{
  auto const it = m_ugc.find(id);
  if (it == end(m_ugc))
    return;

  ugc = it->second;
}

void Storage::SetUGCUpdate(FeatureID const & id, UGCUpdate const & ugc)
{
  m_ugc[id] = ugc;
}

void Storage::Load()
{
}

void Storage::Save()
{
}
}  // namespace ugc
