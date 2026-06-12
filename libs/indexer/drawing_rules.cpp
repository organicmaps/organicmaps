#include "indexer/drawing_rules.hpp"

#include "indexer/classificator.hpp"
#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"
#include "base/logging.hpp"

#include <string_view>
#include <unordered_map>
#include <utility>

namespace drule
{
namespace
{
uint32_t const DEFAULT_BG_COLOR = 0xEEEEDD;
}  // namespace

bool BaseRule::TestFeature(FeatureType & ft, int zoom) const
{
  if (nullptr == m_selector)
    return true;
  return m_selector->Test(ft, zoom);
}

void BaseRule::SetSelector(std::unique_ptr<ISelector> && selector)
{
  m_selector = std::move(selector);
}

RulesHolder::RulesHolder() : m_bgColors(scales::UPPER_STYLE_SCALE + 1, DEFAULT_BG_COLOR) {}

RulesHolder::~RulesHolder()
{
  Clean();
}

void RulesHolder::Clean()
{
  m_dRules.clear();
  m_lineRules.clear();
  m_areaRules.clear();
  m_symbolRules.clear();
  m_captionRules.clear();
  m_pathtextRules.clear();
  m_shieldRules.clear();
  m_colors.clear();
}

BaseRule const * RulesHolder::Find(Key const & k) const
{
  ASSERT_LESS(k.m_index, m_dRules.size(), ());
  return m_dRules[k.m_index];
}

uint32_t RulesHolder::GetBgColor(int scale) const
{
  ASSERT_LESS(scale, static_cast<int>(m_bgColors.size()), ());
  ASSERT_GREATER_OR_EQUAL(scale, 0, ());
  return m_bgColors[scale];
}

uint32_t RulesHolder::GetColor(std::string_view name) const
{
  auto const it = m_colors.find(name);
  if (it == m_colors.end())
  {
    LOG(LWARNING, ("Requested color", name, "is not found"));
    return 0;
  }
  return it->second;
}

namespace
{
RulesHolder & rules(MapStyle mapStyle)
{
  static RulesHolder h[MapStyleCount];
  return h[mapStyle];
}
}  // namespace

RulesHolder & GetCurrentRules()
{
  return rules(GetStyleReader().GetCurrentStyle());
}

RulesHolder & GetOutdoorRules()
{
  auto const style = GetStyleReader().GetCurrentStyle();
  return rules(MapStyleIsDark(style) ? MapStyleOutdoorsDark : MapStyleOutdoorsLight);
}

RulesHolder & GetRules(MapStyle mapStyle)
{
  return rules(mapStyle);
}

// Builds rules for one decoded family variant into a RulesHolder and the current classificator tree.
// Walks the tree exactly like the old DoSetIndex did, but resolves a type to its draw rules via an
// O(1) name lookup (and an incrementally built path) instead of a per-node binary search.
class RulesLoader
{
public:
  RulesLoader(RulesHolder & holder, DrulesFormat const & fmt, size_t variant, Classificator & classifTree)
    : m_holder(holder)
    , m_fmt(fmt)
    , m_variant(variant)
    , m_classif(classifTree)
  {
    m_byName.reserve(fmt.types.size());
    for (auto const & t : fmt.types)
      m_byName.emplace(std::string_view(t.name), &t);
  }

  void Run()
  {
    m_classif.GetMutableRoot()->ForEachObject([this](ClassifObject * p) { Walk(p); });
  }

private:
  // idx is validated against the palette size at decode time (see Decoder::ReadColorIdx).
  uint32_t Color(uint32_t idx) const { return m_fmt.colors[m_variant][idx]; }

  void Walk(ClassifObject * p)
  {
    size_t const prevLen = m_path.size();
    if (!m_path.empty())
      m_path += '-';
    m_path += p->GetName();

    auto const it = m_byName.find(std::string_view(m_path));
    if (it != m_byName.end())
      for (auto const & el : it->second->elements)
        AddElement(p, el);

    p->ForEachObject([this](ClassifObject * c) { Walk(c); });

    m_path.resize(prevLen);
  }

