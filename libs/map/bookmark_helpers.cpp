#include "map/bookmark_helpers.hpp"

#include "drape_frontend/visual_params.hpp"

#include "kml/serdes.hpp"
#include "kml/serdes_binary.hpp"
#include "kml/serdes_geojson.hpp"
#include "kml/serdes_gpx.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_data.hpp"

#include "platform/localization.hpp"
#include "platform/platform.hpp"
#include "platform/preferred_languages.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "coding/internal/file_data.hpp"
#include "coding/zip_reader.hpp"

#include "base/file_name_utils.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <map>
#include <sstream>

namespace
{
struct BookmarkMatchInfo
{
  BookmarkMatchInfo(kml::BookmarkIcon icon, BookmarkBaseType type) : m_icon(icon), m_type(type) {}

  kml::BookmarkIcon m_icon;
  BookmarkBaseType m_type;
};

std::map<std::string, BookmarkMatchInfo> const kFeatureTypeToBookmarkMatchInfo = {
    {"amenity-veterinary", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},
    {"leisure-dog_park", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},
    {"tourism-zoo", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},

    {"amenity-bar", {kml::BookmarkIcon::Bar, BookmarkBaseType::Food}},
    {"amenity-biergarten", {kml::BookmarkIcon::Pub, BookmarkBaseType::Food}},
    {"amenity-pub", {kml::BookmarkIcon::Pub, BookmarkBaseType::Food}},
    {"amenity-cafe", {kml::BookmarkIcon::Cafe, BookmarkBaseType::Food}},

    {"amenity-bbq", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"amenity-food_court", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"amenity-restaurant", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"leisure-picnic_table", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"tourism-picnic_site", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},

    {"amenity-fast_food", {kml::BookmarkIcon::FastFood, BookmarkBaseType::Food}},

    {"amenity-place_of_worship-buddhist", {kml::BookmarkIcon::Buddhism, BookmarkBaseType::ReligiousPlace}},

    {"amenity-college", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"amenity-courthouse", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"amenity-kindergarten", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"amenity-library", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"amenity-police", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"amenity-prison", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"amenity-school", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"building-university", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"office", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"office-diplomatic", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"office-lawyer", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},

    {"amenity-grave_yard-christian", {kml::BookmarkIcon::Christianity, BookmarkBaseType::ReligiousPlace}},
    {"amenity-place_of_worship-christian", {kml::BookmarkIcon::Christianity, BookmarkBaseType::ReligiousPlace}},
    {"landuse-cemetery-christian", {kml::BookmarkIcon::Christianity, BookmarkBaseType::ReligiousPlace}},

    {"amenity-casino", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"amenity-cinema", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"amenity-nightclub", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"shop-bookmaker", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"tourism-theme_park", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},

    {"amenity-theatre", {kml::BookmarkIcon::Theatre, BookmarkBaseType::Entertainment}},

    {"amenity-atm", {kml::BookmarkIcon::Bank, BookmarkBaseType::Exchange}},
    {"amenity-bank", {kml::BookmarkIcon::Bank, BookmarkBaseType::Exchange}},
    {"shop-money_lender", {kml::BookmarkIcon::Bank, BookmarkBaseType::Exchange}},

    {"amenity-bureau_de_change", {kml::BookmarkIcon::Exchange, BookmarkBaseType::Exchange}},

    {"amenity-charging_station", {kml::BookmarkIcon::ChargingStation, BookmarkBaseType::Gas}},
    {"amenity-charging_station-bicycle", {kml::BookmarkIcon::ChargingStation, BookmarkBaseType::Gas}},
    {"amenity-charging_station-motorcar", {kml::BookmarkIcon::ChargingStation, BookmarkBaseType::Gas}},
    {"amenity-fuel", {kml::BookmarkIcon::Gas, BookmarkBaseType::Gas}},

    {"tourism-alpine_hut", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-camp_site", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-chalet", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-guest_house", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-hostel", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-hotel", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-motel", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-resort", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-wilderness_hut", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"tourism-apartment", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},

    {"amenity-place_of_worship-muslim", {kml::BookmarkIcon::Islam, BookmarkBaseType::ReligiousPlace}},

    {"amenity-place_of_worship-jewish", {kml::BookmarkIcon::Judaism, BookmarkBaseType::ReligiousPlace}},

    {"amenity-childcare", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"amenity-clinic", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"amenity-dentist", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"amenity-doctors", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"amenity-hospital", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"emergency-defibrillator", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},

    {"amenity-pharmacy", {kml::BookmarkIcon::Pharmacy, BookmarkBaseType::Medicine}},

    {"natural-bare_rock", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"natural-cave_entrance", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"natural-peak", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"natural-rock", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"natural-volcano", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},

    {"amenity-arts_centre", {kml::BookmarkIcon::Art, BookmarkBaseType::Museum}},
    {"tourism-gallery", {kml::BookmarkIcon::Art, BookmarkBaseType::Museum}},

    {"tourism-museum", {kml::BookmarkIcon::Museum, BookmarkBaseType::Museum}},

    {"boundary-national_park", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"landuse-forest", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"leisure-garden", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"leisure-nature_reserve", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"leisure-park", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},

    {"amenity-bicycle_parking", {kml::BookmarkIcon::BicycleParking, BookmarkBaseType::Parking}},
    {"amenity-bicycle_parking-covered", {kml::BookmarkIcon::BicycleParkingCovered, BookmarkBaseType::Parking}},
    {"amenity-bicycle_rental", {kml::BookmarkIcon::BicycleRental, BookmarkBaseType::Parking}},

    {"amenity-motorcycle_parking", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"amenity-parking", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"highway-services", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"tourism-caravan_site", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"amenity-vending_machine-parking_tickets", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},

    {"amenity-ice_cream", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"amenity-marketplace", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"amenity-vending_machine", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"shop", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},

    {"amenity-place_of_worship", {kml::BookmarkIcon::Sights, BookmarkBaseType::ReligiousPlace}},
    {"historic-archaeological_site", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-boundary_stone", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-castle", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-fort", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-memorial", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-monument", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-ruins", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-ship", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-tomb", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-wayside_cross", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic-wayside_shrine", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"tourism-artwork", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"tourism-attraction", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"waterway-waterfall", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},

    {"tourism-information", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"tourism-information-office", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"tourism-information-visitor_centre", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},

    {"leisure-fitness_centre", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"leisure-skiing", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"leisure-sports_centre-climbing", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"leisure-sports_centre-shooting", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"leisure-sports_centre-yoga", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"sport", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},

    {"leisure-stadium", {kml::BookmarkIcon::Stadium, BookmarkBaseType::Entertainment}},

    {"leisure-sports_centre-swimming", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"leisure-swimming_pool", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"leisure-water_park", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"natural-beach", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"sport-diving", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"sport-scuba_diving", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"sport-swimming", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},

    {"aeroway-aerodrome", {kml::BookmarkIcon::Airport, BookmarkBaseType::None}},
    {"aeroway-aerodrome-international", {kml::BookmarkIcon::Airport, BookmarkBaseType::None}},
    {"aeroway-terminal", {kml::BookmarkIcon::Airport, BookmarkBaseType::None}},

    {"amenity-bus_station", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"amenity-car_sharing", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"amenity-ferry_terminal", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"amenity-taxi", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"building-train_station", {kml::BookmarkIcon::Transport, BookmarkBaseType::Building}},
    {"highway-bus_stop", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"public_transport-platform", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"railway-halt", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"railway-station", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"railway-tram_stop", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},

    {"tourism-viewpoint", {kml::BookmarkIcon::Viewpoint, BookmarkBaseType::Sights}},

    {"amenity-drinking_water", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"amenity-fountain", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"amenity-water_point", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"man_made-water_tap", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"natural-spring", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},

    {"shop-funeral_directors", {kml::BookmarkIcon::None, BookmarkBaseType::None}}};

// Maki icons to Organic Maps BookmarkIcon mapping
// Reference: https://labs.mapbox.com/maki-icons/
// Maps Maki symbol names (from GeoJSON marker-symbol property) to BookmarkIcon and BookmarkBaseType values
std::map<std::string, BookmarkMatchInfo> const kMakiSymbolToBookmarkIcon = {
    // Aerialway & Transport
    {"aerialway", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"airfield", {kml::BookmarkIcon::Airport, BookmarkBaseType::None}},
    {"airport", {kml::BookmarkIcon::Airport, BookmarkBaseType::None}},
    {"bus", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"car", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"car-rental", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"car-repair", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"ferry", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"ferry-JP", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"heliport", {kml::BookmarkIcon::Airport, BookmarkBaseType::None}},
    {"scooter", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"taxi", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"terminal", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},

    // Rail & Transit
    {"rail", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"rail-light", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"rail-metro", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},

    // Commerce & Shopping
    {"alcohol-shop", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"bakery", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"bank", {kml::BookmarkIcon::Bank, BookmarkBaseType::Exchange}},
    {"bank-JP", {kml::BookmarkIcon::Bank, BookmarkBaseType::Exchange}},
    {"bar", {kml::BookmarkIcon::Bar, BookmarkBaseType::Food}},
    {"bbq", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"beer", {kml::BookmarkIcon::Bar, BookmarkBaseType::Food}},
    {"bicycle", {kml::BookmarkIcon::BicycleParking, BookmarkBaseType::Parking}},
    {"bicycle-share", {kml::BookmarkIcon::BicycleParking, BookmarkBaseType::Parking}},
    {"blood-bank", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"cafe", {kml::BookmarkIcon::Cafe, BookmarkBaseType::Food}},
    {"casino", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"clothing-store", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"commercial", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"confectionery", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"convenience", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"dentist", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"doctor", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"fast-food", {kml::BookmarkIcon::FastFood, BookmarkBaseType::Food}},
    {"florist", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"fuel", {kml::BookmarkIcon::Gas, BookmarkBaseType::Gas}},
    {"furniture", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"gaming", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"garden-centre", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"gift", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"grocery", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"hairdresser", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"hardware", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"ice-cream", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"jewelry-store", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"karaoke", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"laundry", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"mobile-phone", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"optician", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"paint", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"pharmacy", {kml::BookmarkIcon::Pharmacy, BookmarkBaseType::Medicine}},
    {"post", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"post-JP", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"restaurant", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"restaurant-bbq", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"restaurant-noodle", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"restaurant-pizza", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"restaurant-seafood", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"restaurant-sushi", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"shoe", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"shop", {kml::BookmarkIcon::Shop, BookmarkBaseType::Shop}},
    {"suitcase", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"teahouse", {kml::BookmarkIcon::Cafe, BookmarkBaseType::Food}},
    {"telephone", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},

    // Sports & Recreation
    {"american-football", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"amusement-park", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"animal-shelter", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},
    {"aquarium", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},
    {"baseball", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"basketball", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"beach", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"bowling-alley", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"campsite", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"cinema", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"cricket", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"dog-park", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},
    {"fitness-centre", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"garden", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"golf", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"horse-riding", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"observation-tower", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"park", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"park-alt1", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"picnic-site", {kml::BookmarkIcon::Food, BookmarkBaseType::Food}},
    {"pitch", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"playground", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"racetrack", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"racetrack-boat", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"racetrack-cycling", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"racetrack-horse", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"skateboard", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"skiing", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"soccer", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"stadium", {kml::BookmarkIcon::Stadium, BookmarkBaseType::Entertainment}},
    {"swimming", {kml::BookmarkIcon::Swim, BookmarkBaseType::Swim}},
    {"table-tennis", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"tennis", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"theatre", {kml::BookmarkIcon::Theatre, BookmarkBaseType::Entertainment}},
    {"volleyball", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},

    // Education & Information
    {"college", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"college-JP", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"information", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"library", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"museum", {kml::BookmarkIcon::Museum, BookmarkBaseType::Museum}},
    {"school", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"school-JP", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},

    // Health & Emergency
    {"charging-station", {kml::BookmarkIcon::ChargingStation, BookmarkBaseType::Gas}},
    {"defibrillator", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"emergency-phone", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"hospital", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"hospital-JP", {kml::BookmarkIcon::Medicine, BookmarkBaseType::Medicine}},
    {"police", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"police-JP", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"veterinary", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},

    // Accommodation
    {"embassy", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"hotel", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},
    {"lodging", {kml::BookmarkIcon::Hotel, BookmarkBaseType::Hotel}},

    // Landmarks & Sights
    {"art-gallery", {kml::BookmarkIcon::Art, BookmarkBaseType::Museum}},
    {"attraction", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"castle", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"castle-JP", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"historic", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"landmark", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"landmark-JP", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"monument", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"monument-JP", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"place-of-worship", {kml::BookmarkIcon::Sights, BookmarkBaseType::ReligiousPlace}},
    {"viewpoint", {kml::BookmarkIcon::Viewpoint, BookmarkBaseType::Sights}},

    // Religious Sites
    {"religious-buddhist", {kml::BookmarkIcon::Buddhism, BookmarkBaseType::ReligiousPlace}},
    {"religious-christian", {kml::BookmarkIcon::Christianity, BookmarkBaseType::ReligiousPlace}},
    {"religious-jewish", {kml::BookmarkIcon::Judaism, BookmarkBaseType::ReligiousPlace}},
    {"religious-muslim", {kml::BookmarkIcon::Islam, BookmarkBaseType::ReligiousPlace}},
    {"religious-shinto", {kml::BookmarkIcon::Sights, BookmarkBaseType::ReligiousPlace}},

    // Natural Features
    {"cave_entrance", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"dam", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"drinking-water", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"farm", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"forest", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"hot-spring", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"hot_spring", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"landuse", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"mountain", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"music", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"natural", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},
    {"peak", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"spring", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"volcano", {kml::BookmarkIcon::Mountain, BookmarkBaseType::Mountain}},
    {"waterfall", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"wetland", {kml::BookmarkIcon::Park, BookmarkBaseType::Park}},

    // Infrastructure & Utilities
    {"barrier", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"bridge", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"building", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"building-alt1", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"caution", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"cemetery", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"cemetery-JP", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"communications-tower", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"construction", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"city", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"cross", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"danger", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"diamond", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"elevator", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"entrance", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"entrance-alt1", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"fence", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"fire-station", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"fire-station-JP", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"gate", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"globe", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"harbor", {kml::BookmarkIcon::Transport, BookmarkBaseType::None}},
    {"highway-rest-area", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"home", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"industry", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"lift-gate", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"lighthouse", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"lighthouse-JP", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"logging", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"marae", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"marker", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"marker-stroked", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"nightclub", {kml::BookmarkIcon::Entertainment, BookmarkBaseType::Entertainment}},
    {"parking", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"parking-garage", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"parking-paid", {kml::BookmarkIcon::Parking, BookmarkBaseType::Parking}},
    {"prison", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"ranger-station", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"recycling", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"residential-community", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"road-accident", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"roadblock", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"rocket", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"shelter", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"slaughterhouse", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"slipway", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"snowmobile", {kml::BookmarkIcon::Sport, BookmarkBaseType::Entertainment}},
    {"square", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"square-stroked", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"star", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"star-stroked", {kml::BookmarkIcon::Sights, BookmarkBaseType::Sights}},
    {"town", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"town-hall", {kml::BookmarkIcon::Building, BookmarkBaseType::Building}},
    {"triangle", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"triangle-stroked", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"tunnel", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"toilet", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"toll", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"village", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"warehouse", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"waste-basket", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"watch", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"water", {kml::BookmarkIcon::Water, BookmarkBaseType::Water}},
    {"watermill", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"wheelchair", {kml::BookmarkIcon::Information, BookmarkBaseType::Sights}},
    {"windmill", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"zoo", {kml::BookmarkIcon::Animals, BookmarkBaseType::Animals}},

    // Shape markers (generic)
    {"circle", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"circle-stroked", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"arrow", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
    {"heart", {kml::BookmarkIcon::None, BookmarkBaseType::None}},
};

void ValidateKmlData(std::unique_ptr<kml::FileData> & data)
{
  if (!data)
    return;

  for (auto & t : data->m_tracksData)
    if (t.m_layers.empty())
      t.m_layers.emplace_back(kml::KmlParser::GetDefaultTrackLayer());
}

/// @todo(KK): This code is a temporary solution for the filtering the duplicated points in KMLs.
/// When the deserealizer reads the data from the KML that uses <gx:Track>
/// as a first step will be parsed the list of the timestamps <when> and than the list of the coordinates <gx:coord>.
/// So the filtering can be done only when all the data is parsed.
void RemoveDuplicatedTrackPoints(std::unique_ptr<kml::FileData> & data)
{
  for (auto & trackData : data->m_tracksData)
  {
    auto const & geometry = trackData.m_geometry;
    kml::MultiGeometry validGeometry;

    for (size_t lineIndex = 0; lineIndex < geometry.m_lines.size(); ++lineIndex)
    {
      auto const & line = geometry.m_lines[lineIndex];
      auto const & timestamps = geometry.m_timestamps[lineIndex];

      if (line.empty())
      {
        LOG(LWARNING, ("Empty line in track:", trackData.m_name[kml::kDefaultLang]));
        continue;
      }

      bool const hasTimestamps = geometry.HasTimestampsFor(lineIndex);
      if (hasTimestamps && timestamps.size() != line.size())
        MYTHROW(kml::DeserializerKml::DeserializeException,
                ("Timestamps count", timestamps.size(), "doesn't match points count", line.size()));

      validGeometry.m_lines.emplace_back();
      validGeometry.m_timestamps.emplace_back();

      auto & validLine = validGeometry.m_lines.back();
      auto & validTimestamps = validGeometry.m_timestamps.back();

      for (size_t pointIndex = 0; pointIndex < line.size(); ++pointIndex)
      {
        auto const & currPoint = line[pointIndex];

        // We don't expect vertical surfaces, so do not compare heights here.
        // Will get a lot of duplicating points otherwise after import some user KMLs.
        // https://github.com/organicmaps/organicmaps/issues/3895
        if (validLine.empty() || !AlmostEqualAbs(validLine.back().GetPoint(), currPoint.GetPoint(), kMwmPointAccuracy))
        {
          validLine.push_back(currPoint);
          if (hasTimestamps)
            validTimestamps.push_back(timestamps[pointIndex]);
        }
      }
    }

    trackData.m_geometry = std::move(validGeometry);
  }
}

bool IsBadCharForPath(strings::UniChar c)
{
  if (c < ' ')
    return true;
  for (strings::UniChar const illegalChar : {':', '/', '\\', '<', '>', '\"', '|', '?', '*'})
    if (illegalChar == c)
      return true;

  return false;
}
}  // namespace

std::string GetBookmarksDirectory()
{
  return base::JoinPath(GetPlatform().SettingsDir(), "bookmarks");
}

std::string GetTrashDirectory()
{
  std::string const trashDir = base::JoinPath(GetPlatform().SettingsDir(), std::string{kTrashDirectoryName});
  if (!Platform::IsFileExistsByFullPath(trashDir) && !Platform::MkDirChecked(trashDir))
    CHECK(false, ("Failed to create .Trash directory."));
  return trashDir;
}

std::string RemoveInvalidSymbols(std::string const & name)
{
  strings::UniString filtered;
  filtered.reserve(name.size());
  auto it = name.begin();
  while (it != name.end())
  {
    auto const c = ::utf8::unchecked::next(it);
    if (!IsBadCharForPath(c))
      filtered.push_back(c);
  }
  return strings::ToUtf8(filtered);
}

// Returns extension with a dot in a lower case.
std::string GetLowercaseFileExt(std::string const & filePath)
{
  return strings::MakeLowerCase(base::GetFileExtension(filePath));
}

std::optional<FileType> GetFileType(std::string const & filePath)
{
  auto const ext = GetLowercaseFileExt(filePath);
  if (ext == kKmlExtension)
    return FileType::Kml;
  else if (ext == kKmzExtension)
    return FileType::Kmz;
  else if (ext == kKmbExtension)
    return FileType::Kmb;
  else if (ext == kGpxExtension)
    return FileType::Gpx;
  else if (ext == kGeoJsonExtension)
    return FileType::GeoJson;
  else if (ext == kJsonExtension)
    return FileType::Json;
  else
    return {};
}

std::string GenerateUniqueFileName(std::string const & path, std::string name, std::string_view ext)
{
  // Remove extension, if file name already contains it.
  if (name.ends_with(ext))
    name.resize(name.size() - ext.size());

  size_t counter = 1;
  std::string suffix, res;
  do
  {
    res = name;
    res = base::JoinPath(path, res.append(suffix).append(ext));
    if (!Platform::IsFileExistsByFullPath(res))
      break;
    suffix = strings::to_string(counter++);
  }
  while (true);

  return res;
}

std::string GenerateValidAndUniqueFilePath(std::string const & fileName, FileType const fileType)
{
  std::string filePath = RemoveInvalidSymbols(fileName);
  if (filePath.empty())
    filePath = kDefaultBookmarksFileName;

  return GenerateUniqueFileName(GetBookmarksDirectory(), std::move(filePath), GetFileTypeExtension(fileType));
}

std::string GenerateValidAndUniqueTrashedFilePath(std::string const & fileName)
{
  std::string extension = base::GetFileExtension(fileName);
  std::string filePath = RemoveInvalidSymbols(fileName);
  if (filePath.empty())
    filePath = kDefaultBookmarksFileName;
  return GenerateUniqueFileName(GetTrashDirectory(), std::move(filePath), extension);
}

std::string const kDefaultBookmarksFileName = "Bookmarks";

// Populate empty category & track names based on file name: assign file name to category name,
// if there is only one unnamed track - assign file name to it, otherwise add numbers 1, 2, 3...
// to file name to build names for all unnamed tracks
void FillEmptyNames(std::unique_ptr<kml::FileData> & kmlData, std::string const & file)
{
  auto start = file.find_last_of('/') + 1;
  auto end = file.find_last_of('.');
  if (end == std::string::npos)
    end = file.size();
  auto const name = file.substr(start, end - start);

  if (kmlData->m_categoryData.m_name.empty())
    kmlData->m_categoryData.m_name[kml::kDefaultLang] = name;

  if (kmlData->m_tracksData.empty())
    return;

  auto const emptyNames = std::count_if(kmlData->m_tracksData.begin(), kmlData->m_tracksData.end(),
                                        [](kml::TrackData const & t) { return t.m_name.empty(); });
  if (emptyNames == 0)
    return;

  auto emptyTrackNum = 1;
  for (auto & track : kmlData->m_tracksData)
  {
    if (track.m_name.empty())
    {
      if (emptyNames == 1)
      {
        track.m_name[kml::kDefaultLang] = name;
        return;
      }
      else
      {
        track.m_name[kml::kDefaultLang] = name + " " + std::to_string(emptyTrackNum);
        emptyTrackNum++;
      }
    }
  }
}

std::unique_ptr<kml::FileData> LoadKmlFile(std::string const & file, FileType fileType)
{
  std::unique_ptr<kml::FileData> kmlData;
  try
  {
    kmlData = LoadKmlData(FileReader(file), fileType);
    if (kmlData != nullptr)
      FillEmptyNames(kmlData, file);
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "loading failure:", e.what()));
    kmlData.reset();
  }
  if (kmlData == nullptr)
    LOG(LWARNING, ("Loading bookmarks failed, file", file));
  return kmlData;
}

