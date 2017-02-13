#include "testing/testing.hpp"

#include "openlr/openlr_model.hpp"
#include "openlr/openlr_sample.hpp"
#include "openlr/openlr_simple_parser.hpp"

#include "indexer/index.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "3party/pugixml/src/pugixml.hpp"

using namespace openlr;

UNIT_TEST(ParseOpenlr)
{
  auto const openlrData = "<?xml version=\"1.0\"?>"
      "    <Report dictionaryUpdateDateTime=\"2016-09-15T03:40:51\" dictionaryVersion=\"16.2\">"
      "      <reportSegments>"
      "        <ReportSegmentID>8446643</ReportSegmentID>"
      "        <ReportSegmentLRC>"
      "          <method>"
      "            <olr:version>1.0</olr:version>"
      "            <olr:locationReference>"
      "              <olr:optionLinearLocationReference>"
      "                <olr:first>"
      "                  <olr:coordinate>"
      "                    <olr:longitude>1738792</olr:longitude>"
      "                    <olr:latitude>2577486</olr:latitude>"
      "                  </olr:coordinate>"
      "                  <olr:lineProperties>"
      "                    <olr:frc olr:table=\"olr001_FunctionalRoadClass\" olr:code=\"6\"/>"
      "                    <olr:fow olr:table=\"olr002_FormOfWay\" olr:code=\"2\"/>"
      "                    <olr:bearing>"
      "                      <olr:value>8</olr:value>"
      "                    </olr:bearing>"
      "                  </olr:lineProperties>"
      "                  <olr:pathProperties>"
      "                    <olr:lfrcnp olr:table=\"olr001_FunctionalRoadClass\" olr:code=\"7\"/>"
      "                    <olr:dnp>"
      "                      <olr:value>3572</olr:value>"
      "                    </olr:dnp>"
      "                    <olr:againstDrivingDirection>true</olr:againstDrivingDirection>"
      "                  </olr:pathProperties>"
      "                </olr:first>"
      "                <olr:last>"
      "                  <olr:coordinate>"
      "                    <olr:longitude>-1511</olr:longitude>"
      "                    <olr:latitude>2858</olr:latitude>"
      "                  </olr:coordinate>"
      "                  <olr:lineProperties>"
      "                    <olr:frc olr:table=\"olr001_FunctionalRoadClass\" olr:code=\"7\"/>"
      "                    <olr:fow olr:table=\"olr002_FormOfWay\" olr:code=\"3\"/>"
      "                    <olr:bearing>"
      "                      <olr:value>105</olr:value>"
      "                    </olr:bearing>"
      "                  </olr:lineProperties>"
      "                </olr:last>"
      "                <olr:positiveOffset>"
      "                  <olr:value>1637</olr:value>"
      "                </olr:positiveOffset>"
      "                <olr:negativeOffset>"
      "                  <olr:value>919</olr:value>"
      "                </olr:negativeOffset>"
      "              </olr:optionLinearLocationReference>"
      "            </olr:locationReference>"
      "          </method>"
      "        </ReportSegmentLRC>"
      "        <LinearConnectivity>"
      "          <negLink>"
      "            <ReportSegmentID>8446642</ReportSegmentID>"
      "          </negLink>"
      "          <posLink>"
      "            <ReportSegmentID>91840286</ReportSegmentID>"
      "          </posLink>"
      "        </LinearConnectivity>"
      "        <segmentLength>1018</segmentLength>"
      "        <segmentRefSpeed>0</segmentRefSpeed>"
      "     </reportSegments>";

  vector<openlr::LinearSegment> segments;
  pugi::xml_document doc;
  TEST_EQUAL(doc.load(openlrData), pugi::xml_parse_status::status_ok, ());
  TEST(openlr::ParseOpenlr(doc, segments), ());

  TEST_EQUAL(segments.size(), 1, ());

  auto const & segment = segments.front();
  TEST_EQUAL(segment.m_segmentId, 8446643, ());
  TEST_EQUAL(segment.m_segmentLengthMeters, 1018, ());

  auto const locRef = segment.m_locationReference;
  TEST_EQUAL(locRef.m_points.size(), 2, ());

  auto const firstPoint = locRef.m_points.front();
  auto expectedLatLon = ms::LatLon{55.30683, 37.31041};
  TEST(firstPoint.m_latLon.EqualDxDy(expectedLatLon, 1e-5), (firstPoint.m_latLon, "!=", expectedLatLon));
  TEST_EQUAL(firstPoint.m_bearing, 8, ());
  TEST(firstPoint.m_formOfWay == openlr::FormOfWay::MultipleCarriageway,
       ("Wrong form of a way."));
  TEST(firstPoint.m_functionalRoadClass == openlr::FunctionalRoadClass::FRC6,
       ("Wrong functional road class."));
  TEST_EQUAL(firstPoint.m_distanceToNextPoint, 3572, ());
  TEST(firstPoint.m_lfrcnp == openlr::FunctionalRoadClass::FRC7, ("Wrong functional road class."));
  TEST_EQUAL(firstPoint.m_againstDrivingDirection, true, ());

  auto const secondPoint = locRef.m_points.back();
  expectedLatLon = ms::LatLon{55.33541, 37.29530};
  TEST(secondPoint.m_latLon.EqualDxDy(expectedLatLon, 1e-5), (secondPoint.m_latLon, "!=", expectedLatLon));
  TEST_EQUAL(secondPoint.m_bearing, 105, ());
  TEST(secondPoint.m_formOfWay == openlr::FormOfWay::SingleCarriageway, ("Wrong form of way."));
  TEST(secondPoint.m_functionalRoadClass == openlr::FunctionalRoadClass::FRC7,
       ("Wrong functional road class."));

  TEST_EQUAL(locRef.m_positiveOffsetMeters, 1637, ());
  TEST_EQUAL(locRef.m_negativeOffsetMeters, 919, ());
}

