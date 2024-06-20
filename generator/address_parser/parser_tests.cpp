#include "testing/testing.hpp"

#include "processor.hpp"
#include "tiger_parser.hpp"

#include "generator/generator_tests_support/test_generator.hpp"

#include <fstream>

namespace addr_parser_tests
{

UNIT_TEST(TigerParser_Smoke)
{
  double constexpr kEpsLL = 1.0E-6;

  tiger::AddressEntry e;
  TEST(!tiger::ParseLine("from;to;interpolation;street;city;state;postcode;geometry", e), ());

  e = {};
  TEST(tiger::ParseLine("9587;9619;all;;Platte;MO;64152;LINESTRING(-94.691917 39.210393,-94.692370 39.210351)", e), ());

  e = {};
  TEST(tiger::ParseLine("698;600;all;Boston St;Berkeley;WV;25401;LINESTRING(-77.970484 39.464604,-77.970540 39.464630)", e), ());
  TEST_EQUAL(e.m_from, "600", ());
  TEST_EQUAL(e.m_to, "698", ());
  TEST_EQUAL(e.m_street, "Boston St", ());
  TEST_EQUAL(e.m_postcode, "25401", ());
  TEST_EQUAL(e.m_interpol, feature::InterpolType::Any, ());
  TEST_EQUAL(e.m_geom.size(), 1, ());

  TEST(ms::LatLon((39.464604 + 39.464630) / 2.0, (-77.970484 -77.970540) / 2.0).EqualDxDy(e.m_geom.back(), kEpsLL), ());

  e = {};
  TEST(tiger::ParseLine("798;700;all;Boston St;Berkeley;WV;25401;LINESTRING(-77.968929 39.463906,-77.969118 39.463990,-77.969427 39.464129,-77.969946 39.464353,-77.970027 39.464389)", e), ());
  TEST_EQUAL(e.m_from, "700", ());
  TEST_EQUAL(e.m_to, "798", ());
  TEST_EQUAL(e.m_street, "Boston St", ());
  TEST_EQUAL(e.m_postcode, "25401", ());
  TEST_EQUAL(e.m_interpol, feature::InterpolType::Any, ());
  TEST_EQUAL(e.m_geom.size(), 4, ());

  TEST(ms::LatLon(39.463906, -77.968929).EqualDxDy(e.m_geom.back(), kEpsLL), ());
  TEST(ms::LatLon(39.464389, -77.970027).EqualDxDy(e.m_geom.front(), kEpsLL), ());

  TEST(tiger::ParseLine("0;98;even;Austin Ln;Mifflin;PA;17044;LINESTRING(-77.634119 40.597239,-77.634200 40.597288,-77.634679 40.598169,-77.634835 40.598393,-77.635116 40.598738,-77.635518 40.599388,-77.635718 40.599719,-77.635833 40.599871,-77.635856 40.599920)", e), ());
  TEST_EQUAL(e.m_from, "0", ());
  TEST_EQUAL(e.m_to, "98", ());
}

UNIT_TEST(Processor_Smoke)
{
  std::string const kTestFileName = "temp.txt";
  std::string const outPath = "./addrs";
  CHECK(Platform::MkDirChecked(outPath), ());

  {
    std::ofstream of(kTestFileName);
    of << "698;600;all;Boston St;Berkeley;WV;25401;LINESTRING(-77.970484 39.464604,-77.970540 39.464630)" << std::endl;
    of << "901;975;all;Pease Rd;Sutter;CA;95991;LINESTRING(-121.623833 39.171033,-121.626563 39.171015)" << std::endl;
    of << "7677;7601;all;N Main St;Livingston;NY;14560;LINESTRING(-77.595115 42.645859,-77.595450 42.648883)" << std::endl;
    of << "1;99;all;Stokes Dr;Pend Oreille;WA;99156;LINESTRING(-117.209595 48.058395,-117.209584 48.058352)" << std::endl;
  }

  addr_generator::Processor processor("./data" /* dataPath */, outPath, 2 /* numThreads */);

  {
    std::ifstream ifile(kTestFileName);
    processor.Run(ifile);
  }

  Platform::FilesList files;
  Platform::GetFilesByExt(outPath, TEMP_ADDR_EXTENSION, files);
  TEST_EQUAL(files.size(), 4, ());

  CHECK(base::DeleteFileX(kTestFileName), ());
  CHECK(Platform::RmDirRecursively(outPath), ());
}

using TestRawGenerator = generator::tests_support::TestRawGenerator;

UNIT_CLASS_TEST(TestRawGenerator, Processor_Generator)
{
  std::string const outPath = "./addrs";
  CHECK(Platform::MkDirChecked(outPath), ());

  // Prepare tmp address files.
  std::string mwmName;
  {
    std::stringstream ss;
    ss << "698;600;all;Boston St;Berkeley;WV;25401;LINESTRING(-77.970484 39.464604,-77.970540 39.464630)" << std::endl
       << "798;700;all;Boston St;Berkeley;WV;25401;LINESTRING(-77.968929 39.463906,-77.969118 39.463990,-77.969427 39.464129,-77.969946 39.464353,-77.970027 39.464389)" << std::endl;

    addr_generator::Processor processor("./data" /* dataPath */, outPath, 1 /* numThreads */);
    processor.Run(ss);

    Platform::FilesList files;
    Platform::GetFilesByExt(outPath, TEMP_ADDR_EXTENSION, files);
    TEST_EQUAL(files.size(), 1, ());

    mwmName = std::move(files.front());
    base::GetNameWithoutExt(mwmName);
  }

  // Build mwm.
  GetGenInfo().m_addressesDir = outPath;

  uint32_t const addrInterpolType = classif().GetTypeByPath({"addr:interpolation"});
  uint32_t const addrNodeType = classif().GetTypeByPath({"building", "address"});

  BuildFB("./data/osm_test_data/us_tiger_1.osm", mwmName, false /* makeWorld */);

  size_t addrInterpolCount = 0, addrNodeCount = 0;
  ForEachFB(mwmName, [&](feature::FeatureBuilder const & fb)
  {
    if (fb.HasType(addrInterpolType))
      ++addrInterpolCount;
    if (fb.HasType(addrNodeType))
      ++addrNodeCount;
  });

  TEST_EQUAL(addrInterpolCount, 1, ());
  TEST_EQUAL(addrNodeCount, 3, ());

  BuildFeatures(mwmName);

  // Cleanup.
  CHECK(Platform::RmDirRecursively(outPath), ());
}

} // namespace addr_parser_tests