static std::vector<std::string> GetFilePathsToLoadFromKmz(std::string const & filePath)
{
  // Extract KML files from KMZ archive and save to temp KMLs with unique name.
  std::vector<std::string> kmlFilePaths;
  try
  {
    ZipFileReader::FileList files;
    ZipFileReader::FilesList(filePath, files);
    files.erase(std::remove_if(files.begin(), files.end(),
                               [](auto const & file) { return GetLowercaseFileExt(file.first) != kKmlExtension; }),
                files.end());
    for (auto const & [kmlFileInZip, size] : files)
    {
      auto const name = base::FileNameFromFullPath(kmlFileInZip);
      auto fileSavePath = GenerateValidAndUniqueFilePath(kmlFileInZip, FileType::Kml);
      ZipFileReader::UnzipFile(filePath, kmlFileInZip, fileSavePath);
      kmlFilePaths.push_back(std::move(fileSavePath));
    }
  }
  catch (RootException const & e)
  {
    LOG(LWARNING, ("Error unzipping file", filePath, e.Msg()));
  }
  return kmlFilePaths;
}

static std::vector<std::string> GetFilePathsToLoadFromKmb(std::string const & filePath)
{
  // Convert input KMB file and save to temp KML with unique name.
  auto kmlData = LoadKmlFile(filePath, FileType::Kmb);
  if (kmlData == nullptr)
    return {};

  auto fileSavePath = GenerateValidAndUniqueFilePath(base::FileNameFromFullPath(filePath), FileType::Kml);
  if (!SaveKmlFileByExt(*kmlData, fileSavePath))
    return {};
  return {std::move(fileSavePath)};
}

