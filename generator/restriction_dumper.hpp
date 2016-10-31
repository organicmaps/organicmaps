#pragma once

#include "std/fstream.hpp"
#include "std/string.hpp"

class RelationElement;

class RestrictionDumper
{
  ofstream m_stream;

public:
  void Open(string const & fullPath);
  bool IsOpened();

  /// \brief Writes |relationElement| to |m_stream| if |relationElement| is a supported restriction.
  /// \note For the time being only line-point-line restritions are processed. The other restrictions
  /// are ignored.
  // @TODO(bykoianko) It's necessary to process all kind of restrictions.
  void Write(RelationElement const & relationElement);
};
