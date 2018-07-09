#pragma once

#include "geometry/rect2d.hpp"

#include "coding/file_writer.hpp"

#include <limits>
#include <string>
#include <vector>

class FeatureBuilder1;

namespace feature
{
// Writes features to dat file.
class FeaturesCollector
{
  char m_writeBuffer[48000];
  size_t m_writePosition = 0;
  uint32_t m_featureID = 0;

protected:
  static uint32_t constexpr kInvalidFeatureId = std::numeric_limits<uint32_t>::max();

  FileWriter m_datFile;
  m2::RectD m_bounds;

private:
  void Write(char const * src, size_t size);
  void FlushBuffer();

protected:

  /// @return feature offset in the file, which is used as an ID later
  uint32_t WriteFeatureBase(std::vector<char> const & bytes, FeatureBuilder1 const & fb);

  void Flush();

public:
  FeaturesCollector(std::string const & fName);
  virtual ~FeaturesCollector();

  static uint32_t GetFileSize(FileWriter const & f);
  std::string const & GetFilePath() const { return m_datFile.GetName(); }
  /// \brief Serializes |f|.
  /// \returns feature id of serialized feature if |f| is serialized after the call
  /// and |kInvalidFeatureId| if not.
  /// \note See implementation operator() in derived class for cases when |f| cannot be
  /// serialized.
  virtual uint32_t operator()(FeatureBuilder1 const & f);
  virtual uint32_t operator()(FeatureBuilder1 & f)
  {
    auto const & f1 = f;
    return (*this)(f1);
  };
  virtual void Finish() {}
};

class FeaturesAndRawGeometryCollector : public FeaturesCollector
{
  FileWriter m_rawGeometryFileStream;
  size_t m_rawGeometryCounter = 0;

public:
  FeaturesAndRawGeometryCollector(std::string const & featuresFileName,
                                  std::string const & rawGeometryFileName);
  ~FeaturesAndRawGeometryCollector();

  uint32_t operator()(FeatureBuilder1 const & f) override;
};
}