static std::vector<std::string> GetFilePathsToLoadByType(std::string const & filePath, FileType const fileType)
{
  // Copy input file to temp file with unique name.
  auto fileSavePath = GenerateValidAndUniqueFilePath(base::FileNameFromFullPath(filePath), fileType);
  if (!base::CopyFileX(filePath, fileSavePath))
    return {};
  return {std::move(fileSavePath)};
}

std::vector<std::string> GetKMLOrGPXFilesPathsToLoad(std::string const & filePath)
{
  // Copy or convert file from 'filePath' to temp folder.
  // KMZ archives are unpacked to temp folder.
  if (auto const fileType = GetFileType(filePath))
  {
    switch (*fileType)
    {
    case FileType::Kml:
    case FileType::Gpx:
    case FileType::GeoJson:
    case FileType::Json: return GetFilePathsToLoadByType(filePath, *fileType);
    case FileType::Kmz: return GetFilePathsToLoadFromKmz(filePath);
    case FileType::Kmb: return GetFilePathsToLoadFromKmb(filePath);
    }
    UNREACHABLE();
  }
  else
  {
    LOG(LWARNING, ("Unknown file type", filePath));
    return {};
  }
}

std::unique_ptr<kml::FileData> LoadKmlData(Reader const & reader, FileType fileType)
{
  auto data = std::make_unique<kml::FileData>();
  try
  {
    if (fileType == FileType::Kmb)
    {
      kml::binary::DeserializerKml des(*data);
      des.Deserialize(reader);
    }
    else if (fileType == FileType::Kml)
    {
      kml::DeserializerKml des(*data);
      des.Deserialize(reader);
    }
    else if (fileType == FileType::Gpx)
    {
      kml::DeserializerGpx des(*data);
      des.Deserialize(reader);
    }
    else if (fileType == FileType::GeoJson || fileType == FileType::Json)
    {
      kml::GeoJsonReader des(*data);
      std::string content;
      reader.ReadAsString(content);

      std::string_view content_view(content);
      des.Deserialize(content_view);
    }
    else
    {
      CHECK(false, ("Not supported FileType"));
    }
    ValidateKmlData(data);
    RemoveDuplicatedTrackPoints(data);
  }
  catch (Reader::Exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "reading failure:", e.what()));
    return nullptr;
  }
  catch (kml::binary::DeserializerKml::DeserializeException const & e)
  {
    LOG(LWARNING, ("KML", fileType, "deserialization failure:", e.what()));
    return nullptr;
  }
  catch (kml::DeserializerKml::DeserializeException const & e)
  {
    LOG(LWARNING, ("KML", fileType, "deserialization failure:", e.what()));
    return nullptr;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "loading failure:", e.what()));
    return nullptr;
  }
  return data;
}

