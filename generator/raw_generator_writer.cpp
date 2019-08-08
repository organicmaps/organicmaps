#include "generator/raw_generator_writer.hpp"

#include "coding/varint.hpp"

#include "base/file_name_utils.hpp"

#include <iterator>

namespace generator
{
RawGeneratorWriter::RawGeneratorWriter(std::shared_ptr<FeatureProcessorQueue> const & queue,
                                       std::string const & path)
  : m_queue(queue), m_path(path) {}


RawGeneratorWriter::~RawGeneratorWriter()
{
  ShutdownAndJoin();
}

void RawGeneratorWriter::Run()
{
  m_thread = std::thread([&]() {
    while (true)
    {
      FeatureProcessorChank chank;
      m_queue->WaitAndPop(chank);
      if (chank.IsEmpty())
        return;

      Write(chank.Get());
    }
  });
}

std::vector<std::string> RawGeneratorWriter::GetNames()
{
  ShutdownAndJoin();
  std::vector<std::string> names;
  names.reserve(m_collectors.size());
  for (const auto  & p : m_collectors)
    names.emplace_back(p.first);

  return names;
}

void RawGeneratorWriter::Write(std::vector<ProcessedData> const & vecChanks)
{
  for (auto const & chank : vecChanks)
  {
    for (auto const & affiliation : chank.m_affiliations)
    {
      if (affiliation.empty())
        continue;

      auto collectorIt = m_collectors.find(affiliation);
      if (collectorIt == std::cend(m_collectors))
      {
        auto path = base::JoinPath(m_path, affiliation + DATA_FILE_EXTENSION_TMP);
        auto writer = std::make_unique<FileWriter>(std::move(path));
        collectorIt = m_collectors.emplace(affiliation, std::move(writer)).first;
      }

      auto & collector = collectorIt->second;
      auto const & buffer = chank.m_buffer;
      WriteVarUint(*collector, static_cast<uint32_t>(buffer.size()));
      collector->Write(buffer.data(), buffer.size());
    }
  }
}

void RawGeneratorWriter::ShutdownAndJoin()
{
  m_queue->Push({});
  if (m_thread.joinable())
    m_thread.join();
}
}  // namespace generator
