#pragma once
#include "../indexer/index.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/base.hpp"
#include "../std/function.hpp"
#include "../std/string.hpp"

class FileReader;
class FeatureType;

namespace search
{

class Result;

class Engine
{
public:
  typedef Index<FileReader>::Type IndexType;

  explicit Engine(IndexType const * pIndex);

  void Search(string const & query,
              m2::RectD const & rect,
              function<void (Result const &)> const & f);

private:
  IndexType const * m_pIndex;
};

}  // namespace search
