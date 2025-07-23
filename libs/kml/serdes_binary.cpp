#include "kml/serdes_binary.hpp"

namespace kml
{
namespace binary
{
SerializerKml::SerializerKml(FileData & data) : m_data(data)
{
  ClearCollectionIndex();

  // Collect all strings and substitute each for index.
  auto const avgSz = data.m_bookmarksData.size() * 2 + data.m_tracksData.size() * 2 + 10;
  LocalizableStringCollector collector(avgSz);
  CollectorVisitor<decltype(collector)> visitor(collector);
  m_data.Visit(visitor);
  m_strings = collector.StealCollection();
}

SerializerKml::~SerializerKml()
{
  ClearCollectionIndex();
}

void SerializerKml::ClearCollectionIndex()
{
  LocalizableStringCollector collector(0);
  CollectorVisitor<decltype(collector)> clearVisitor(collector, true /* clear index */);
  m_data.Visit(clearVisitor);
}

DeserializerKml::DeserializerKml(FileData & data) : m_data(data)
{
  m_data = {};
}
}  // namespace binary
}  // namespace kml
