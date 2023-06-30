#include "testing/testing.hpp"

#include "generator/sponsored_scoring.hpp"

#include "geometry/distance_on_sphere.hpp"
#include "geometry/latlon.hpp"

namespace sponsored_scoring_tests
{

generator::sponsored::MatchStats GetMatch(ms::LatLon osmLL, std::string const & osmName,
                                          ms::LatLon hotelLL, std::string const & hotelName)
{
  // The same as SponsoredDataset::kDistanceLimitMeters
  return { ms::DistanceOnEarth(osmLL, hotelLL), 150.0, hotelName, osmName };
}

UNIT_TEST(SponsoredScoring_Paris)
{
  TEST(!GetMatch({48.8474633, 2.3712106}, "Hôtel de Marseille",
                 {48.8473730, 2.3712020}, "Hotel Riesner").IsMatched(), ());

  TEST(GetMatch({48.8760697, 2.3456749}, "Holiday Villa",
                {48.8761570, 2.3455750}, "Hotel Villa Lafayette Paris IX").IsMatched(), ());

  TEST(GetMatch({48.8664199, 2.2892440}, "Hôtel Baltimore",
                {48.8663780, 2.2895710}, "Sofitel Paris Baltimore Tour Eiffel").IsMatched(), ());

  TEST(!GetMatch({48.8808205, 2.3517253}, "Grand Hotel Magenta",
                 {48.8806950, 2.3521320}, "Hotel Cambrai").IsMatched(), ());

  // But may be false on the ground.
  TEST(GetMatch({48.8733283, 2.3004615}, "Hôtel Balzac",
                {48.8735222, 2.3004904}, "Apart Inn Paris - Balzac").IsMatched(), ());

  TEST(!GetMatch({48.8470895, 2.3710844}, "Hôtel Mignon",
                 {48.8473730, 2.3712020}, "Hotel Riesner").IsMatched(), ());

}

} // namespace sponsored_scoring_tests
