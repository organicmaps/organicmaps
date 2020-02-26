#pragma once

#include "topography_generator/utils/contours.hpp"

#include "coding/file_writer.hpp"
#include "coding/file_reader.hpp"
#include "coding/geometry_coding.hpp"
#include "coding/internal/file_data.hpp"

namespace topography_generator
{
template <typename ValueType>
class SerializerContours
{
public:
  explicit SerializerContours(Contours<ValueType> && contours): m_contours(std::move(contours)) {}

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    WriteToSink(sink, base::Underlying(Contours<ValueType>::Version::Latest));
    WriteToSink(sink, m_contours.m_minValue);
    WriteToSink(sink, m_contours.m_maxValue);
    WriteToSink(sink, m_contours.m_valueStep);
    WriteToSink(sink, static_cast<uint32_t>(m_contours.m_invalidValuesCount));

    WriteToSink(sink, static_cast<uint32_t>(m_contours.m_contours.size()));
    for (auto const & levelContours : m_contours.m_contours)
      SerializeContours(sink, levelContours.first, levelContours.second);
  }

private:
  template <typename Sink>
  void SerializeContours(Sink & sink, ValueType value,
                         std::vector<topography_generator::Contour> const & contours)
  {
    WriteToSink(sink, value);
    WriteToSink(sink, static_cast<uint32_t>(contours.size()));
    for (auto const & contour : contours)
      SerializeContour(sink, contour);
  }

  template <typename Sink>
  void SerializeContour(Sink & sink, topography_generator::Contour const & contour)
  {
    serial::GeometryCodingParams codingParams(kPointCoordBits, contour[0]);
    codingParams.Save(sink);
    serial::SaveOuterPath(contour, codingParams, sink);
  }

  Contours<ValueType> m_contours;
};

template <typename ValueType>
class DeserializerContours
{
public:
  template <typename Reader>
  void Deserialize(Reader & reader, Contours<ValueType> & contours)
  {
    NonOwningReaderSource source(reader);

    using VersT = typename Contours<ValueType>::Version;
    auto const v = static_cast<VersT>(ReadPrimitiveFromSource<std::underlying_type_t<VersT>>(source));
    CHECK(v == Contours<ValueType>::Version::Latest, ());

    contours.m_minValue = ReadPrimitiveFromSource<ValueType>(source);
    contours.m_maxValue = ReadPrimitiveFromSource<ValueType>(source);
    contours.m_valueStep = ReadPrimitiveFromSource<ValueType>(source);
    contours.m_invalidValuesCount = ReadPrimitiveFromSource<uint32_t>(source);

    size_t const levelsCount = ReadPrimitiveFromSource<uint32_t>(source);
    for (size_t i = 0; i < levelsCount; ++i)
    {
      ValueType levelValue;
      std::vector<topography_generator::Contour> levelContours;
      DeserializeContours(source, levelValue, levelContours);
      contours.m_contours[levelValue] = std::move(levelContours);
    }
  }

private:
  void DeserializeContours(NonOwningReaderSource & source, ValueType & value,
                            std::vector<topography_generator::Contour> & contours)
  {
    value = ReadPrimitiveFromSource<ValueType>(source);
    size_t const contoursCount = ReadPrimitiveFromSource<uint32_t>(source);
    contours.resize(contoursCount);
    for (auto & contour : contours)
      DeserializeContour(source, contour);
  }

  void DeserializeContour(NonOwningReaderSource & source,
                          topography_generator::Contour & contour)
  {
    serial::GeometryCodingParams codingParams;
    codingParams.Load(source);
    std::vector<m2::PointD> points;
    serial::LoadOuterPath(source, codingParams, points);
    contour = std::move(points);
  }
};

template <typename ValueType>
bool SaveContrours(std::string const & filePath,
                   Contours<ValueType> && contours)
{
  auto const tmpFilePath = filePath + ".tmp";
  try
  {
    FileWriter file(tmpFilePath);
    SerializerContours<ValueType> ser(std::move(contours));
    ser.Serialize(file);
  }
  catch (FileWriter::Exception const & ex)
  {
    LOG(LWARNING, ("File writer exception raised:", ex.what(), ", file", tmpFilePath));
    return false;
  }
  base::DeleteFileX(filePath);
  VERIFY(base::RenameFileX(tmpFilePath, filePath), (tmpFilePath, filePath));
  return true;
}

template <typename ValueType>
bool LoadContours(std::string const & filePath, Contours<ValueType> & contours)
{
  try
  {
    FileReader file(filePath);
    DeserializerContours<ValueType> des;
    des.Deserialize(file, contours);
  }
  catch (FileReader::Exception const & ex)
  {
    LOG(LWARNING, ("File reader exception raised:", ex.what(), ", file", filePath));
    return false;
  }
  return true;
}
}  // namespace topography_generator
