#include "towns_dumper.hpp"

#include "coding/file_writer.hpp"

#include "base/logging.hpp"

#include "std/string.hpp"
#include "std/sstream.hpp"

TownsDumper::TownsDumper() {}
void TownsDumper::Dump(string filePath)
{
  ASSERT(!filePath.empty(), ());
  LOG(LINFO, ("Have", m_records.size(), "Towns"));
  ostringstream stream;
  stream.precision(9);
  for (auto const & record : m_records)
  {
    stream << record.point.lat << ";" << record.point.lon << ";" << record.id << std::endl;
  }
  string result = stream.str();
  FileWriter file(filePath);
  file.Write(result.c_str(), result.length());
  file.Flush();
}