  void ResolveCaption(CaptionRule & c) const
  {
    if (c.primary)
    {
      c.primary->color = Color(c.primary->color);
      c.primary->stroke_color = Color(c.primary->stroke_color);
    }
    if (c.secondary)
    {
      c.secondary->color = Color(c.secondary->color);
      c.secondary->stroke_color = Color(c.secondary->stroke_color);
    }
  }

  // Emission order (lines, area, symbol, caption, path_text, shield) must match the old loader so
  // that drule::Key indices, priorities and GetSuitable ordering stay bit-identical.
  void AddElement(ClassifObject * p, Element const & el)
  {
    int const scale = el.scale;
    auto const & applyIf = el.applyIf;

    for (auto const & line : el.lines)
    {
      LineRuleHolder h;
      h.m_rule = line;
      h.m_rule.color = Color(h.m_rule.color);
      Add(p, scale, drule::line, m_holder.m_lineRules, std::move(h), line.priority, applyIf);
    }
    if (el.area)
    {
      AreaRuleHolder h;
      h.m_rule = *el.area;
      h.m_rule.color = Color(h.m_rule.color);
      if (h.m_rule.border)
        h.m_rule.border->color = Color(h.m_rule.border->color);
      Add(p, scale, drule::area, m_holder.m_areaRules, std::move(h), el.area->priority, applyIf);
    }
    if (el.symbol)
    {
      SymbolRuleHolder h;
      h.m_rule = *el.symbol;
      Add(p, scale, drule::symbol, m_holder.m_symbolRules, std::move(h), el.symbol->priority, applyIf);
    }
    if (el.caption)
    {
      CaptionRuleHolder h;
      h.m_rule = *el.caption;
      ResolveCaption(h.m_rule);
      Add(p, scale, drule::caption, m_holder.m_captionRules, std::move(h), el.caption->priority, applyIf);
    }
    if (el.pathText)
    {
      PathTextRuleHolder h;
      h.m_rule = *el.pathText;
      ResolveCaption(h.m_rule);
      Add(p, scale, drule::pathtext, m_holder.m_pathtextRules, std::move(h), el.pathText->priority, applyIf);
    }
    if (el.shield)
    {
      ShieldRuleHolder h;
      h.m_rule = *el.shield;
      auto & r = h.m_rule;
      r.color = Color(r.color);
      r.stroke_color = Color(r.stroke_color);
      r.text_color = Color(r.text_color);
      r.text_stroke_color = Color(r.text_stroke_color);
      Add(p, scale, drule::shield, m_holder.m_shieldRules, std::move(h), el.shield->priority, applyIf);
    }
  }

  template <class Holder, class Storage>
  void Add(ClassifObject * p, int scale, TypeT type, Storage & storage, Holder && holder, int priority,
           std::vector<std::string> const & applyIf)
  {
    std::unique_ptr<ISelector> selector;
    if (!applyIf.empty())
    {
      selector = ParseSelector(applyIf);
      if (selector == nullptr)
      {
        LOG(LERROR, ("Runtime selector has not been created:", applyIf));
        return;
      }
    }

    storage.push_back(std::forward<Holder>(holder));
    BaseRule * obj = &storage.back();
    obj->SetSelector(std::move(selector));

    m_holder.m_dRules.push_back(obj);
    Key k(scale, type, m_holder.m_dRules.size() - 1);
    k.SetPriority(priority);
    p->SetVisibilityOnScale(true, scale);
    p->AddDrawRule(k);
  }

