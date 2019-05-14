#pragma once

#include "local_ads/event.hpp"

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace local_ads
{
void WriteCountryName(FileWriter & writer, std::string const & countryName);

void WriteZigZag(FileWriter & writer, int64_t duration);

template <typename Duration>
void WriteTimestamp(FileWriter & writer, Timestamp ts)
{
  int64_t const d = std::chrono::duration_cast<Duration>(ts.time_since_epoch()).count();
  WriteZigZag(writer, d);
}

void WriteRawData(FileWriter & writer, std::vector<uint8_t> const & rawData);

std::string ReadCountryName(ReaderSource<FileReader> & src);

int64_t ReadZigZag(ReaderSource<FileReader> & src);

template <typename Duration>
Timestamp ReadTimestamp(ReaderSource<FileReader> & src)
{
  int64_t const d = ReadZigZag(src);
  return Timestamp(Duration(d));
}

std::vector<uint8_t> ReadRawData(ReaderSource<FileReader> & src);
}  // namespace local_ads
