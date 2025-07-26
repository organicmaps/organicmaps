#pragma once

#include "generator/collection_base.hpp"
#include "generator/translator_interface.hpp"

#include <memory>

namespace generator
{
// This class allows you to work with a group of translators as with one.
class TranslatorCollection
  : public CollectionBase<std::shared_ptr<TranslatorInterface>>
  , public TranslatorInterface
{
public:
  // TranslatorInterface overrides:
  std::shared_ptr<TranslatorInterface> Clone() const override;

  void Emit(OsmElement const & element) override;

  void Finish() override;
  bool Save() override;

  IMPLEMENT_TRANSLATOR_IFACE(TranslatorCollection);
  void MergeInto(TranslatorCollection & other) const;
};
}  // namespace generator