static bool SaveGpxData(kml::FileData & kmlData, Writer & writer)
{
  try
  {
    kml::gpx::SerializerGpx ser(kmlData);
    ser.Serialize(writer);
  }
  catch (Writer::Exception const & e)
  {
    LOG(LWARNING, ("GPX writing failure:", e.what()));
    return false;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("GPX serialization failure:", e.what()));
    return false;
  }
  return true;
}

static bool SaveGeoJsonData(kml::FileData const & kmlData, Writer & writer)
{
  try
  {
    kml::GeoJsonWriter exporter(writer);
    exporter.Write(kmlData, false);
  }
  catch (Writer::Exception const & e)
  {
    LOG(LWARNING, ("GeoJSON writing failure:", e.what()));
    return false;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("GeoJSON serialization failure:", e.what()));
    return false;
  }
  return true;
}

static bool SaveKmlFile(kml::FileData & kmlData, std::string const & file, FileType fileType)
{
  FileWriter writer(file);
  switch (fileType)
  {
  case FileType::Kml:  // fallthrough
  case FileType::Kmb: return SaveKmlData(kmlData, writer, fileType);
  case FileType::Gpx: return SaveGpxData(kmlData, writer);
  case FileType::GeoJson:  // fallthrough
  case FileType::Json: return SaveGeoJsonData(kmlData, writer);
  default:
  {
    LOG(LWARNING, ("Unexpected FileType", fileType));
    return false;
  }
  }
}

