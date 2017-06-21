#include "ugc/storage.hpp"

#include "indexer/feature_decl.hpp"

namespace ugc
{
Storage::Storage(std::string const & filename)
  : m_filename(filename)
{
  Load();
}

UGCUpdate const * Storage::GetUGCUpdate(FeatureID const & id) const
{
  auto const it = m_ugc.find(id);
  if (it != end(m_ugc))
    return &it->second;
  return nullptr;
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
