#include "country_specifier_builder.hpp"

#include "generator/regions/specs/afghanistan.hpp"
#include "generator/regions/specs/albania.hpp"
#include "generator/regions/specs/algeria.hpp"
#include "generator/regions/specs/andorra.hpp"
#include "generator/regions/specs/angola.hpp"
#include "generator/regions/specs/antigua_and_barbuda.hpp"
#include "generator/regions/specs/australia.hpp"
#include "generator/regions/specs/austria.hpp"
#include "generator/regions/specs/azerbaijan.hpp"
#include "generator/regions/specs/bahamas.hpp"
#include "generator/regions/specs/bahrain.hpp"
#include "generator/regions/specs/bangladesh.hpp"
#include "generator/regions/specs/barbados.hpp"
#include "generator/regions/specs/belarus.hpp"
#include "generator/regions/specs/belgium.hpp"
#include "generator/regions/specs/benin.hpp"
#include "generator/regions/specs/bhutan.hpp"
#include "generator/regions/specs/bosnia_and_herzegovina.hpp"
#include "generator/regions/specs/brazil.hpp"
#include "generator/regions/specs/bulgaria.hpp"
#include "generator/regions/specs/burkina_faso.hpp"
#include "generator/regions/specs/burundi.hpp"
#include "generator/regions/specs/cameroon.hpp"
#include "generator/regions/specs/canada.hpp"
#include "generator/regions/specs/chad.hpp"
#include "generator/regions/specs/china.hpp"
#include "generator/regions/specs/cote_divoire.hpp"
#include "generator/regions/specs/croatia.hpp"
#include "generator/regions/specs/cyprus.hpp"
#include "generator/regions/specs/czech_republic.hpp"
#include "generator/regions/specs/democratic_republic_of_the_congo.hpp"
#include "generator/regions/specs/denmark.hpp"
#include "generator/regions/specs/dominican_republic.hpp"
#include "generator/regions/specs/egypt.hpp"
#include "generator/regions/specs/estonia.hpp"
#include "generator/regions/specs/ethiopia.hpp"
#include "generator/regions/specs/finland.hpp"
#include "generator/regions/specs/france.hpp"
#include "generator/regions/specs/french_polynesia.hpp"
#include "generator/regions/specs/gabon.hpp"
#include "generator/regions/specs/gambia.hpp"
#include "generator/regions/specs/georgia.hpp"
#include "generator/regions/specs/germany.hpp"
#include "generator/regions/specs/ghana.hpp"
#include "generator/regions/specs/greece.hpp"
#include "generator/regions/specs/guinea.hpp"
#include "generator/regions/specs/haiti.hpp"
#include "generator/regions/specs/hong_kong.hpp"
#include "generator/regions/specs/hungary.hpp"
#include "generator/regions/specs/iceland.hpp"
#include "generator/regions/specs/india.hpp"
#include "generator/regions/specs/indonesia.hpp"
#include "generator/regions/specs/iran.hpp"
#include "generator/regions/specs/iraq.hpp"
#include "generator/regions/specs/ireland.hpp"
#include "generator/regions/specs/isle_of_man.hpp"
#include "generator/regions/specs/israel.hpp"
#include "generator/regions/specs/italy.hpp"
#include "generator/regions/specs/japan.hpp"
#include "generator/regions/specs/jordan.hpp"
#include "generator/regions/specs/kosovo.hpp"
#include "generator/regions/specs/laos.hpp"
#include "generator/regions/specs/latvia.hpp"
#include "generator/regions/specs/lebanon.hpp"
#include "generator/regions/specs/lesotho.hpp"
#include "generator/regions/specs/liberia.hpp"
#include "generator/regions/specs/libya.hpp"
#include "generator/regions/specs/lithuania.hpp"
#include "generator/regions/specs/luxembourg.hpp"
#include "generator/regions/specs/macedonia.hpp"
#include "generator/regions/specs/madagascar.hpp"
#include "generator/regions/specs/malawi.hpp"
#include "generator/regions/specs/malaysia.hpp"
#include "generator/regions/specs/mali.hpp"
#include "generator/regions/specs/mauritania.hpp"
#include "generator/regions/specs/moldova.hpp"
#include "generator/regions/specs/morocco.hpp"
#include "generator/regions/specs/mozambique.hpp"
#include "generator/regions/specs/myanmar.hpp"
#include "generator/regions/specs/nepal.hpp"
#include "generator/regions/specs/netherlands.hpp"
#include "generator/regions/specs/new_zealand.hpp"
#include "generator/regions/specs/niger.hpp"
#include "generator/regions/specs/nigeria.hpp"
#include "generator/regions/specs/north_korea.hpp"
#include "generator/regions/specs/norway.hpp"
#include "generator/regions/specs/philippines.hpp"
#include "generator/regions/specs/poland.hpp"
#include "generator/regions/specs/portugal.hpp"
#include "generator/regions/specs/romania.hpp"
#include "generator/regions/specs/russia.hpp"
#include "generator/regions/specs/senegal.hpp"
#include "generator/regions/specs/serbia.hpp"
#include "generator/regions/specs/sierra_leone.hpp"
#include "generator/regions/specs/slovakia.hpp"
#include "generator/regions/specs/slovenia.hpp"
#include "generator/regions/specs/south_africa.hpp"
#include "generator/regions/specs/south_korea.hpp"
#include "generator/regions/specs/south_sudan.hpp"
#include "generator/regions/specs/swaziland.hpp"
#include "generator/regions/specs/sweden.hpp"
#include "generator/regions/specs/switzerland.hpp"
#include "generator/regions/specs/syria.hpp"
#include "generator/regions/specs/taiwan.hpp"
#include "generator/regions/specs/tajikistan.hpp"
#include "generator/regions/specs/tanzania.hpp"
#include "generator/regions/specs/thailand.hpp"
#include "generator/regions/specs/the_central_african_republic.hpp"
#include "generator/regions/specs/togo.hpp"
#include "generator/regions/specs/tunisia.hpp"
#include "generator/regions/specs/turkey.hpp"
#include "generator/regions/specs/uganda.hpp"
#include "generator/regions/specs/ukraine.hpp"
#include "generator/regions/specs/united_kingdom.hpp"
#include "generator/regions/specs/united_states.hpp"
#include "generator/regions/specs/vanuatu.hpp"
#include "generator/regions/specs/vietnam.hpp"