bool SaveKmlFileSafe(kml::FileData & kmlData, std::string const & file, FileType fileType)
{
  LOG(LINFO, ("Save kml file of type", fileType, "to", file));
  return base::WriteToTempAndRenameToFile(
      file, [&kmlData, fileType](std::string const & fileName) { return SaveKmlFile(kmlData, fileName, fileType); });
}

bool SaveKmlFileByExt(kml::FileData & kmlData, std::string const & file)
{
  auto const ext = base::GetFileExtension(file);
  return SaveKmlFileSafe(kmlData, file, ext == kKmbExtension ? FileType::Kmb : FileType::Kml);
}

bool SaveKmlData(kml::FileData & kmlData, Writer & writer, FileType fileType)
{
  try
  {
    if (fileType == FileType::Kmb)
    {
      kml::binary::SerializerKml ser(kmlData);
      ser.Serialize(writer);
    }
    else if (fileType == FileType::Kml)
    {
      kml::SerializerKml ser(kmlData);
      ser.Serialize(writer);
    }
    else
    {
      CHECK(false, ("Not supported FileType"));
    }
  }
  catch (Writer::Exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "writing failure:", e.what()));
    return false;
  }
  catch (std::exception const & e)
  {
    LOG(LWARNING, ("KML", fileType, "serialization failure:", e.what()));
    return false;
  }
  return true;
}

