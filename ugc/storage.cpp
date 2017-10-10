#include "ugc/storage.hpp"

#include "indexer/feature_decl.hpp"

namespace ugc
{
Storage::Storage(std::string const & filename)
  : m_filename(filename)
{
  Load();
}

UGCUpdate Storage::GetUGCUpdate(FeatureID const & id) const
{
  // Dummy
  return {};
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
