#include "../../testing/testing.hpp"

#include "../country.hpp"

#include "../../coding/file_writer.hpp"
#include "../../coding/file_reader.hpp"

#include "../../base/start_mem_debug.hpp"

using namespace storage;

UNIT_TEST(CountrySerialization)
{
//  string const TEST_URL = "http://someurl.com/somemap.dat";
//  uint64_t const TEST_SIZE = 123456790;
//  char const * TEST_FILE_NAME = "some_temporary_update_file.tmp";
//  Country c("North America", "USA", "Alaska");
//  c.AddUrl(TUrl(TEST_URL, TEST_SIZE));

//  {
//    TCountriesContainer countries;
//    countries[c.Group()].push_back(c);

//    FileWriter writer(TEST_FILE_NAME);
//    SaveCountries(countries, writer);
//  }

//  TCountriesContainer loadedCountries;
//  {
//    TEST( LoadCountries(loadedCountries, TEST_FILE_NAME), ());
//  }

//  TEST_GREATER(loadedCountries.size(), 0, ());
//  Country const & c2 = loadedCountries.begin()->second.front();
//  TEST_EQUAL(c.Group(), loadedCountries.begin()->first, ());
//  TEST_EQUAL(c.Group(), c2.Group(), ());
//  TEST_EQUAL(c.Name(), c2.Name(), ());
//  TEST_GREATER(c2.Urls().size(), 0, ());
//  TEST_EQUAL(*c.Urls().begin(), *c2.Urls().begin(), ());

//  FileWriter::DeleteFile(TEST_FILE_NAME);
}