void ResetIds(kml::FileData & kmlData)
{
  kmlData.m_categoryData.m_id = kml::kInvalidMarkGroupId;
  for (auto & bmData : kmlData.m_bookmarksData)
    bmData.m_id = kml::kInvalidMarkId;
  for (auto & trackData : kmlData.m_tracksData)
    trackData.m_id = kml::kInvalidTrackId;
  for (auto & compilationData : kmlData.m_compilationsData)
    compilationData.m_id = kml::kInvalidMarkGroupId;
}

static bool TruncType(std::string & type)
{
  auto const pos = type.rfind('-');
  if (pos == std::string::npos)
    return false;
  type.resize(pos);
  return true;
}

BookmarkBaseType GetBookmarkBaseType(std::vector<uint32_t> const & featureTypes)
{
  auto const & c = classif();
  for (auto typeIndex : featureTypes)
  {
    auto const type = c.GetTypeForIndex(typeIndex);
    auto typeStr = c.GetReadableObjectName(type);

    do
    {
      auto const itType = kFeatureTypeToBookmarkMatchInfo.find(typeStr);
      if (itType != kFeatureTypeToBookmarkMatchInfo.cend())
        return itType->second.m_type;
    }
    while (TruncType(typeStr));
  }
  return BookmarkBaseType::None;
}