UNIT_TEST(LoadSamplePool_Test)
{
  platform::tests_support::ScopedFile sample(
      "sample.txt",
      "8442794\tRussia_Moscow Oblast_East-36328-0-fwd-58.3775=Russia_Moscow Oblast_East-36328-1-fwd-14.0846\n"
      "8442817\tRussia_Moscow Oblast_East-36324-11-bwd-101.362=Russia_Moscow Oblast_East-36324-10-bwd-48.2464=Russia_Moscow Oblast_East-36324-9-bwd-92.06\n"
      "8442983\tRussia_Moscow Oblast_East-45559-1-bwd-614.231=Russia_Moscow Oblast_East-45559-0-bwd-238.259\n"
      "8442988\tRussia_Moscow Oblast_East-36341-3-bwd-81.3394\n");

  Index emptyIndex;  // Empty is ok for this test.
  auto const pool = LoadSamplePool(sample.GetFullPath(), emptyIndex);

  TEST_EQUAL(pool.size(), 4, ());

  TEST(pool[0].m_evaluation == openlr::ItemEvaluation::Unevaluated,
       ("pool[0].m_evaluation != openlr::ItemEvaluation::Unevaluated"));
  TEST_EQUAL(pool[0].m_partnerSegmentId.Get(), 8442794, ());
  TEST_EQUAL(pool[1].m_partnerSegmentId.Get(), 8442817, ());
  TEST_EQUAL(pool[2].m_partnerSegmentId.Get(), 8442983, ());

  TEST_EQUAL(pool[0].m_segments.size(), 2, ());
  TEST_EQUAL(pool[1].m_segments.size(), 3, ());
  TEST_EQUAL(pool[2].m_segments.size(), 2, ());

  TEST_EQUAL(pool[0].m_segments[0].m_segId, 0, ());
  TEST_EQUAL(pool[1].m_segments[0].m_segId, 11, ());
  TEST_EQUAL(pool[2].m_segments[0].m_segId, 1, ());

  TEST(pool[0].m_segments[0].m_isForward, ());
  TEST(!pool[1].m_segments[0].m_isForward, ());
  TEST(!pool[2].m_segments[0].m_isForward, ());
}
