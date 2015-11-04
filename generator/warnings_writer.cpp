#include "warnings_writer.hpp"

#include "coding/file_writer.hpp"

namespace routing
{
void WarningsWriter::Write()
{
  if (m_fileName.empty())
    return;
  stringstream out;
  out << "Lat;Lon;Type;\n";
  for (auto const & point : m_points)
  {
    out << point.first.lat << ";" << point.first.lon << ";" << char(point.second) << ";" << std::endl;
  }
  FileWriter writer(m_fileName);
  string const result = out.str();
  writer.Write(result.data(), result.size());
}

void WarningsWriter::AddPoint(ms::LatLon const & point, PointType type)
{
  if (!m_fileName.empty())
    m_points.emplace_back(point, type);
}
}  // namespace routing
