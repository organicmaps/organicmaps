#pragma once

#include "generator/affiliation.hpp"

#include "coding/file_writer.hpp"

#include "base/thread_pool_computational.hpp"

#include <istream>
#include <mutex>

namespace addr_generator
{

class Processor
{
  feature::CountriesFilesAffiliation m_affiliation;
  base::ComputationalThreadPool m_workers;

  std::mutex m_mtx;
  std::map<std::string, FileWriter> m_country2writer;
  std::string m_outputPath;

  FileWriter & GetWriter(std::string const & country);

public:
  explicit Processor(std::string const & dataPath, std::string const & outputPath, size_t numThreads);

  void Run(std::istream & is);
};

}  // namespace addr_generator
