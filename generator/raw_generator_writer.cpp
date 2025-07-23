#include "generator/raw_generator_writer.hpp"

#include "coding/varint.hpp"

#include "base/file_name_utils.hpp"

#include <iterator>

namespace generator
{
RawGeneratorWriter::RawGeneratorWriter(std::shared_ptr<FeatureProcessorQueue> const & queue, std::string const & path)
  : m_queue(queue)
  , m_path(path)
{}

RawGeneratorWriter::~RawGeneratorWriter()
{
  ShutdownAndJoin();
}

void RawGeneratorWriter::Run()
{
  m_thread = std::thread([&]()
  {
    while (true)
    {
      FeatureProcessorChunk chunk;
      m_queue->WaitAndPop(chunk);
      // As a sign of the end of tasks, we use an empty message. We have the right to do that,
      // because there is only one reader.
      if (!chunk.has_value())
        return;

      Write(*chunk);
    }
  });
}

std::vector<std::string> RawGeneratorWriter::GetNames()
{
  CHECK(!m_thread.joinable(), ());

  std::vector<std::string> names;
  names.reserve(m_writers.size());
  for (auto const & p : m_writers)
    names.emplace_back(p.first);

  return names;
}

void RawGeneratorWriter::Write(std::vector<ProcessedData> const & vecChunks)
{
  for (auto const & chunk : vecChunks)
  {
    for (auto const & affiliation : chunk.m_affiliations)
    {
      if (affiliation.empty())
        continue;

      auto writerIt = m_writers.find(affiliation);
      if (writerIt == std::cend(m_writers))
      {
        auto path = base::JoinPath(m_path, affiliation + DATA_FILE_EXTENSION_TMP);
        LOG(LINFO, ("Start writing to ", path));
        auto writer = std::make_unique<FileWriter>(std::move(path));
        writerIt = m_writers.emplace(affiliation, std::move(writer)).first;
      }

      auto & writer = writerIt->second;
      auto const & buffer = chunk.m_buffer;
      WriteVarUint(*writer, static_cast<uint32_t>(buffer.size()));
      writer->Write(buffer.data(), buffer.size());
    }
  }
}

void RawGeneratorWriter::ShutdownAndJoin()
{
  if (m_thread.joinable())
  {
    m_queue->Push({});
    m_thread.join();
  }
}
}  // namespace generator