kml::BookmarkIcon GetBookmarkIconByFeatureType(uint32_t type)
{
  auto typeStr = classif().GetReadableObjectName(type);
  do
  {
    auto const itIcon = kFeatureTypeToBookmarkMatchInfo.find(typeStr);
    if (itIcon != kFeatureTypeToBookmarkMatchInfo.cend())
      return itIcon->second.m_icon;
  }
  while (TruncType(typeStr));

  return kml::BookmarkIcon::None;
}

kml::BookmarkIcon GetBookmarkIconByMakiSymbol(std::string const & makiSymbol)
{
  std::string symbol = makiSymbol;
  do
  {
    auto const itIcon = kMakiSymbolToBookmarkIcon.find(symbol);
    if (itIcon != kMakiSymbolToBookmarkIcon.cend())
      return itIcon->second.m_icon;
  }
  while (TruncType(symbol));

  return kml::BookmarkIcon::None;
}

void SaveFeatureTypes(feature::TypesHolder const & types, kml::BookmarkData & bmData)
{
  auto const & c = classif();
  feature::TypesHolder copy(types);
  copy.SortBySpec();
  bmData.m_featureTypes.reserve(copy.Size());
  for (auto it = copy.begin(); it != copy.end(); ++it)
  {
    if (c.IsTypeValid(*it))
    {
      bmData.m_featureTypes.push_back(c.GetIndexForType(*it));
      if (bmData.m_icon == kml::BookmarkIcon::None)
        bmData.m_icon = GetBookmarkIconByFeatureType(*it);
    }
  }
}

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name)
{
  auto const mapLanguageNorm = languages::Normalize(languages::GetCurrentMapLanguage());
  return kml::GetPreferredBookmarkStr(name, mapLanguageNorm);
}

