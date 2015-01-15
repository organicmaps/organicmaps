#pragma once

#include "../std/string.hpp"
#include "../std/vector.hpp"

#include "../std/set.hpp"
namespace routing
{


/*!
 * \brief The OutgoungNodeT struct. Contains node identifier and it's coordinate to calculate same nodes at different mwm's
 */
/*struct OutgoungNodeT {
  size_t node_id;
  double x;
  double y;
};*/
typedef size_t OutgoingNodeT;

typedef std::vector<OutgoingNodeT> OutgoingVectorT;

/// @param[in]  baseDir   Full path to .mwm files directory.
/// @param[in]  countryName   Country name same with .mwm and .border file name.
/// @param[in]  osrmFile  Full path to .osrm file (all prepared osrm files should be there).
void BuildRoutingIndex(string const & baseDir, string const & countryName, string const & osrmFile);

/// @param[in]  baseDir   Full path to .mwm files directory.
void BuildCrossesRoutingIndex(string const & baseDir);
}
