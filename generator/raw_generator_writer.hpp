#pragma once

#include "generator/feature_builder.hpp"
#include "generator/features_processing_helpers.hpp"

#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace generator
{
class RawGeneratorWriter
{
public:
  RawGeneratorWriter(std::shared_ptr<FeatureProcessorQueue> const & queue, std::string const & path);
  ~RawGeneratorWriter();

  void Run();
  void ShutdownAndJoin();
  std::vector<std::string> GetNames();

private:
  using FeatureBuilderWriter = feature::FeatureBuilderWriter<feature::serialization_policy::MaxAccuracy>;

  void Write(std::vector<ProcessedData> const & vecChanks);

  std::thread m_thread;
  std::shared_ptr<FeatureProcessorQueue> m_queue;
  std::string m_path;
  std::unordered_map<std::string, std::unique_ptr<FileWriter>> m_writers;
};
}  // namespace generator
