#include "geocoder/geocoder.hpp"
#include "geocoder/result.hpp"

#include "base/string_utils.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "3party/gflags/src/gflags/gflags.h"

using namespace geocoder;
using namespace std;

DEFINE_string(hierarchy_path, "", "Path to the hierarchy file for the geocoder");
DEFINE_string(queries_path, "", "Path to the file with queries");
DEFINE_int32(top, 5, "Number of top results to show for every query, -1 to show all results");

void PrintResults(vector<Result> const & results)
{
  cout << "Found results: " << results.size() << endl;
  if (results.empty())
    return;
  cout << "Top results:" << endl;
  for (size_t i = 0; i < results.size(); ++i)
  {
    if (FLAGS_top >= 0 && i >= FLAGS_top)
      break;
    cout << "  " << DebugPrint(results[i]) << endl;
  }
}

void ProcessQueriesFromFile(string const & path)
{
  ifstream stream(path.c_str());
  CHECK(stream.is_open(), ("Can't open", path));

  Geocoder geocoder(FLAGS_hierarchy_path);

  vector<Result> results;
  string s;
  while (getline(stream, s))
  {
    strings::Trim(s);
    if (s.empty())
      continue;

    cout << s << endl;
    geocoder.ProcessQuery(s, results);
    PrintResults(results);
    cout << endl;
  }
}

void ProcessQueriesFromCommandLine()
{
  Geocoder geocoder(FLAGS_hierarchy_path);

  string query;
  vector<Result> results;
  while (true)
  {
    cout << "> ";
    if (!getline(cin, query))
      break;
    if (query == "q" || query == ":q" || query == "quit")
      break;
    geocoder.ProcessQuery(query, results);
    PrintResults(results);
  }
}

int main(int argc, char * argv[])
{
  ios_base::sync_with_stdio(false);

  google::SetUsageMessage("Geocoder command line interface.");
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (!FLAGS_queries_path.empty())
  {
    ProcessQueriesFromFile(FLAGS_queries_path);
    return 0;
  }

  ProcessQueriesFromCommandLine();
  return 0;
}
