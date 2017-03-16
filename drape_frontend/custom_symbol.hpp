#pragma once

#include "indexer/feature_decl.hpp"

#include <map>
#include <memory>
#include <string>

namespace df
{
struct CustomSymbol
{
  std::string m_symbolName;
  bool m_prioritized = false;

  CustomSymbol() = default;
  CustomSymbol(std::string const & name, bool prioritized)
    : m_symbolName(name), m_prioritized(prioritized)
  {}
};

using CustomSymbols = std::map<FeatureID, CustomSymbol>;

struct CustomSymbolsContext
{
  CustomSymbols m_symbols;

  explicit CustomSymbolsContext(CustomSymbols && symbols)
    : m_symbols(std::move(symbols))
  {}
};

using CustomSymbolsContextPtr = std::shared_ptr<CustomSymbolsContext>;
using CustomSymbolsContextWeakPtr = std::weak_ptr<CustomSymbolsContext>;
} //  namespace df
