#pragma once

#include "geometry/rect2d.hpp"

#include "coding/file_writer.hpp"

#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace feature
{
class FeatureBuilder;

// Writes features to file.
class FeaturesCollector
{
public:
  static size_t constexpr kBufferSize = 48000;

  FeaturesCollector(std::string const & fName, FileWriter::Op op = FileWriter::Op::OP_WRITE_TRUNCATE);
  virtual ~FeaturesCollector();

  static uint64_t GetCurrentPosition();
  std::string const & GetFilePath() const { return m_dataFile.GetName(); }
  /// \brief Serializes |f|.
  /// \returns Feature id of serialized feature.
  virtual uint32_t Collect(FeatureBuilder const & f);
  virtual void Finish() {}

protected:
  /// \return Feature offset in the file, which is used as an ID later
  uint32_t WriteFeatureBase(std::vector<char> const & bytes, FeatureBuilder const & fb);
  void Flush();

  FileWriter m_dataFile;
  m2::RectD m_bounds;

private:
  void Write(char const * src, size_t size);
  void FlushBuffer();

  std::vector<char> m_writeBuffer;
  size_t m_writePosition = 0;
  uint32_t m_featureID = 0;
};

class FeaturesAndRawGeometryCollector : public FeaturesCollector
{
  FileWriter m_rawGeometryFileStream;
  size_t m_rawGeometryCounter = 0;

public:
  FeaturesAndRawGeometryCollector(std::string const & featuresFileName, std::string const & rawGeometryFileName);
  ~FeaturesAndRawGeometryCollector() override;

  uint32_t Collect(FeatureBuilder const & f) override;
};

uint32_t CheckedFilePosCast(FileWriter const & f);
}  // namespace feature
