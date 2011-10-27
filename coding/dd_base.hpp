#pragma once
#include "reader.hpp"
#include "../base/base.hpp"
#include "../base/exception.hpp"


DECLARE_EXCEPTION(DDParseException, RootException);

// TODO: Remove DDParseInfo and use constructors instead of Parse() method.
template <typename TReader>
class DDParseInfo
{
public:
  DDParseInfo(TReader const & reader, bool failOnError) :
      m_source(reader), m_bFailOnError(failOnError)
  {
  }

  ReaderSource<TReader> & Source()
  {
    return m_source;
  }

private:
  ReaderSource<TReader> m_source;
  bool m_bFailOnError;
};
