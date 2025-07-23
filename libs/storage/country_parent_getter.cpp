#include "storage/country_parent_getter.hpp"

namespace storage
{
CountryParentGetter::CountryParentGetter(std::string const & countriesFile, std::string const & countriesDir)
{
  if (countriesFile.empty())
    m_storage = std::make_shared<Storage>();
  else
    m_storage = std::make_shared<Storage>(countriesFile, countriesDir);
}

std::string CountryParentGetter::operator()(std::string const & id) const
{
  return m_storage->GetParentIdFor(id);
}
}  // namespace storage
