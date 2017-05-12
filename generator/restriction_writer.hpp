#pragma once

#include <fstream>
#include <string>

class RelationElement;

namespace routing
{
class RestrictionWriter
{
public:
  void Open(std::string const & fullPath);

  /// \brief Writes |relationElement| to |m_stream| if |relationElement| is a supported restriction.
  /// See restriction_generator.hpp for the description of the format.
  /// \note For the time being only line-point-line restrictions are processed. The other
  /// restrictions are ignored.
  // @TODO(bykoianko) It's necessary to process all kind of restrictions.
  void Write(RelationElement const & relationElement);

private:
  bool IsOpened() const;

  std::ofstream m_stream;
};
}  // namespace routing
