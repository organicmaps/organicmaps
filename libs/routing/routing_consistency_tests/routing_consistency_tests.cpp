#include "routing/routing_integration_tests/routing_test_tools.hpp"
#include "testing/testing.hpp"

#include "generator/borders.hpp"

#include "storage/country_decl.hpp"

#include "geometry/mercator.hpp"

#include "geometry/point2d.hpp"

#include "base/logging.hpp"

#include <fstream>
#include <iostream>
#include <map>
#include <string>

#include <gflags/gflags.h>

namespace routing_consistency_tests
{
using namespace routing;
using namespace std;
using storage::CountryInfo;

double constexpr kMinimumRouteDistanceM = 10000.;
double constexpr kRouteLengthAccuracy = 0.15;

// Testing stub to make routing test tools linkable.
static CommandLineOptions g_options;
CommandLineOptions const & GetTestingOptions()
{
  return g_options;
}

DEFINE_string(input_file, "", "File with statistics output.");
DEFINE_string(data_path, "../../data/", "Working directory, 'path_to_exe/../../data' if empty.");
DEFINE_string(user_resource_path, "", "User defined resource path for classificator.txt and etc.");
DEFINE_bool(verbose, false, "Output processed lines to log.");
DEFINE_uint64(confidence, 5, "Maximum test count for each single mwm file.");

// Information about successful user routing.
struct UserRoutingRecord
{
  m2::PointD start;
  m2::PointD stop;
  double distance;
};

// Parsing value from statistics.
double GetDouble(string const & incomingString, string const & key)
{
  auto it = incomingString.find(key);
  if (it == string::npos)
    return 0;
  // Skip "key="
  it += key.size() + 1;
  auto end = incomingString.find(" ", it);
  string number = incomingString.substr(it, end - it);
  return stod(number);
}

// Decoding statistics line. Returns true if the incomeString is the valid record about
// OSRM routing attempt.
bool ParseUserString(string const & incomeString, UserRoutingRecord & result)
{
  // Check if it is a proper routing record.
  if (incomeString.find("Routing_CalculatingRoute") == string::npos)
    return false;
  if (incomeString.find("result=NoError") == string::npos)
    return false;
  if (incomeString.find("name=vehicle") == string::npos)
    return false;
  if (GetDouble(incomeString, "startDirectionX") != 0 && GetDouble(incomeString, "startDirectionY") != 0)
    return false;

  // Extract numbers from a record.
  result.distance = GetDouble(incomeString, "distance");
  result.start = mercator::FromLatLon(GetDouble(incomeString, "startLat"), GetDouble(incomeString, "startLon"));
  result.stop = mercator::FromLatLon(GetDouble(incomeString, "finalLat"), GetDouble(incomeString, "finalLon"));
  return true;
}

class RouteTester
{
public:
  RouteTester() : m_components(integration::GetVehicleComponents(VehicleType::Car)) {}

  bool BuildRoute(UserRoutingRecord const & record)
  {
    m_components.GetRouter().ClearState();
    auto const result = integration::CalculateRoute(m_components, record.start, m2::PointD::Zero(), record.stop);
    if (result.second != RouterResultCode::NoError)
    {
      LOG(LINFO, ("Can't build the route. Code:", result.second));
      return false;
    }
    auto const delta = record.distance * kRouteLengthAccuracy;
    auto const routeLength = result.first->GetTotalDistanceMeters();
    if (abs(routeLength - record.distance) < delta)
      return true;

    LOG(LINFO, ("Route has invalid length. Expected:", record.distance, "have:", routeLength));
    return false;
  }

  bool CheckRecord(UserRoutingRecord const & record)
  {
    CountryInfo startCountry, finishCountry;
    m_components.GetCountryInfoGetter().GetRegionInfo(record.start, startCountry);
    m_components.GetCountryInfoGetter().GetRegionInfo(record.stop, finishCountry);
    if (startCountry.m_name != finishCountry.m_name || startCountry.m_name.empty())
      return false;

    if (record.distance < kMinimumRouteDistanceM)
      return false;

    if (m_checkedCountries[startCountry.m_name] > FLAGS_confidence)
      return false;

    return true;
  }

  bool BuildRouteByRecord(UserRoutingRecord const & record)
  {
    CountryInfo startCountry;
    m_components.GetCountryInfoGetter().GetRegionInfo(record.start, startCountry);
    m_checkedCountries[startCountry.m_name] += 1;
    if (!BuildRoute(record))
    {
      m_errors[startCountry.m_name] += 1;
      return false;
    }
    return true;
  }

  void PrintStatistics()
  {
    LOG(LINFO, ("Checked", m_checkedCountries.size(), "countries."));
    LOG(LINFO, ("Found", m_errors.size(), "maps with errors."));
    for (auto const & record : m_errors)
      if (record.second == m_checkedCountries[record.first])
        LOG(LINFO, ("ERROR!", record.first, " seems to be broken!"));
      else
        LOG(LINFO, ("Warning! Country:", record.first, "has", record.second, "errors on",
                    m_checkedCountries[record.first], "checks"));
  }

private:
  integration::IRouterComponents & m_components;

  map<string, size_t> m_checkedCountries;
  map<string, size_t> m_errors;
};

void ReadInput(istream & stream, RouteTester & tester)
{
  string line;
  while (stream.good())
  {
    getline(stream, line);
    strings::Trim(line);
    if (line.empty())
      continue;
    UserRoutingRecord record;
    if (!ParseUserString(line, record))
      continue;
    if (tester.CheckRecord(record))
    {
      if (FLAGS_verbose)
        LOG(LINFO, ("Checked", line));
      tester.BuildRouteByRecord(record);
    }
  }
  tester.PrintStatistics();
}

int RunRoutingConsistencyTests(int argc, char ** argv)
{
  gflags::SetUsageMessage("Check mwm and routing files consistency. Calculating roads from a user statistics.");

  gflags::ParseCommandLineFlags(&argc, &argv, true);

  g_options.m_dataPath = FLAGS_data_path.c_str();
  g_options.m_resourcePath = FLAGS_user_resource_path.c_str();
  if (FLAGS_input_file.empty())
    return 1;

  RouteTester tester;
  ifstream stream(FLAGS_input_file);
  ReadInput(stream, tester);

  return 0;
}
}  // namespace routing_consistency_tests

int main(int argc, char ** argv)
{
  return ::routing_consistency_tests::RunRoutingConsistencyTests(argc, argv);
}
