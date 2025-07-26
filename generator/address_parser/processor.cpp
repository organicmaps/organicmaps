#include "processor.hpp"

#include "tiger_parser.hpp"

#include "geometry/mercator.hpp"

#include "coding/point_coding.hpp"

#include "base/file_name_utils.hpp"

#include "defines.hpp"

namespace addr_generator
{

Processor::Processor(std::string const & dataPath, std::string const & outputPath, size_t numThreads)
  : m_affiliation(dataPath, true /* haveBordersForWholeWorld */)
  , m_workers(numThreads)
  , m_outputPath(outputPath)
{}

FileWriter & Processor::GetWriter(std::string const & country)
{
  auto res = m_country2writer.try_emplace(country, base::JoinPath(m_outputPath, country) + TEMP_ADDR_EXTENSION);
  return res.first->second;
}

void Processor::Run(std::istream & is)
{
  std::atomic<size_t> incomplete = 0;
  size_t total = 0;

  std::string line;
  while (std::getline(is, line))
  {
    ++total;

    m_workers.SubmitWork([this, &incomplete, copy = std::move(line)]() mutable
    {
      std::string_view line = copy;

      // Remove possible trailing "\t\n" (pipe reading from tar).
      size_t const i = line.rfind(')');
      if (i == std::string::npos)
      {
        LOG(LWARNING, ("Invalid string:", line));
        return;
      }
      line = line.substr(0, i + 1);

      tiger::AddressEntry e;
      if (!tiger::ParseLine(line, e))
      {
        LOG(LWARNING, ("Bad entry:", line));
        return;
      }

      if (e.m_from.empty() || e.m_to.empty() || e.m_street.empty())
      {
        ++incomplete;
        return;
      }

      std::vector<m2::PointD> points;
      points.reserve(e.m_geom.size());
      for (auto const & ll : e.m_geom)
        points.push_back(mercator::FromLatLon(ll));

      auto const countries = m_affiliation.GetAffiliations(points);

      std::vector<int64_t> iPoints;
      iPoints.reserve(points.size());
      for (auto const & p : points)
        iPoints.push_back(PointToInt64Obsolete(p, kPointCoordBits));

      std::lock_guard guard(m_mtx);
      for (auto const & country : countries)
      {
        auto & writer = GetWriter(country);
        e.Save(writer);
        rw::Write(writer, iPoints);
      }
    });
  }

  m_workers.WaitingStop();

  LOG(LINFO, ("Total entries:", total, "Incomplete:", incomplete));
}

}  // namespace addr_generator
