#pragma once

#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"

#include <chrono>
#include <string>
#include <vector>

namespace local_ads
{
void WriteCountryName(FileWriter & writer, std::string const & countryName);

void WriteDuration(FileWriter & writer, int64_t duration);

template <typename Duration>
void WriteTimestamp(FileWriter & writer, std::chrono::steady_clock::time_point ts)
{
  int64_t const d = std::chrono::duration_cast<Duration>(ts.time_since_epoch()).count();
  WriteDuration(writer, d);
}

void WriteRawData(FileWriter & writer, std::vector<uint8_t> const & rawData);

std::string ReadCountryName(ReaderSource<FileReader> & src);

int64_t ReadDuration(ReaderSource<FileReader> & src);

template <typename Duration>
std::chrono::steady_clock::time_point ReadTimestamp(ReaderSource<FileReader> & src)
{
  int64_t const d = ReadDuration(src);
  return std::chrono::steady_clock::time_point(Duration(d));
}

std::vector<uint8_t> ReadRawData(ReaderSource<FileReader> & src);
}  // namespace local_ads