namespace generator
{
namespace regions
{
std::unique_ptr<CountrySpecifier> GetCountrySpecifier(std::string const & countryName)
{
  if (countryName == u8"Afghanistan")
    return std::make_unique<specs::AfghanistanSpecifier>();
  if (countryName == u8"Albania")
    return std::make_unique<specs::AlbaniaSpecifier>();
  if (countryName == u8"Algeria")
    return std::make_unique<specs::AlgeriaSpecifier>();
  if (countryName == u8"Andorra")
    return std::make_unique<specs::AndorraSpecifier>();
  if (countryName == u8"Angola")
    return std::make_unique<specs::AngolaSpecifier>();
  if (countryName == u8"Antigua and Barbuda")
    return std::make_unique<specs::AntiguaAndBarbudaSpecifier>();
  if (countryName == u8"Australia")
    return std::make_unique<specs::AustraliaSpecifier>();
  if (countryName == u8"Austria")
    return std::make_unique<specs::AustriaSpecifier>();
  if (countryName == u8"Azerbaijan")
    return std::make_unique<specs::AzerbaijanSpecifier>();
  if (countryName == u8"Bahamas")
    return std::make_unique<specs::BahamasSpecifier>();
  if (countryName == u8"Bahrain")
    return std::make_unique<specs::BahrainSpecifier>();
  if (countryName == u8"Bangladesh")
    return std::make_unique<specs::BangladeshSpecifier>();
  if (countryName == u8"Barbados")
    return std::make_unique<specs::BarbadosSpecifier>();
  if (countryName == u8"Belarus")
    return std::make_unique<specs::BelarusSpecifier>();
  if (countryName == u8"Belgium")
    return std::make_unique<specs::BelgiumSpecifier>();
  if (countryName == u8"Benin")
    return std::make_unique<specs::BeninSpecifier>();
  if (countryName == u8"Bhutan")
    return std::make_unique<specs::BhutanSpecifier>();
  if (countryName == u8"Bosnia and Herzegovina")
    return std::make_unique<specs::BosniaAndHerzegovinaSpecifier>();
  if (countryName == u8"Brazil")
    return std::make_unique<specs::BrazilSpecifier>();
  if (countryName == u8"Bulgaria")
    return std::make_unique<specs::BulgariaSpecifier>();
  if (countryName == u8"Burkina Faso")
    return std::make_unique<specs::BurkinaFasoSpecifier>();
  if (countryName == u8"Burundi")
    return std::make_unique<specs::BurundiSpecifier>();
  if (countryName == u8"Canada")
    return std::make_unique<specs::CanadaSpecifier>();
  if (countryName == u8"Cameroon")
    return std::make_unique<specs::CameroonSpecifier>();
  if (countryName == u8"The Central African Republic")
    return std::make_unique<specs::TheCentralAfricanRepublicSpecifier>();
  if (countryName == u8"Chad")
    return std::make_unique<specs::ChadSpecifier>();
  if (countryName == u8"Croatia")
    return std::make_unique<specs::CroatiaSpecifier>();
  if (countryName == u8"China")
    return std::make_unique<specs::ChinaSpecifier>();
  if (countryName == u8"Hong Kong")
    return std::make_unique<specs::HongKongSpecifier>();
  if (countryName == u8"Cyprus")
    return std::make_unique<specs::CyprusSpecifier>();
  if (countryName == u8"Czech Republic")
    return std::make_unique<specs::CzechRepublicSpecifier>();
  if (countryName == u8"Democratic Republic of the Congo")
    return std::make_unique<specs::DemocraticRepublicOfTheCongoSpecifier>();
  if (countryName == u8"Denmark")
    return std::make_unique<specs::DenmarkSpecifier>();
  if (countryName == u8"Dominican Republic")
    return std::make_unique<specs::DominicanRepublicSpecifier>();
  if (countryName == u8"Egypt")
    return std::make_unique<specs::EgyptSpecifier>();
  if (countryName == u8"Estonia")
    return std::make_unique<specs::EstoniaSpecifier>();
  if (countryName == u8"Ethiopia")
    return std::make_unique<specs::EthiopiaSpecifier>();
  if (countryName == u8"Finland")
    return std::make_unique<specs::FinlandSpecifier>();
  if (countryName == u8"France")
    return std::make_unique<specs::FranceSpecifier>();
  if (countryName == u8"French Polynesia")
    return std::make_unique<specs::FrenchPolynesiaSpecifier>();
  if (countryName == u8"Gabon")
    return std::make_unique<specs::GabonSpecifier>();
  if (countryName == u8"Gambia")
    return std::make_unique<specs::GambiaSpecifier>();
  if (countryName == u8"Georgia")
    return std::make_unique<specs::GeorgiaSpecifier>();
  if (countryName == u8"Germany")
    return std::make_unique<specs::GermanySpecifier>();
  if (countryName == u8"Ghana")
    return std::make_unique<specs::GhanaSpecifier>();
  if (countryName == u8"Guinea")
    return std::make_unique<specs::GuineaSpecifier>();
  if (countryName == u8"Greece")
    return std::make_unique<specs::GreeceSpecifier>();
  if (countryName == u8"Haiti")
    return std::make_unique<specs::HaitiSpecifier>();
  if (countryName == u8"Hungary")
    return std::make_unique<specs::HungarySpecifier>();
  if (countryName == u8"Iceland")
    return std::make_unique<specs::IcelandSpecifier>();
  if (countryName == u8"India")
    return std::make_unique<specs::IndiaSpecifier>();
  if (countryName == u8"Indonesia")
    return std::make_unique<specs::IndonesiaSpecifier>();
  if (countryName == u8"Iraq")
    return std::make_unique<specs::IraqSpecifier>();
  if (countryName == u8"Iran")
    return std::make_unique<specs::IranSpecifier>();
  if (countryName == u8"Ireland")
    return std::make_unique<specs::IrelandSpecifier>();
  if (countryName == u8"Isle of Man")
    return std::make_unique<specs::IsleOfManSpecifier>();
  if (countryName == u8"Israel")
    return std::make_unique<specs::IsraelSpecifier>();
  if (countryName == u8"Italy")
    return std::make_unique<specs::ItalySpecifier>();
  if (countryName == u8"Côte d'Ivoire")
    return std::make_unique<specs::CoteDivoireSpecifier>();
  if (countryName == u8"Japan")
    return std::make_unique<specs::JapanSpecifier>();
  if (countryName == u8"Jordan")
    return std::make_unique<specs::JordanSpecifier>();
  if (countryName == u8"Kosovo")
    return std::make_unique<specs::KosovoSpecifier>();
  if (countryName == u8"Laos")
    return std::make_unique<specs::LaosSpecifier>();
  if (countryName == u8"Latvia")
    return std::make_unique<specs::LatviaSpecifier>();
  if (countryName == u8"Lebanon")
    return std::make_unique<specs::LebanonSpecifier>();
  if (countryName == u8"Lesotho")
    return std::make_unique<specs::LesothoSpecifier>();
  if (countryName == u8"Liberia")
    return std::make_unique<specs::LiberiaSpecifier>();
  if (countryName == u8"Libya")
    return std::make_unique<specs::LibyaSpecifier>();
  if (countryName == u8"Lithuania")
    return std::make_unique<specs::LithuaniaSpecifier>();
  if (countryName == u8"Luxembourg")
    return std::make_unique<specs::LuxembourgSpecifier>();
  if (countryName == u8"Macedonia")
    return std::make_unique<specs::MacedoniaSpecifier>();
  if (countryName == u8"Madagascar")
    return std::make_unique<specs::MadagascarSpecifier>();
  if (countryName == u8"Malaysia")
    return std::make_unique<specs::MalaysiaSpecifier>();
  if (countryName == u8"Malawi")
    return std::make_unique<specs::MalawiSpecifier>();
  if (countryName == u8"Mali")
    return std::make_unique<specs::MaliSpecifier>();
  if (countryName == u8"Mauritania")
    return std::make_unique<specs::MauritaniaSpecifier>();
  if (countryName == u8"Moldova")
    return std::make_unique<specs::MoldovaSpecifier>();
  if (countryName == u8"Morocco")
    return std::make_unique<specs::MoroccoSpecifier>();
  if (countryName == u8"Mozambique")
    return std::make_unique<specs::MozambiqueSpecifier>();
  if (countryName == u8"Myanmar")
    return std::make_unique<specs::MyanmarSpecifier>();
  if (countryName == u8"Netherlands")
    return std::make_unique<specs::NetherlandsSpecifier>();
  if (countryName == u8"Nepal")
    return std::make_unique<specs::NepalSpecifier>();
  if (countryName == u8"New Zealand")
    return std::make_unique<specs::NewZealandSpecifier>();
  if (countryName == u8"Niger")
    return std::make_unique<specs::NigerSpecifier>();
  if (countryName == u8"Nigeria")
    return std::make_unique<specs::NigeriaSpecifier>();
  if (countryName == u8"North Korea")
    return std::make_unique<specs::NorthKoreaSpecifier>();
  if (countryName == u8"Norway")
    return std::make_unique<specs::NorwaySpecifier>();
  if (countryName == u8"Philippines")
    return std::make_unique<specs::PhilippinesSpecifier>();
  if (countryName == u8"Poland")
    return std::make_unique<specs::PolandSpecifier>();
  if (countryName == u8"Portugal")
    return std::make_unique<specs::PortugalSpecifier>();
  if (countryName == u8"Romania")
    return std::make_unique<specs::RomaniaSpecifier>();
  if (countryName == u8"Russia")
    return std::make_unique<specs::RussiaSpecifier>();
  if (countryName == u8"Serbia")
    return std::make_unique<specs::SerbiaSpecifier>();
  if (countryName == u8"Senegal")
    return std::make_unique<specs::SenegalSpecifier>();
  if (countryName == u8"Sierra Leone")
    return std::make_unique<specs::SierraLeoneSpecifier>();
  if (countryName == u8"Slovakia")
    return std::make_unique<specs::SlovakiaSpecifier>();
  if (countryName == u8"Slovenia")
    return std::make_unique<specs::SloveniaSpecifier>();
  if (countryName == u8"South Africa")
    return std::make_unique<specs::SouthAfricaSpecifier>();
  if (countryName == u8"South Korea")
    return std::make_unique<specs::SouthKoreaSpecifier>();
  if (countryName == u8"South Korea")
    return std::make_unique<specs::SouthKoreaSpecifier>();
  if (countryName == u8"South Sudan")
    return std::make_unique<specs::SouthSudanSpecifier>();
  if (countryName == u8"Swaziland")
    return std::make_unique<specs::SwazilandSpecifier>();
  if (countryName == u8"Switzerland")
    return std::make_unique<specs::SwitzerlandSpecifier>();
  if (countryName == u8"Sweden")
    return std::make_unique<specs::SwedenSpecifier>();
  if (countryName == u8"Syria")
    return std::make_unique<specs::SyriaSpecifier>();
  if (countryName == u8"Taiwan")
    return std::make_unique<specs::TaiwanSpecifier>();
  if (countryName == u8"Tajikistan")
    return std::make_unique<specs::TajikistanSpecifier>();
  if (countryName == u8"Tanzania")
    return std::make_unique<specs::TanzaniaSpecifier>();
  if (countryName == u8"Thailand")
    return std::make_unique<specs::ThailandSpecifier>();
  if (countryName == u8"Togo")
    return std::make_unique<specs::TogoSpecifier>();
  if (countryName == u8"Tunisia")
    return std::make_unique<specs::TunisiaSpecifier>();
  if (countryName == u8"Turkey")
    return std::make_unique<specs::TurkeySpecifier>();
  if (countryName == u8"Uganda")
    return std::make_unique<specs::UgandaSpecifier>();
  if (countryName == u8"Ukraine")
    return std::make_unique<specs::UkraineSpecifier>();
  if (countryName == u8"United Kingdom")
    return std::make_unique<specs::UnitedKingdomSpecifier>();
  if (countryName == u8"United States")
    return std::make_unique<specs::UnitedStatesSpecifier>();
  if (countryName == u8"Vanuatu")
    return std::make_unique<specs::VanuatuSpecifier>();
  if (countryName == u8"Vietnam")
    return std::make_unique<specs::VietnamSpecifier>();

  return std::make_unique<CountrySpecifier>();
}
}  // namespace regions
}  // namespace generator
