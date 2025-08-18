#pragma once

#include "testing/testing.hpp"

#include "coding/reader.hpp"

#include <string>

namespace
{
template <class ReaderT>
void ReadToStringFromPos(ReaderT const & reader, std::string & str, uint64_t pos, size_t size)
{
  str.resize(size);
  reader.Read(pos, &str[0], str.size());
}

template <class SourceT>
void ReadToStringFromSource(SourceT & source, std::string & str, size_t size)
{
  str.resize(size);
  source.Read(&str[0], str.size());
}
}  // namespace

template <typename ReaderT>
void TestReader(ReaderT const & reader)
{
  ReaderSource<ReaderT> source(reader);
  std::string d1;
  ReadToStringFromSource(source, d1, 6);
  TEST_EQUAL(d1, "Quick ", ());

  ReadToStringFromSource(source, d1, 6);
  TEST_EQUAL(d1, "brown ", ());

  ReaderT subReader = source.SubReader(10);
  ReadToStringFromPos(subReader, d1, 1, 3);
  TEST_EQUAL(d1, "ox ", ());

  ReaderT subSubReader = subReader.SubReader(2, 8);
  ReadToStringFromPos(subSubReader, d1, 0, 2);
  TEST_EQUAL(d1, "x ", ());

  ReadToStringFromSource(source, d1, 5);
  TEST_EQUAL(d1, "over ", ());

  ReaderSource<ReaderT> subReaderSource(subReader);
  ReadToStringFromSource(subReaderSource, d1, 6);
  TEST_EQUAL(d1, "fox ju", ());
}
