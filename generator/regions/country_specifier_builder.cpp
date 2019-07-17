#include "country_specifier_builder.hpp"

namespace generator
{
namespace regions
{
// static
std::unordered_map<std::string, std::function<std::unique_ptr<CountrySpecifier>(void)>>
    CountrySpecifierBuilder::m_specifiers;
}  // namespace regions
}  // namespace generator