std::string GetPreferredBookmarkStr(kml::LocalizableString const & name, feature::RegionData const & regionData)
{
  auto const mapLanguageNorm = languages::Normalize(languages::GetCurrentMapLanguage());
  return kml::GetPreferredBookmarkStr(name, regionData, mapLanguageNorm);
}

std::string GetLocalizedFeatureType(std::vector<uint32_t> const & types)
{
  return kml::GetLocalizedFeatureType(types);
}

std::string GetLocalizedBookmarkBaseType(BookmarkBaseType type)
{
  switch (type)
  {
  case BookmarkBaseType::None: return {};
  case BookmarkBaseType::Hotel: return platform::GetLocalizedString("hotels");
  case BookmarkBaseType::Animals: return platform::GetLocalizedString("animals");
  case BookmarkBaseType::Building: return platform::GetLocalizedString("buildings");
  case BookmarkBaseType::Entertainment: return platform::GetLocalizedString("entertainment");
  case BookmarkBaseType::Exchange: return platform::GetLocalizedString("money");
  case BookmarkBaseType::Food: return platform::GetLocalizedString("food_places");
  case BookmarkBaseType::Gas: return platform::GetLocalizedString("fuel_places");
  case BookmarkBaseType::Medicine: return platform::GetLocalizedString("medicine");
  case BookmarkBaseType::Mountain: return platform::GetLocalizedString("mountains");
  case BookmarkBaseType::Museum: return platform::GetLocalizedString("museums");
  case BookmarkBaseType::Park: return platform::GetLocalizedString("parks");
  case BookmarkBaseType::Parking: return platform::GetLocalizedString("parkings");
  case BookmarkBaseType::ReligiousPlace: return platform::GetLocalizedString("religious_places");
  case BookmarkBaseType::Shop: return platform::GetLocalizedString("shops");
  case BookmarkBaseType::Sights: return platform::GetLocalizedString("tourist_places");
  case BookmarkBaseType::Swim: return platform::GetLocalizedString("swim_places");
  case BookmarkBaseType::Water: return platform::GetLocalizedString("water");
  case BookmarkBaseType::Count: CHECK(false, ("Invalid bookmark base type")); return {};
  }
  UNREACHABLE();
}

std::string GetPreferredBookmarkName(kml::BookmarkData const & bmData)
{
  return kml::GetPreferredBookmarkName(bmData, languages::GetCurrentMapLanguage());
}

void ExpandRectForPreview(m2::RectD & rect)
{
  if (!rect.IsValid())
    return;

  rect.Scale(df::kBoundingBoxScale);
}
