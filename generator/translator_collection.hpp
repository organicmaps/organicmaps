#pragma once

#include "generator/collection_base.hpp"
#include "generator/translator_interface.hpp"

#include <memory>

namespace generator
{
// This class allows you to work with a group of translators as with one.
class TranslatorCollection : public CollectionBase<std::shared_ptr<TranslatorInterface>>, public TranslatorInterface
{
public:
  // TranslatorInterface overrides:
  void Emit(OsmElement /* const */ & element) override;
  bool Finish() override;
  void GetNames(std::vector<std::string> & names) const override;
};
}  // namespace generator
