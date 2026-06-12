#include "indexer/classificator_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/drawing_rules.hpp"
#include "indexer/drules_format.hpp"
#include "indexer/map_style_reader.hpp"

#include "platform/platform.hpp"

#include "coding/reader.hpp"
#include "coding/reader_streambuf.hpp"

#include "base/buffer_vector.hpp"
#include "base/logging.hpp"

#include <iostream>
#include <memory>
#include <string>

namespace
{
void ReadCommon(Classificator & c, std::unique_ptr<Reader> classificator, std::unique_ptr<Reader> types)
{
  c.Clear();

  {
    // LOG(LINFO, ("Reading classificator"));
    ReaderStreamBuf buffer(std::move(classificator));

    std::istream s(&buffer);
    c.ReadClassificator(s);
  }

  {
    // LOG(LINFO, ("Reading types mapping"));
    ReaderStreamBuf buffer(std::move(types));

    std::istream s(&buffer);
    c.ReadTypesMapping(s);
  }
}

using VariantsT = buffer_vector<MapStyle, 2>;

// Styles that share one family file; light and dark are loaded together so that switching between
// them (e.g. auto day/night) needs no extra I/O.
VariantsT FamilyVariants(MapStyle style)
{
  switch (style)
  {
  case MapStyleDefaultLight:
  case MapStyleDefaultDark: return {MapStyleDefaultLight, MapStyleDefaultDark};
  case MapStyleVehicleLight:
  case MapStyleVehicleDark: return {MapStyleVehicleLight, MapStyleVehicleDark};
  case MapStyleOutdoorsLight:
  case MapStyleOutdoorsDark: return {MapStyleOutdoorsLight, MapStyleOutdoorsDark};
  case MapStyleMerged: return {MapStyleMerged};
  case MapStyleCount: break;
  }
  return {MapStyleDefaultLight, MapStyleDefaultDark};
}

// Force-loads every variant of style's family from a single decode of the shared family file,
// writing into each variant's own classificator tree and rules holder. It never changes the global
// current style, so background tile readers keep observing the previously-current (immutable) style
// the whole time; the switch to the new style happens later via a single atomic SetCurrentStyle.
void LoadFamily(MapStyle style)
{
  Platform & p = GetPlatform();
  VariantsT const variants = FamilyVariants(style);

  drule::DrulesFormat const fmt = drule::DecodeRules(variants.front());

  for (MapStyle const variant : variants)
  {
    Classificator & c = classif(variant);
    ReadCommon(c, p.GetReader("classificator.txt"), p.GetReader("types.txt"));
    drule::GetRules(variant).LoadFromFormat(fmt, GetStyleReader().GetDrawingRulesVariant(variant), c);
    c.SetLoaded(true);
  }
}
}  // namespace

namespace classificator
{
bool IsStyleLoaded(MapStyle mapStyle)
{
  return classif(mapStyle).IsLoaded();
}

void EnsureStyleLoaded(MapStyle mapStyle)
{
  if (!classif(mapStyle).IsLoaded())
    LoadFamily(mapStyle);
}

void Load()
{
  LOG(LDEBUG, ("Reading of classificator started"));

  // Drop every loaded flag so the designer's rebuilt drules are re-read on reload, then load the
  // current and outdoors families now (others, e.g. vehicle, load lazily on first use). Outdoors is
  // always made resident because GetOutdoorRules/GetOutdoorClassif render forced-outdoors tracks.
  for (size_t i = 0; i < MapStyleCount; ++i)
    classif(static_cast<MapStyle>(i)).SetLoaded(false);

  LoadFamily(GetStyleReader().GetCurrentStyle());
  EnsureStyleLoaded(MapStyleOutdoorsLight);

  LOG(LDEBUG, ("Reading of classificator finished"));
}

void LoadTypes(std::string const & classificatorFileStr, std::string const & typesFileStr)
{
  ReadCommon(classif(),
             std::make_unique<MemReaderWithExceptions>(classificatorFileStr.data(), classificatorFileStr.size()),
             std::make_unique<MemReaderWithExceptions>(typesFileStr.data(), typesFileStr.size()));
}
}  // namespace classificator