  RulesHolder & m_holder;
  DrulesFormat const & m_fmt;
  size_t m_variant;
  Classificator & m_classif;
  std::unordered_map<std::string_view, TypeEntry const *> m_byName;
  std::string m_path;
};

void RulesHolder::InitBackgroundColors(DrulesFormat const & fmt, size_t variant)
{
  // Background color is not stored explicitly; the color of the "natural-land" area element is used.
  // If it is absent the default background color is used.
  uint32_t bgColorDefault = DEFAULT_BG_COLOR;
  std::unordered_map<int, uint32_t> bgColorForScale;

  for (auto const & te : fmt.types)
  {
    if (te.name != "natural-land")
      continue;
    for (auto const & el : te.elements)
    {
      if (!el.area)
        continue;
      uint32_t const color = fmt.colors[variant][el.area->color];
      bgColorDefault = color;
      // A non-zero scale must occur at most once for natural-land: VERIFY the emplace inserted (the
      // result is checked in debug only; release still runs it, keeping the first value as before).
      if (el.scale != 0)
        VERIFY(bgColorForScale.try_emplace(el.scale, color).second, ("Duplicate natural-land scale", el.scale));
    }
    break;
  }

  ASSERT_EQUAL(m_bgColors.size(), scales::UPPER_STYLE_SCALE + 1, ());
  for (int scale = 0; scale <= scales::UPPER_STYLE_SCALE; ++scale)
  {
    auto const it = bgColorForScale.find(scale);
    m_bgColors[scale] = it != bgColorForScale.end() ? it->second : bgColorDefault;
  }
}

void RulesHolder::InitColors(DrulesFormat const & fmt, size_t variant)
{
  ASSERT(m_colors.empty(), ());
  for (auto const & nc : fmt.namedColors)
    m_colors.emplace(nc.name, fmt.colors[variant][nc.colorIdx]);
}

void RulesHolder::LoadFromFormat(DrulesFormat const & fmt, size_t variant, Classificator & classifTree)
{
  CHECK_LESS(variant, fmt.VariantCount(), ());
  // Per-variant overrides are decoded but not applied yet; fail loudly if a future file uses them.
  CHECK(variant >= fmt.overrides.size() || fmt.overrides[variant].empty(),
        ("Per-variant drules overrides are not supported yet"));
  Clean();

  // std::deque never relocates its elements, so the raw pointers pushed into m_dRules (and cached by
  // Stylist during tile reads) stay valid without pre-sizing.
  RulesLoader(*this, fmt, variant, classifTree).Run();

  // RuleCounts is the writer's exact rule total; the loader emits a subset (a failed runtime selector
  // drops a rule), so overrunning it means the kothic writer and this loader disagree.
  ASSERT_LESS_OR_EQUAL(m_lineRules.size(), fmt.ruleCounts[kRuleLines], ());
  ASSERT_LESS_OR_EQUAL(m_areaRules.size(), fmt.ruleCounts[kRuleAreas], ());
  ASSERT_LESS_OR_EQUAL(m_symbolRules.size(), fmt.ruleCounts[kRuleSymbols], ());
  ASSERT_LESS_OR_EQUAL(m_captionRules.size(), fmt.ruleCounts[kRuleCaptions], ());
  ASSERT_LESS_OR_EQUAL(m_pathtextRules.size(), fmt.ruleCounts[kRulePathTexts], ());
  ASSERT_LESS_OR_EQUAL(m_shieldRules.size(), fmt.ruleCounts[kRuleShields], ());

  InitBackgroundColors(fmt, variant);
  InitColors(fmt, variant);
}

DrulesFormat DecodeRules(MapStyle mapStyle)
{
  auto const & styleReader = GetStyleReader();
  std::string buffer;

  // Prefer a WritableDir/styles/ override, but fall back to the bundled file when the override is
  // missing, carries a foreign magic/version, or fails to decode: a stale/truncated override must
  // not crash the release app (the decoder traps a structurally broken body with a DEBUG-only ASSERT
  // so designers catch it while building). Each attempt decodes into its own DrulesFormat.
  if (styleReader.ReadDrawingRulesOverride(mapStyle, buffer))
  {
    DrulesFormat fmt;
    try
    {
      if (DecodeDrules(buffer, fmt))
        return fmt;
      LOG(LWARNING, ("Ignoring a drules override with a foreign format; using the bundled file."));
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Ignoring a corrupt drules override; using the bundled file:", e.Msg()));
    }
    buffer.clear();
  }

  styleReader.GetDrawingRulesReader(mapStyle).ReadAsString(buffer);
  DrulesFormat fmt;
  CHECK(DecodeDrules(buffer, fmt), ("Invalid bundled drawing rules file"));
  return fmt;
}
}  // namespace drule
