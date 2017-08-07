#pragma once

#include "storage/storage.hpp"

#include <memory>
#include <string>

namespace storage
{
class CountryParentGetter
{
public:
  CountryParentGetter(std::string const & countriesFile = "",
                      std::string const & countriesDir = "");
  std::string operator()(std::string const & id) const;

private:
  shared_ptr<Storage> m_storage;
};
}  // namespace storage
