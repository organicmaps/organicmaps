#pragma once
#include "../indexer/index.hpp"
#include "../geometry/rect2d.hpp"
#include "../base/base.hpp"
#include "../base/runner.hpp"
#include "../std/function.hpp"
#include "../std/scoped_ptr.hpp"
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
  scoped_ptr<threads::IRunner> m_pRunner;
};

}  // namespace search
