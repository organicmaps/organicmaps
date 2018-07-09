#include "testing/testing.hpp"

#include "indexer/cell_coverer.hpp"
#include "indexer/indexer_tests/bounds.hpp"

#include "geometry/covering_utils.hpp"

#include "coding/hex.hpp"

#include "base/logging.hpp"

// Unit test uses m2::CellId<30> for historical reasons, the actual production code uses RectId.
typedef m2::CellId<30> CellIdT;

UNIT_TEST(CellIdToStringRecode)
{
  char const kTest[] = "21032012203";
  TEST_EQUAL(CellIdT::FromString(kTest).ToString(), kTest, ());
}

UNIT_TEST(GoldenCoverRect)
{
  vector<CellIdT> cells;
  CoverRect<OrthoBounds>({27.43, 53.83, 27.70, 53.96}, 4, RectId::DEPTH_LEVELS, cells);

  TEST_EQUAL(cells.size(), 4, ());

  TEST_EQUAL(cells[0].ToString(), "32012211300", ());
  TEST_EQUAL(cells[1].ToString(), "32012211301", ());
  TEST_EQUAL(cells[2].ToString(), "32012211302", ());
  TEST_EQUAL(cells[3].ToString(), "32012211303", ());
}

UNIT_TEST(ArtificialCoverRect)
{
  typedef Bounds<0, 0, 16, 16> TestBounds;

  vector<CellIdT> cells;
  CoverRect<TestBounds>({5, 5, 11, 11}, 4, RectId::DEPTH_LEVELS, cells);

  TEST_EQUAL(cells.size(), 4, ());

  TEST_EQUAL(cells[0].ToString(), "03", ());
  TEST_EQUAL(cells[1].ToString(), "12", ());
  TEST_EQUAL(cells[2].ToString(), "21", ());
  TEST_EQUAL(cells[3].ToString(), "30", ());
}
