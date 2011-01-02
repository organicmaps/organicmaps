#include "../../testing/testing.hpp"

#include "../data_header_reader.hpp"
#include "../data_header.hpp"
#include "../cell_id.hpp"

#include "../../coding/file_reader.hpp"
#include "../../coding/file_writer.hpp"


UNIT_TEST(DataHeaderSerialization)
{
  char const * fileName = "mfj4340smn54123.tmp";
  feature::DataHeader header1;
  // normalize rect due to convertation rounding errors
  m2::RectD rect(11.5, 12.6, 13.7, 14.8);
  std::pair<int64_t, int64_t> cellIds = RectToInt64(rect);
  rect = Int64ToRect(cellIds);

  header1.SetBounds(rect);
  uint64_t const controlNumber = 0x54321643;
  {
    FileWriter writer(fileName);
    feature::WriteDataHeader(writer, header1);
    writer.Write(&controlNumber, sizeof(controlNumber));
  }

  feature::DataHeader header2;
  TEST_GREATER(feature::ReadDataHeader(FileReader(fileName), header2), 0, ());

  TEST_EQUAL(header1.Bounds(), header2.Bounds(), ());

  {
    FileReader reader(fileName);
    uint64_t const headerSize = feature::GetSkipHeaderSize(reader);
    TEST_GREATER(headerSize, 0, ());
    uint64_t number = 0;
    reader.Read(headerSize, &number, sizeof(number));
    TEST_EQUAL(controlNumber, number, ());
  }

  FileWriter::DeleteFile(fileName);
}
