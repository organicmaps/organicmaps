#include "testing/testing.hpp"

#include "editor/editor_config.hpp"
#include "editor/osm_tag_policy.hpp"
#include "editor/xml_feature.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/edit_journal.hpp"
#include "indexer/editable_map_object.hpp"
#include "indexer/feature_meta.hpp"

#include "coding/string_utf8_multilang.hpp"

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace osm_tag_policy_test
{
using editor::ApplyFieldEdit;
using editor::Canonicalize;
using editor::FieldWriteStatus;
using editor::HasCanonicalForm;
using editor::HasFieldPolicy;
using editor::ValuesEquivalent;
using editor::XMLFeature;

XMLFeature MakeNode(std::vector<std::pair<std::string, std::string>> const & tags)
{
  XMLFeature feature(XMLFeature::Type::Node);
  for (auto const & [key, value] : tags)
    feature.SetTagValue(key, value);
  return feature;
}

// @returns the value of @a key, or "-" if the tag is absent, so that both can be tested at once.
std::string TagOrNone(XMLFeature const & feature, std::string_view key)
{
  return feature.HasTag(key) ? feature.GetTagValue(key) : "-";
}

// @returns every journal key the editor can produce, taken from the code that produces them
// (EditableMapObject::LogDiffInJournal): the editable metadata fields, the keys the diff writes
// itself, and one key per name language.
std::vector<std::string> EditableFields()
{
  std::vector<std::string> keys;
  for (auto const type : editor::GetEditableMetadataFields())
    keys.emplace_back(feature::ToString(type));

  for (auto const key : osm::kDirectJournalKeys)
    keys.emplace_back(key);

  for (auto const & language : StringUtf8Multilang::GetSupportedLanguages())
    keys.emplace_back(StringUtf8Multilang::GetOSMTagByCode(StringUtf8Multilang::GetLangIndex(language.m_code)));

  return keys;
}

UNIT_TEST(OsmTagPolicy_CanonicalizeIdentity)
{
  TEST_EQUAL(Canonicalize("name", "Café Ätsch"), "Café Ätsch", ());
  TEST_EQUAL(Canonicalize("name:de", "Freiburg"), "Freiburg", ());
  TEST_EQUAL(Canonicalize("addr:housenumber", "12 a"), "12 a", ());
  TEST_EQUAL(Canonicalize("addr:street", "Улица Ленина"), "Улица Ленина", ());
  TEST_EQUAL(Canonicalize("phone", "+1 555 1234"), "+1 555 1234", ());
  TEST_EQUAL(Canonicalize("email", "a@b.com"), "a@b.com", ());
  TEST_EQUAL(Canonicalize("opening_hours", "Mo-Fr 8:00-17:00"), "Mo-Fr 8:00-17:00", ());
  // Not editable, and unknown keys are passed through unchanged.
  TEST_EQUAL(Canonicalize("nonexistent_tag", "whatever"), "whatever", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeWebsite)
{
  // The generator strips a trailing slash right after the host, and nothing else.
  TEST_EQUAL(Canonicalize("website", "https://organicmaps.app/"), "https://organicmaps.app", ());
  TEST_EQUAL(Canonicalize("website", "https://organicmaps.app"), "https://organicmaps.app", ());
  TEST_EQUAL(Canonicalize("website", "organicmaps.app/"), "organicmaps.app", ());
  TEST_EQUAL(Canonicalize("website", "organicmaps.app/news/"), "organicmaps.app/news/", ());
  TEST_EQUAL(Canonicalize("website:menu", "https://organicmaps.app/"), "https://organicmaps.app", ());
  // The editor's own input formatter adds a missing protocol; canonicalization must not, or every
  // protocol-less website in OSM would look like a third-party edit.
  TEST_EQUAL(Canonicalize("website", "organicmaps.app"), "organicmaps.app", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeInternet)
{
  TEST_EQUAL(Canonicalize("internet_access", "wlan"), "wlan", ());
  TEST_EQUAL(Canonicalize("internet_access", "WLAN"), "wlan", ());
  TEST_EQUAL(Canonicalize("internet_access", "free"), "wlan", ());
  TEST_EQUAL(Canonicalize("internet_access", "wifi"), "wlan", ());
  TEST_EQUAL(Canonicalize("internet_access", "public"), "wlan", ());
  TEST_EQUAL(Canonicalize("internet_access", "terminal"), "terminal", ());
  // The generator drops what it cannot parse, so the MWM holds nothing and the two compare equal.
  TEST_EQUAL(Canonicalize("internet_access", "terminal;wlan"), "", ());
  TEST_EQUAL(Canonicalize("internet_access", "customers"), "", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeNumbers)
{
  TEST_EQUAL(Canonicalize("height", "12"), "12", ());
  TEST_EQUAL(Canonicalize("height", "2 m"), "2", ());
  TEST_EQUAL(Canonicalize("height", "40'"), "12.2", ("Imperial units are converted to metres."));
  TEST_EQUAL(Canonicalize("height", "0"), "", ());

  TEST_EQUAL(Canonicalize("building:levels", "3.0"), "3", ());
  TEST_EQUAL(Canonicalize("building:levels", "４"), "4", ("Full width digits."));
  TEST_EQUAL(Canonicalize("building:levels", "2-5"), "2", ());
  TEST_EQUAL(Canonicalize("building:levels", "many"), "", ());

  TEST_EQUAL(Canonicalize("level", "１"), "1", ("Full width digits."));
  TEST_EQUAL(Canonicalize("level", "1;2"), "1;2", ("A level can be a list or a range."));

  TEST_EQUAL(Canonicalize("stars", "5"), "5", ());
  TEST_EQUAL(Canonicalize("stars", "8"), "", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeYesNo)
{
  TEST_EQUAL(Canonicalize("self_service", "Yes"), "yes", ());
  TEST_EQUAL(Canonicalize("self_service", "partially"), "partially", ());
  TEST_EQUAL(Canonicalize("self_service", "limited"), "", ());
  TEST_EQUAL(Canonicalize("outdoor_seating", "YES"), "yes", ());
  TEST_EQUAL(Canonicalize("drive_through", "No"), "no", ());
  TEST_EQUAL(Canonicalize("drive_through", "only"), "", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeSocial)
{
  TEST_EQUAL(Canonicalize("contact:facebook", "https://facebook.com/organicmaps"), "organicmaps", ());
  TEST_EQUAL(Canonicalize("contact:facebook", "organicmaps"), "organicmaps", ());
  TEST_EQUAL(Canonicalize("contact:instagram", "https://instagram.com/organicmaps"), "organicmaps", ());
  TEST_EQUAL(Canonicalize("contact:twitter", "https://twitter.com/organicmaps"), "organicmaps", ());
  TEST_EQUAL(Canonicalize("contact:twitter", "@organicmaps"), "organicmaps", ());
  TEST_EQUAL(Canonicalize("contact:vk", "https://vk.com/organicmaps"), "organicmaps", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeCuisine)
{
  classificator::Load();

  // A set: sorted, deduplicated, and everything OM has no classifier type for is dropped.
  TEST_EQUAL(Canonicalize("cuisine", "pizza;burger"), "burger;pizza", ());
  TEST_EQUAL(Canonicalize("cuisine", "burger;pizza"), "burger;pizza", ());
  TEST_EQUAL(Canonicalize("cuisine", " pizza ; burger "), "burger;pizza", ());
  TEST_EQUAL(Canonicalize("cuisine", "pizza;pizza"), "pizza", ());
  TEST_EQUAL(Canonicalize("cuisine", "pizza;klingon"), "pizza", ());
  TEST_EQUAL(Canonicalize("cuisine", "klingon"), "", ());

  // Every token is normalized the way the generator normalizes it before the classifier lookup, or
  // the map would show a cuisine the editor cannot find in the tag it came from.
  TEST_EQUAL(Canonicalize("cuisine", "BBQ"), "barbecue", ());
  TEST_EQUAL(Canonicalize("cuisine", "barbeque"), "barbecue", ());
  TEST_EQUAL(Canonicalize("cuisine", "doughnut"), "donut", ());
  TEST_EQUAL(Canonicalize("cuisine", "steak"), "steak_house", ());
  TEST_EQUAL(Canonicalize("cuisine", "coffee"), "coffee_shop", ());
  TEST_EQUAL(Canonicalize("cuisine", "Ice Cream"), "ice_cream", ());
  TEST_EQUAL(Canonicalize("cuisine", "Ice   Cream"), "ice_cream", ("Repeated spaces are collapsed."));
  TEST_EQUAL(Canonicalize("cuisine", "pizza,burger"), "burger;pizza", ("A comma separates as well."));
  TEST_EQUAL(Canonicalize("cuisine", "Pizza, BBQ"), "barbecue;pizza", ());
}

UNIT_TEST(OsmTagPolicy_CanonicalizeRealWorldValues)
{
  classificator::Load();

  // A canonicalizer that drops a value the generator keeps makes the field uneditable on every object
  // that spells it that way: the map shows a value no tag of OM's canonical view holds, so nothing
  // matches what the user saw and the edit is refused. Cuisine was exactly that gap, and only a human
  // reviewer noticed - so the fields OM normalizes on import are pinned here against values OSM
  // really holds, in both directions.
  struct Value
  {
    std::string_view m_key;
    std::string_view m_raw;
    std::string_view m_canonical;
  };

  Value const kKept[] = {
      // Only a slash right after the host is stripped, and only from a lower-cased scheme - which is
      // what the map shows, so the editor must not tidy it either.
      {"website", "https://organicmaps.app", "https://organicmaps.app"},
      {"website", "http://www.example.com/", "http://www.example.com"},
      {"website", "www.example.com", "www.example.com"},
      {"website", "example.com/menu/", "example.com/menu/"},
      {"website", "https://example.com/a/b", "https://example.com/a/b"},
      {"website", "https://example.com?x=1", "https://example.com?x=1"},
      {"website", "https://example.com/#anchor", "https://example.com/#anchor"},
      {"website", "HTTPS://EXAMPLE.COM/", "HTTPS://EXAMPLE.COM/"},

      {"internet_access", "wlan", "wlan"},
      {"internet_access", "WLAN", "wlan"},
      {"internet_access", "yes", "yes"},
      {"internet_access", "no", "no"},
      {"internet_access", "wired", "wired"},
      {"internet_access", "terminal", "terminal"},
      {"internet_access", "free", "wlan"},
      {"internet_access", "wifi", "wlan"},
      {"internet_access", "public", "wlan"},

      // A height is metres, however OSM spelled it.
      {"height", "12", "12"},
      {"height", "12.5", "12.5"},
      {"height", "2 m", "2"},
      {"height", "2m", "2"},
      {"height", "3.5 metres", "3.5"},
      {"height", "12 m (39 ft)", "12"},
      {"height", "40'", "12.2"},
      {"height", "6'6\"", "2"},
      {"height", "10 ft", "3"},
      {"height", "1,5", "1"},

      {"building:levels", "1", "1"},
      {"building:levels", "3.0", "3"},
      {"building:levels", "2.5", "2.5"},
      {"building:levels", "0", "0"},
      {"building:levels", "２", "2"},
      {"building:levels", "2-5", "2"},
      {"building:levels", "1;2", "1"},

      // A level keeps its list or range, with full width digits normalized.
      {"level", "0", "0"},
      {"level", "-1", "-1"},
      {"level", "0.5", "0.5"},
      {"level", "1;2", "1;2"},
      {"level", "3-5", "3-5"},
      {"level", "１", "1"},

      {"stars", "1", "1"},
      {"stars", "5", "5"},
      {"stars", "7", "7"},
      {"stars", "3S", "3"},
      {"stars", "4,5", "4"},

      {"drive_through", "yes", "yes"},
      {"drive_through", "no", "no"},
      {"drive_through", "Yes", "yes"},
      {"self_service", "yes", "yes"},
      {"self_service", "no", "no"},
      {"self_service", "only", "only"},
      {"self_service", "partially", "partially"},
      {"outdoor_seating", "yes", "yes"},
      {"outdoor_seating", "no", "no"},

      // Every spelling the generator folds, and both separators it splits on.
      {"cuisine", "pizza", "pizza"},
      {"cuisine", "ice_cream", "ice_cream"},
      {"cuisine", "Ice Cream", "ice_cream"},
      {"cuisine", "BBQ", "barbecue"},
      {"cuisine", "barbeque", "barbecue"},
      {"cuisine", "doughnut", "donut"},
      {"cuisine", "steak", "steak_house"},
      {"cuisine", "coffee", "coffee_shop"},
      {"cuisine", "pizza;pasta;italian", "italian;pasta;pizza"},
      {"cuisine", "italian,pizza", "italian;pizza"},
      {"cuisine", "Pizza, Pasta", "pasta;pizza"},
      {"cuisine", "pizza;klingon", "pizza"},
  };

  for (auto const & [key, raw, canonical] : kKept)
  {
    TEST_EQUAL(Canonicalize(key, raw), canonical, (key, raw));
    TEST_EQUAL(Canonicalize(key, canonical), canonical, ("Canonicalization must be idempotent", key, raw));
  }

  // Values the generator drops as well, so the map shows nothing for them. The field is then not
  // editable on such an object: the writer has no source to match and refuses rather than overwriting
  // a value OM never showed.
  std::pair<std::string_view, std::string_view> const kDropped[] = {
      {"internet_access", "customers"},
      {"internet_access", "limited"},
      {"internet_access", "service"},
      {"internet_access", "wlan;terminal"},
      {"height", "~10"},
      {"building:levels", "many"},
      {"building:levels", "168"},
      {"stars", "0"},
      {"stars", "8"},
      {"stars", "10"},
      {"drive_through", "only"},
      {"self_service", "limited"},
      {"outdoor_seating", "seasonal"},
      {"cuisine", "Fish & Chips"},
      {"cuisine", "klingon"},
      // OM's classifier spells this one "hotdog" and neither side folds the OSM spelling into it, so
      // the map shows no cuisine at all for such a POI and the field cannot be edited on it. Folding
      // it belongs to the classifier rather than here: it would change what the generator writes.
      {"cuisine", "hot_dog"},
  };

  for (auto const & [key, raw] : kDropped)
    TEST(Canonicalize(key, raw).empty(), (key, raw, Canonicalize(key, raw)));

  // Every cuisine OM knows about, taken from the classifier the editor offers them from: a cuisine
  // the user can pick has to survive canonicalization, or the edit could not be matched to the tag it
  // came from.
  std::vector<std::string_view> const kCuisinePath = {"cuisine"};
  classif().GetObject(classif().GetTypeByPath(kCuisinePath))->ForEachObject([](ClassifObject const & o) {
    TEST_EQUAL(Canonicalize("cuisine", o.GetName()), o.GetName(), ());
  });

  // opening_hours is stored verbatim, so what matters is that a real value parses: an unparseable one
  // falls back to a raw comparison, where a cosmetic server-side rewrite looks like a third-party edit.
  std::pair<std::string_view, std::string_view> const kOpeningHours[] = {
      {"Mo-Fr 08:00-17:00", "Mo-Fr 8:00-17:00"},
      {"Mo-Sa 09:00-20:00; Su off", "Mo-Sa 9:00-20:00;Su off"},
      {"Mo-Fr 09:00-12:00,13:00-18:00", "Mo-Fr 9:00-12:00,13:00-18:00"},
      {"Mo-Fr 07:30-19:00; Sa 08:00-13:00", "Mo-Fr 7:30-19:00; Sa 8:00-13:00"},
      {"Mo-Fr 08:00-17:00; PH off", "Mo-Fr 8:00-17:00; PH off"},
  };

  for (auto const & [value, rewritten] : kOpeningHours)
    TEST(ValuesEquivalent("opening_hours", value, rewritten), (value, rewritten));

  // Known limit, the same one the rule order has: the values are compared as they are written, not by
  // the hours they mean, so an equivalent rewrite is reported as a change.
  TEST(!ValuesEquivalent("opening_hours", "Mo-Su 00:00-24:00", "24/7"), ());
}

UNIT_TEST(OsmTagPolicy_Idempotent)
{
  classificator::Load();

  // The upload guard compares an already canonical old_value against a canonicalized server value,
  // so applying the canonicalizer twice must not change anything.
  std::pair<char const *, char const *> const kValues[] = {
      {"name", "Café"},
      {"addr:housenumber", "12 a"},
      {"phone", "+1 555 1234; +1 555 9999"},
      {"opening_hours", "Mo-Fr 8:00-17:00"},
      {"website", "https://organicmaps.app/"},
      {"website", "organicmaps.app"},
      {"website:menu", "https://organicmaps.app/menu/"},
      {"contact:facebook", "https://facebook.com/organicmaps"},
      {"contact:instagram", "@organicmaps"},
      {"contact:twitter", "https://twitter.com/organicmaps"},
      {"contact:vk", "https://vk.com/organicmaps"},
      {"contact:line", "https://line.me/R/ti/p/organicmaps"},
      {"internet_access", "Free"},
      {"internet_access", "terminal;wlan"},
      {"internet_access", "customers"},
      {"stars", "5"},
      {"stars", "8"},
      {"height", "40'"},
      {"height", "0"},
      {"building:levels", "3.0"},
      {"building:levels", "many"},
      {"level", "１;2"},
      {"self_service", "Yes"},
      {"outdoor_seating", "limited"},
      {"drive_through", "No"},
      {"cuisine", "pizza;burger;klingon"},
      {"cuisine", "BBQ"},
      {"cuisine", "Ice Cream, doughnut"},
      {"cuisine", ""},
  };

  for (auto const & [key, value] : kValues)
  {
    auto const once = Canonicalize(key, value);
    TEST_EQUAL(Canonicalize(key, once), once, (key, value));
  }
}

UNIT_TEST(OsmTagPolicy_ValuesEquivalent)
{
  classificator::Load();

  TEST(ValuesEquivalent("website", "https://organicmaps.app/", "https://organicmaps.app"), ());
  TEST(!ValuesEquivalent("website", "https://organicmaps.app", "https://organicmaps.org"), ());
  TEST(ValuesEquivalent("internet_access", "free", "wlan"), ());
  TEST(ValuesEquivalent("height", "40'", "12.2"), ());
  TEST(ValuesEquivalent("cuisine", "pizza;burger", "burger;pizza"), ());
  TEST(ValuesEquivalent("cuisine", "pizza;klingon", "pizza"), ("Unknown cuisines are not in the map."));
  TEST(ValuesEquivalent("cuisine", "BBQ", "barbecue"), ("The generator folds the spelling."));
  TEST(!ValuesEquivalent("phone", "+1 555 1234", "+1 555 9999"), ());

  // A field whose value is a set of values is compared as a set, so that a server-side reordering is
  // not reported as a change.
  TEST(ValuesEquivalent("phone", "+1 555 1234;+1 555 9999", "+1 555 9999; +1 555 1234"), ());
  TEST(!ValuesEquivalent("phone", "+1 555 1234;+1 555 9999", "+1 555 9999"), ());
  TEST(ValuesEquivalent("email", "a@b.com;c@d.com", "c@d.com;a@b.com"), ());
}

UNIT_TEST(OsmTagPolicy_OpeningHoursEquivalent)
{
  // Compared through the parser, so a cosmetic rewrite is not a change.
  TEST(ValuesEquivalent("opening_hours", "Mo-Fr 8:00-17:00", "Mo-Fr 08:00-17:00"), ());
  TEST(!ValuesEquivalent("opening_hours", "Mo-Fr 8:00-17:00", "Mo-Fr 8:00-18:00"), ());

  // Both sides unparseable: fall back to a raw comparison.
  TEST(ValuesEquivalent("opening_hours", "whenever we feel like it", "whenever we feel like it"), ());
  TEST(!ValuesEquivalent("opening_hours", "whenever we feel like it", "Mo-Fr 8:00-17:00"), ());
  // One side unparseable: raw comparison too, so a fix is reported as a change.
  TEST(!ValuesEquivalent("opening_hours", "Mo-Fr 8:00-17:00", "Mo-Fr 8:00-17:00 xx"), ());

  // Known limit: rule order is significant in the OSM grammar, so reordering is not equal.
  TEST(!ValuesEquivalent("opening_hours", "Sa off; Mo-Fr 8:00-17:00", "Mo-Fr 8:00-17:00; Sa off"), ());
}

UNIT_TEST(OsmTagPolicy_Completeness)
{
  // Every editable field must have a policy entry: a missing one silently compares the OSM and the
  // MWM value domains against each other. The upload path asserts the same thing for every key it
  // actually writes (Editor::UpdateXMLFeatureTags).
  for (auto const & key : EditableFields())
    TEST(HasFieldPolicy(key), ("No policy for the editable field", key));
}

UNIT_TEST(OsmTagPolicy_NoCanonicalForm)
{
  // Exactly these fields hold an MWM value that no single OSM tag carries: a housenumber composed
  // from addr:conscriptionnumber/addr:streetnumber, a street name taken from OM's street matching,
  // and two fields whose generator validator is gated on the feature type. For them the writer has
  // nothing to match an edit against and applies it to the journal key.
  std::set<std::string_view> const kNoCanonicalForm = {"addr:housenumber", "addr:street", "operator", "ele"};

  for (auto const & key : EditableFields())
    TEST_EQUAL(HasCanonicalForm(key), !kNoCanonicalForm.contains(key), (key));
}

UNIT_TEST(OsmTagPolicy_WriteUpdatesTheAliasThatHeldTheValue)
{
  // The tag that holds the value the user saw is the tag the generator picked, so updating it in
  // place is what makes the new value win the next import as well.
  auto feature = MakeNode({{"mobile", "+1 5551234"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "+1 5559999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "mobile"), "+1 5559999", ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "-", ("OM does not move a value between alias keys."));
}

UNIT_TEST(OsmTagPolicy_WriteLeavesOtherAliasesAlone)
{
  // A phone number in another alias that OM never showed the user is not theirs to change: the
  // object keeps stating both claims, and the one the map shows is the one that was edited.
  auto feature = MakeNode({{"phone", "+1 5551234"}, {"contact:mobile", "+1 5555678"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "+1 5559999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5559999", ());
  TEST_EQUAL(TagOrNone(feature, "contact:mobile"), "+1 5555678", ());
}

UNIT_TEST(OsmTagPolicy_WriteUpdatesABareSocialInPlace)
{
  // The duplicate-tag bug fixed at its source: writing "contact:facebook" here would leave the bare
  // key behind, and the object would state two contradicting handles.
  auto feature = MakeNode({{"facebook", "organicmaps"}});
  auto const result = ApplyFieldEdit(feature, "contact:facebook", "organicmaps", "openstreetmap");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "facebook"), "openstreetmap", ());
  TEST_EQUAL(TagOrNone(feature, "contact:facebook"), "-", ());
}

UNIT_TEST(OsmTagPolicy_WriteAliasWifi)
{
  // "wifi" is the one alias OM does not write back into: its vocabulary is not internet_access's, so
  // a matched value is migrated to the canonical key instead of staying where it was.
  auto feature = MakeNode({{"internet_access", "wlan"}, {"wifi", "free"}});
  auto const result = ApplyFieldEdit(feature, "internet_access", "wlan", "wired");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "internet_access"), "wired", ());
  TEST_EQUAL(TagOrNone(feature, "wifi"), "-", ());

  // The same migration when "wifi" is the only key that holds the field.
  auto bare = MakeNode({{"wifi", "wlan"}});
  TEST_EQUAL(ApplyFieldEdit(bare, "internet_access", "wlan", "wired").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(bare, "internet_access"), "wired", ());
  TEST_EQUAL(TagOrNone(bare, "wifi"), "-", ());
}

UNIT_TEST(OsmTagPolicy_WriteDoesNotTidyContradictingAliases)
{
  // Two keys already state different websites. OM edits the one the user saw - which is the one the
  // generator reads, since that is where the edited value came from - and leaves the other alone,
  // even though the object stays contradictory.
  auto feature = MakeNode({{"url", "http://a.com"}, {"contact:website", "http://b.com"}});
  auto const result = ApplyFieldEdit(feature, "website", "http://b.com", "http://c.com");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "contact:website"), "http://c.com", ());
  TEST_EQUAL(TagOrNone(feature, "url"), "http://a.com", ());
  TEST_EQUAL(TagOrNone(feature, "website"), "-", ());
}

UNIT_TEST(OsmTagPolicy_WriteDoesNotTouchOtherKeys)
{
  // The metadata reader matches "operator" by prefix; the writer must not, or it would delete a
  // wikidata id.
  auto feature = MakeNode({{"operator", "Old"}, {"operator:wikidata", "Q42"}, {"name", "Shop"}});
  auto const result = ApplyFieldEdit(feature, "operator", "Old", "New");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "operator"), "New", ());
  TEST_EQUAL(TagOrNone(feature, "operator:wikidata"), "Q42", ());
  TEST_EQUAL(TagOrNone(feature, "name"), "Shop", ());
}

UNIT_TEST(OsmTagPolicy_ClearRemovesOnlyTheValueTheUserSaw)
{
  // The cost of writing only what OM can see: the numbers the user was never shown stay, and one of
  // them becomes what the map shows next. Deleting them would delete data the user never asked
  // about, which is the more damaging of the two failures - so the clear says that it left a value
  // behind instead of reporting a plain success the user would see contradicted on the next update.
  auto feature = MakeNode({{"phone", "+1 5551234"}, {"contact:phone", "+1 5555678"}, {"mobile", "+1 5559999"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "");

  TEST_EQUAL(result.m_status, FieldWriteStatus::ClearedButStillStated, ());
  TEST_EQUAL(result.m_serverValue, "+1 5555678", ("The value the map will show for the field."));
  TEST_EQUAL(TagOrNone(feature, "phone"), "-", ());
  TEST_EQUAL(TagOrNone(feature, "contact:phone"), "+1 5555678", ());
  TEST_EQUAL(TagOrNone(feature, "mobile"), "+1 5559999", ());

  // Nothing is left that the map can show, so the field really is gone.
  auto alone = MakeNode({{"mobile", "+1 5551234"}});
  TEST_EQUAL(ApplyFieldEdit(alone, "phone", "+1 5551234", "").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(alone, "mobile"), "-", ());
}

UNIT_TEST(OsmTagPolicy_ClearKeepsWhatOMCannotRepresent)
{
  classificator::Load();

  // Clearing takes away the cuisines the user saw; "klingon" has no classifier type, was never
  // shown, and is not theirs to delete.
  auto feature = MakeNode({{"cuisine", "pizza;klingon"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "pizza", "").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "cuisine"), "klingon", ());

  // Nothing is left over, so the tag goes.
  auto known = MakeNode({{"cuisine", "pizza;burger"}});
  TEST_EQUAL(ApplyFieldEdit(known, "cuisine", "burger;pizza", "").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(known, "cuisine"), "-", ());

  // Taking one of several cuisines away leaves the others, tag and all.
  auto one = MakeNode({{"cuisine", "pizza;burger"}});
  TEST_EQUAL(ApplyFieldEdit(one, "cuisine", "burger;pizza", "pizza").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(one, "cuisine"), "pizza", ());
}

UNIT_TEST(OsmTagPolicy_ClearKeepsAnUnparseableValue)
{
  // The map shows nothing for a handle OM cannot parse, so clearing the field did not ask for its
  // removal - it is not a source and the clear does not reach it. The field is gone as far as the
  // user is concerned, so this is a plain success, unlike a clear that leaves a readable value.
  auto feature = MakeNode({{"contact:facebook", "Organic Maps Page"}, {"facebook", "organicmaps"}});
  auto const result = ApplyFieldEdit(feature, "contact:facebook", "organicmaps", "");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "contact:facebook"), "Organic Maps Page", ());
  TEST_EQUAL(TagOrNone(feature, "facebook"), "-", ());
}

UNIT_TEST(OsmTagPolicy_ClearAnAbsentFieldDoesNothing)
{
  auto feature = MakeNode({{"name", "Shop"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "");

  TEST_EQUAL(result.m_status, FieldWriteStatus::NothingToDo, ("The end state the user asked for holds."));
  TEST_EQUAL(TagOrNone(feature, "phone"), "-", ());
}

UNIT_TEST(OsmTagPolicy_ClearAValueTheUserDidNotSee)
{
  // Somebody changed the number after the user's map snapshot: deleting it would delete a value they
  // never saw, so the field is refused and the server value is reported for the upload guard.
  auto feature = MakeNode({{"phone", "+1 5559999"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(result.m_serverValue, "+1 5559999", ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5559999", ());
}

UNIT_TEST(OsmTagPolicy_WritePostcodeUpdatesTheTagThatHeldIt)
{
  // F6's hazard - a left-behind "postal_code" winning the import race - cannot arise: the tag OM
  // edits is the one that won it, because that is where the edited value came from.
  auto feature = MakeNode({{"postal_code", "12345"}, {"contact:postcode", "54321"}});
  auto const result = ApplyFieldEdit(feature, "addr:postcode", "12345", "99999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "postal_code"), "99999", ());
  TEST_EQUAL(TagOrNone(feature, "contact:postcode"), "54321", ());
  TEST_EQUAL(TagOrNone(feature, "addr:postcode"), "-", ());

  // The same claim stated twice is edited twice, or the copy left behind could resurrect the old
  // value on the next import. The postcode OM never showed stays, so the clear says so.
  auto cleared = MakeNode({{"addr:postcode", "12345"}, {"contact:postcode", "54321"}, {"postal_code", "12345"}});
  auto const clearResult = ApplyFieldEdit(cleared, "addr:postcode", "12345", "");
  TEST_EQUAL(clearResult.m_status, FieldWriteStatus::ClearedButStillStated, ());
  TEST_EQUAL(clearResult.m_serverValue, "54321", ());
  TEST_EQUAL(TagOrNone(cleared, "addr:postcode"), "-", ());
  TEST_EQUAL(TagOrNone(cleared, "postal_code"), "-", ());
  TEST_EQUAL(TagOrNone(cleared, "contact:postcode"), "54321", ());
}

UNIT_TEST(OsmTagPolicy_WriteUpdatesEveryTagThatHeldTheValue)
{
  auto feature = MakeNode({{"addr:postcode", "12345"}, {"postal_code", "12345"}});
  auto const result = ApplyFieldEdit(feature, "addr:postcode", "12345", "99999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "addr:postcode"), "99999", ());
  TEST_EQUAL(TagOrNone(feature, "postal_code"), "99999", ());
}

UNIT_TEST(OsmTagPolicy_WriteRefusesWhenNothingHoldsTheValue)
{
  // Somebody else changed the phone number, or OM cannot model how the map produced it. The writer
  // cannot tell the two apart, so it writes nothing and reports the server value.
  auto feature = MakeNode({{"phone", "+1 5551111"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "+1 5559999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(result.m_serverValue, "+1 5551111", ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5551111", ("The user's edit is dropped, not misapplied."));
}

UNIT_TEST(OsmTagPolicy_WriteRefusesWhenTheAliasesDisagree)
{
  // Nothing holds the value the user saw and the two keys state different numbers, so there is no
  // single server value the upload guard could reconcile the field against.
  auto feature = MakeNode({{"phone", "+1 5551111"}, {"mobile", "+1 5552222"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "+1 5559999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Ambiguous, ());
  TEST(result.m_serverValue.empty(), (result.m_serverValue));
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5551111", ());
  TEST_EQUAL(TagOrNone(feature, "mobile"), "+1 5552222", ());
}

UNIT_TEST(OsmTagPolicy_WriteStreetUpdatesContactStreetInPlace)
{
  auto feature = MakeNode({{"contact:street", "Hauptstraße"}});
  auto const result = ApplyFieldEdit(feature, "addr:street", "Hauptstraße", "Nebenstraße");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "contact:street"), "Nebenstraße", ());
  TEST_EQUAL(TagOrNone(feature, "addr:street"), "-", ());

  auto tagged = MakeNode({{"addr:street", "Hauptstraße"}});
  TEST_EQUAL(ApplyFieldEdit(tagged, "addr:street", "Hauptstraße", "Nebenstraße").m_status, FieldWriteStatus::Written,
             ());
  TEST_EQUAL(TagOrNone(tagged, "addr:street"), "Nebenstraße", ());
}

UNIT_TEST(OsmTagPolicy_WriteStreetWithNoSourceIsStillWritten)
{
  // A street name comes from OM's street matching, not from the tag, so a missing source classifies
  // nothing: the edit is applied to the journal key rather than refused. Adding a street to an object
  // that has none is the most valuable street edit there is.
  auto absent = MakeNode({{"name", "Shop"}});
  TEST_EQUAL(ApplyFieldEdit(absent, "addr:street", "Hauptstraße", "Nebenstraße").m_status, FieldWriteStatus::Written,
             ());
  TEST_EQUAL(TagOrNone(absent, "addr:street"), "Nebenstraße", ());

  // The tag spells the street differently from the feature OM matched the object to.
  auto differs = MakeNode({{"addr:street", "Hauptstr."}});
  TEST_EQUAL(ApplyFieldEdit(differs, "addr:street", "Hauptstraße", "Nebenstraße").m_status, FieldWriteStatus::Written,
             ());
  TEST_EQUAL(TagOrNone(differs, "addr:street"), "Nebenstraße", ());
}

UNIT_TEST(OsmTagPolicy_WriteOperatorOnANonQualifyingType)
{
  // The generator's operator validator is gated on the feature type, so the map shows nothing for
  // this object and the field has no canonical form: the tag is overwritten, as it was before.
  auto feature = MakeNode({{"operator", "Old"}});
  auto const result = ApplyFieldEdit(feature, "operator", "", "New");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "operator"), "New", ());
}

UNIT_TEST(OsmTagPolicy_WriteHouseNumberWithNoComponents)
{
  // Without the component tags nothing is composed, so the housenumber is an ordinary value whatever
  // it looks like: "2/2" and "2/a" are house numbers in their own right in Russia and elsewhere, and
  // OM must not read a Czech-style composition into them and invent the tags it is made of.
  auto slashed = MakeNode({{"addr:housenumber", "2/2"}});
  TEST_EQUAL(ApplyFieldEdit(slashed, "addr:housenumber", "2/2", "2/3").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(slashed, "addr:housenumber"), "2/3", ());
  TEST_EQUAL(TagOrNone(slashed, "addr:conscriptionnumber"), "-", ());
  TEST_EQUAL(TagOrNone(slashed, "addr:streetnumber"), "-", ());

  // Including when the slash goes away.
  TEST_EQUAL(ApplyFieldEdit(slashed, "addr:housenumber", "2/3", "5").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(slashed, "addr:housenumber"), "5", ());
  TEST_EQUAL(TagOrNone(slashed, "addr:conscriptionnumber"), "-", ());
  TEST_EQUAL(TagOrNone(slashed, "addr:streetnumber"), "-", ());

  auto lettered = MakeNode({{"addr:housenumber", "2/a"}});
  TEST_EQUAL(ApplyFieldEdit(lettered, "addr:housenumber", "2/a", "2/b").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(lettered, "addr:housenumber"), "2/b", ());
  TEST_EQUAL(TagOrNone(lettered, "addr:conscriptionnumber"), "-", ());
  TEST_EQUAL(TagOrNone(lettered, "addr:streetnumber"), "-", ());

  auto plain = MakeNode({{"addr:housenumber", "12"}});
  TEST_EQUAL(ApplyFieldEdit(plain, "addr:housenumber", "12", "14").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(plain, "addr:housenumber"), "14", ());

  // "contact:housenumber" is read by the generator when there is no "addr:housenumber", so it is an
  // alias and is updated in place - without it OM would lose an edit it can make today.
  auto contact = MakeNode({{"contact:housenumber", "12"}});
  TEST_EQUAL(ApplyFieldEdit(contact, "addr:housenumber", "12", "14").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(contact, "contact:housenumber"), "14", ());
  TEST_EQUAL(TagOrNone(contact, "addr:housenumber"), "-", ());
}

UNIT_TEST(OsmTagPolicy_WriteHouseNumberUpdatesItsComponents)
{
  // A Czech-style address: the generator composes the housenumber out of the two component tags, so
  // the MWM value can be held by no tag at all. The edit is written by inverting the composition -
  // leaving the components as they were would keep the object stating the old address, and would put
  // it back on the map as soon as the new housenumber lost its '/'.
  auto composed = MakeNode({{"addr:conscriptionnumber", "223"}, {"addr:streetnumber", "5"}});
  TEST_EQUAL(ApplyFieldEdit(composed, "addr:housenumber", "223/5", "123/6").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(composed, "addr:housenumber"), "123/6", ());
  TEST_EQUAL(TagOrNone(composed, "addr:conscriptionnumber"), "123", ());
  TEST_EQUAL(TagOrNone(composed, "addr:streetnumber"), "6", ());

  // The same when a tag holds the composed value as well.
  auto tagged =
      MakeNode({{"addr:housenumber", "123/45"}, {"addr:conscriptionnumber", "123"}, {"addr:streetnumber", "45"}});
  TEST_EQUAL(ApplyFieldEdit(tagged, "addr:housenumber", "123/45", "123/46").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(tagged, "addr:housenumber"), "123/46", ());
  TEST_EQUAL(TagOrNone(tagged, "addr:conscriptionnumber"), "123", ());
  TEST_EQUAL(TagOrNone(tagged, "addr:streetnumber"), "46", ());

  // Replaying the same edit changes nothing, components and all.
  TEST_EQUAL(ApplyFieldEdit(tagged, "addr:housenumber", "123/45", "123/46").m_status, FieldWriteStatus::NothingToDo,
             ());
  TEST_EQUAL(TagOrNone(tagged, "addr:streetnumber"), "46", ());

  // A housenumber the components contradict is still theirs to fix: the composition is what the map
  // showed, so writing it splits into the components as usual.
  auto contradicting =
      MakeNode({{"addr:housenumber", "99"}, {"addr:conscriptionnumber", "223"}, {"addr:streetnumber", "5"}});
  TEST_EQUAL(ApplyFieldEdit(contradicting, "addr:housenumber", "223/5", "123/6").m_status, FieldWriteStatus::Written,
             ());
  TEST_EQUAL(TagOrNone(contradicting, "addr:housenumber"), "123/6", ());
  TEST_EQUAL(TagOrNone(contradicting, "addr:conscriptionnumber"), "123", ());
  TEST_EQUAL(TagOrNone(contradicting, "addr:streetnumber"), "6", ());
}

UNIT_TEST(OsmTagPolicy_WriteHouseNumberTheGeneratorWouldRecompose)
{
  // What is left of the refusal once the composition can be inverted: a value that is not a
  // composition of two halves, which the generator would override on the next import.
  auto composed = MakeNode({{"addr:conscriptionnumber", "223"}, {"addr:streetnumber", "5"}});
  TEST_EQUAL(ApplyFieldEdit(composed, "addr:housenumber", "223/5", "123").m_status, FieldWriteStatus::Unrepresentable,
             ());
  TEST_EQUAL(TagOrNone(composed, "addr:housenumber"), "-", ());
  TEST_EQUAL(TagOrNone(composed, "addr:conscriptionnumber"), "223", ());

  TEST_EQUAL(ApplyFieldEdit(composed, "addr:housenumber", "223/5", "1/2/3").m_status, FieldWriteStatus::Unrepresentable,
             ("Two slashes are not a composition of two halves."));
  TEST_EQUAL(ApplyFieldEdit(composed, "addr:housenumber", "223/5", "223/").m_status, FieldWriteStatus::Unrepresentable,
             ("An empty half would delete a component."));

  // "addr:provisionalnumber" holds a conscription number that has not been assigned yet, which OM
  // neither invents nor changes, and writing "addr:conscriptionnumber" next to it would leave the
  // two racing for the same half of the value.
  auto provisional =
      MakeNode({{"addr:housenumber", "223/5"}, {"addr:provisionalnumber", "223"}, {"addr:streetnumber", "5"}});
  TEST_EQUAL(ApplyFieldEdit(provisional, "addr:housenumber", "223/5", "123/6").m_status,
             FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(TagOrNone(provisional, "addr:housenumber"), "223/5", ());
  TEST_EQUAL(TagOrNone(provisional, "addr:provisionalnumber"), "223", ());
  TEST_EQUAL(TagOrNone(provisional, "addr:conscriptionnumber"), "-", ());

  // Clearing is repopulated from the components, so it is refused as well.
  TEST_EQUAL(ApplyFieldEdit(composed, "addr:housenumber", "223/5", "").m_status, FieldWriteStatus::Unrepresentable, ());

  // One component is enough to repopulate a cleared housenumber - and is not OM's to complete, so
  // everything else is an ordinary write.
  auto lone = MakeNode({{"addr:housenumber", "223"}, {"addr:conscriptionnumber", "223"}});
  TEST_EQUAL(ApplyFieldEdit(lone, "addr:housenumber", "223", "").m_status, FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(TagOrNone(lone, "addr:housenumber"), "223", ());

  TEST_EQUAL(ApplyFieldEdit(lone, "addr:housenumber", "223", "224/6").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(lone, "addr:housenumber"), "224/6", ());
  TEST_EQUAL(TagOrNone(lone, "addr:conscriptionnumber"), "223", ());
  TEST_EQUAL(TagOrNone(lone, "addr:streetnumber"), "-", ("A missing component is not invented."));
}

UNIT_TEST(OsmTagPolicy_WriteNewTag)
{
  auto feature = MakeNode({{"name", "Shop"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "website", "", "https://organicmaps.app").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "website"), "https://organicmaps.app", ());

  // A key with no policy entry is written as it is named.
  TEST_EQUAL(ApplyFieldEdit(feature, "brand", "", "Organic Maps").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "brand"), "Organic Maps", ());
}

UNIT_TEST(OsmTagPolicy_WriteLocalizedName)
{
  // "name:<lang>" shares the policy of "name" but is a key of its own.
  auto feature = MakeNode({{"name", "Freiburg"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "name:de", "", "Freiburg im Breisgau").m_status, FieldWriteStatus::Written, ());

  TEST_EQUAL(TagOrNone(feature, "name:de"), "Freiburg im Breisgau", ());
  TEST_EQUAL(TagOrNone(feature, "name"), "Freiburg", ());
}

UNIT_TEST(OsmTagPolicy_WriteAComposedOldName)
{
  // The generator joins a name with its dated variants into one MWM value (osm2type.cpp), so no tag
  // holds what the map showed: the edit is refused instead of being written into "old_name", where
  // the variant would be appended to it again. Without a variant the field is edited as usual.
  auto composed = MakeNode({{"old_name", "Leningrad"}, {"old_name:1920", "Petrograd"}});
  TEST_EQUAL(ApplyFieldEdit(composed, "old_name", "Leningrad;Petrograd", "Sankt-Peterburg").m_status,
             FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(TagOrNone(composed, "old_name"), "Leningrad", ());
  TEST_EQUAL(TagOrNone(composed, "old_name:1920"), "Petrograd", ());

  auto plain = MakeNode({{"old_name", "Leningrad"}});
  TEST_EQUAL(ApplyFieldEdit(plain, "old_name", "Leningrad", "Petrograd").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(plain, "old_name"), "Petrograd", ());
}

UNIT_TEST(OsmTagPolicy_WriteIntoAnOccupiedKey)
{
  classificator::Load();

  // The map showed nothing and a key already holds a value: the user is adding a claim to a field
  // whose values add up, not replacing the one they never saw. A place really can serve two cuisines.
  auto joined = MakeNode({{"cuisine", "klingon"}});
  TEST_EQUAL(ApplyFieldEdit(joined, "cuisine", "", "pizza").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(joined, "cuisine"), "klingon;pizza", ());

  // A phone number states one fact: the user filling the field in meant this is the number, so a
  // second one that appeared after their map snapshot is a third-party conflict rather than
  // something to co-list, and overwriting it would destroy what OM never showed.
  auto occupied = MakeNode({{"phone", "+1 5551111"}});
  auto const result = ApplyFieldEdit(occupied, "phone", "", "+1 5559999");
  TEST_EQUAL(result.m_status, FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(result.m_serverValue, "+1 5551111", ());
  TEST_EQUAL(TagOrNone(occupied, "phone"), "+1 5551111", ());

  // The same for a field that is one indivisible value.
  auto social = MakeNode({{"contact:facebook", "organicmaps"}});
  auto const socialResult = ApplyFieldEdit(social, "contact:facebook", "", "openstreetmap");
  TEST_EQUAL(socialResult.m_status, FieldWriteStatus::Unrepresentable, ());
  TEST_EQUAL(socialResult.m_serverValue, "organicmaps", ());
  TEST_EQUAL(TagOrNone(social, "contact:facebook"), "organicmaps", ());

  // Same when the value is one OM cannot read at all.
  auto junk = MakeNode({{"contact:facebook", "Organic Maps Page"}});
  TEST_EQUAL(ApplyFieldEdit(junk, "contact:facebook", "", "openstreetmap").m_status, FieldWriteStatus::Unrepresentable,
             ());
  TEST_EQUAL(TagOrNone(junk, "contact:facebook"), "Organic Maps Page", ());
}

UNIT_TEST(OsmTagPolicy_WriteReplayIsANoOp)
{
  // The upload went through and its response was lost, so the journal is replayed against an object
  // that already holds the value. Nothing holds the old value any more, but that is not a conflict.
  auto feature = MakeNode({{"mobile", "+1 5559999"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234", "+1 5559999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::NothingToDo, ());
  TEST_EQUAL(TagOrNone(feature, "mobile"), "+1 5559999", ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "-", ());

  // The same for an addition: the key the user filled in already says it.
  auto added = MakeNode({{"mobile", "+1 5559999"}});
  TEST_EQUAL(ApplyFieldEdit(added, "phone", "", "+1 5559999").m_status, FieldWriteStatus::NothingToDo, ());
  TEST_EQUAL(TagOrNone(added, "phone"), "-", ());
}

UNIT_TEST(OsmTagPolicy_WriteKeepsUntouchedCuisineTokens)
{
  classificator::Load();

  // "klingon" has no classifier type, so OM never showed it and it is not in the delta at all.
  auto feature = MakeNode({{"cuisine", "pizza;klingon"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "pizza", "pizza;burger").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "cuisine"), "pizza;klingon;burger", ());

  // A token nobody touched keeps its place and its spelling, however OSM spelled it.
  auto spelled = MakeNode({{"cuisine", "Ice Cream;pizza"}});
  TEST_EQUAL(ApplyFieldEdit(spelled, "cuisine", "ice_cream;pizza", "ice_cream;pizza;burger").m_status,
             FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(spelled, "cuisine"), "Ice Cream;pizza;burger", ());

  // Only the token the user took away goes.
  auto removed = MakeNode({{"cuisine", "pizza;burger;klingon"}});
  TEST_EQUAL(ApplyFieldEdit(removed, "cuisine", "burger;pizza", "pizza").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(removed, "cuisine"), "pizza;klingon", ());
}

UNIT_TEST(OsmTagPolicy_WriteEditsACommaSeparatedCuisineList)
{
  classificator::Load();

  // The generator splits a cuisine on ',' as well as on ';', so OM has to split the raw value the
  // same way: a cuisine listed after a comma is a token of its own, and taking it away has to remove
  // it rather than leave the whole value untouched. What is written back uses the ';' that every
  // multi-valued OSM tag is read with.
  auto feature = MakeNode({{"cuisine", "Pizza, BBQ, klingon"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "barbecue;pizza", "pizza").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "cuisine"), "Pizza;klingon", ());

  auto added = MakeNode({{"cuisine", "Pizza, BBQ"}});
  TEST_EQUAL(ApplyFieldEdit(added, "cuisine", "barbecue;pizza", "barbecue;pizza;burger").m_status,
             FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(added, "cuisine"), "Pizza;BBQ;burger", ());

  auto cleared = MakeNode({{"cuisine", "Pizza, BBQ"}});
  TEST_EQUAL(ApplyFieldEdit(cleared, "cuisine", "barbecue;pizza", "").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(cleared, "cuisine"), "-", ());
}

UNIT_TEST(OsmTagPolicy_WriteMatchesACuisineTheGeneratorRenamed)
{
  classificator::Load();

  // The map shows "barbecue" for "cuisine=BBQ". Without the generator's own token normalization the
  // raw token would match nothing, and the cuisine the user deleted would come back on the next map
  // update.
  auto feature = MakeNode({{"cuisine", "BBQ"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "barbecue", "pizza").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "cuisine"), "pizza", ());

  auto listed = MakeNode({{"cuisine", "pizza;BBQ"}});
  TEST_EQUAL(ApplyFieldEdit(listed, "cuisine", "barbecue;pizza", "pizza").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(listed, "cuisine"), "pizza", ());
}

UNIT_TEST(OsmTagPolicy_WriteReplacesOnePhoneNumber)
{
  auto feature = MakeNode({{"phone", "+1 5551234;+1 5555678"}});
  auto const result = ApplyFieldEdit(feature, "phone", "+1 5551234;+1 5555678", "+1 5551234;+1 5559999");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5551234;+1 5559999", ("The untouched number is kept verbatim."));
}

UNIT_TEST(OsmTagPolicy_WriteCarriesUnknownInternetTokens)
{
  // Both tokens are in OM's vocabulary on their own, but the generator drops a multi-value outright,
  // so the map showed nothing here and the user replaced nothing: their value joins the list.
  auto feature = MakeNode({{"internet_access", "terminal;wlan"}});
  auto const result = ApplyFieldEdit(feature, "internet_access", "", "wired");

  TEST_EQUAL(result.m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "internet_access"), "terminal;wlan;wired", ());

  // A token that is already there in another spelling is not added twice, and a write that would
  // change nothing is not a write.
  auto present = MakeNode({{"internet_access", "terminal;Free"}});
  TEST_EQUAL(ApplyFieldEdit(present, "internet_access", "", "wlan").m_status, FieldWriteStatus::NothingToDo, ());
  TEST_EQUAL(TagOrNone(present, "internet_access"), "terminal;Free", ());
}

UNIT_TEST(OsmTagPolicy_WriteTokenDeltaIsIdempotent)
{
  classificator::Load();

  // Applying the same edit twice - which is what a replayed upload does - changes nothing the second
  // time, because a token that is gone is not removed again and one that is there is not added again.
  auto feature = MakeNode({{"cuisine", "pizza;klingon"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "pizza", "pizza;burger").m_status, FieldWriteStatus::Written, ());
  auto const once = TagOrNone(feature, "cuisine");

  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "pizza", "pizza;burger").m_status, FieldWriteStatus::NothingToDo, ());
  TEST_EQUAL(TagOrNone(feature, "cuisine"), once, ());
}

UNIT_TEST(OsmTagPolicy_WriteNeverGainsAList)
{
  // A field whose value is one indivisible unit is matched and replaced whole, so a residue OSM
  // should not have held in the first place (the "4" here) goes with it.
  auto feature = MakeNode({{"stars", "3;4"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "stars", "3", "5").m_status, FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "stars"), "5", ());

  auto website = MakeNode({{"website", "http://a.com;http://b.com"}});
  TEST_EQUAL(ApplyFieldEdit(website, "website", "http://a.com;http://b.com", "http://c.com").m_status,
             FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(website, "website"), "http://c.com", ());
}

UNIT_TEST(OsmTagPolicy_WriteDoesNotDuplicateTheUsersValue)
{
  classificator::Load();

  // The user's value already covers a token the raw value holds.
  auto feature = MakeNode({{"cuisine", "pizza;klingon"}});
  TEST_EQUAL(ApplyFieldEdit(feature, "cuisine", "pizza", "klingon").m_status, FieldWriteStatus::Written, ());

  TEST_EQUAL(TagOrNone(feature, "cuisine"), "klingon", ());
}

UNIT_TEST(OsmTagPolicy_WriteReplaysTwoEditsOfOneField)
{
  // The user changed one field twice, so the upload applies the net change (edit_journal.hpp).
  osm::EditJournal journal;
  journal.AddTagChange("phone", "+1 5552222", "+1 5553333");
  journal.AddTagChange("phone", "+1 5553333", "+1 5554444");

  auto const changes = osm::CollapseTagChanges(journal.GetJournal());
  TEST_EQUAL(changes.size(), 1, ());
  auto const & change = changes.front();

  auto feature = MakeNode({{"phone", "+1 5551111"}, {"mobile", "+1 5552222"}});
  TEST_EQUAL(ApplyFieldEdit(feature, change.key, change.old_value, change.new_value).m_status,
             FieldWriteStatus::Written, ());
  TEST_EQUAL(TagOrNone(feature, "mobile"), "+1 5554444", ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5551111", ());

  // The upload went through but its response was lost, so the same journal is replayed against the
  // object it produced. There is nothing left to do, and no changeset is worth sending.
  TEST_EQUAL(ApplyFieldEdit(feature, change.key, change.old_value, change.new_value).m_status,
             FieldWriteStatus::NothingToDo, ());
  TEST_EQUAL(TagOrNone(feature, "mobile"), "+1 5554444", ());
  TEST_EQUAL(TagOrNone(feature, "phone"), "+1 5551111", ());

  // Replaying the entries one by one instead would report a conflict on a field nobody else touched.
  auto uploaded = MakeNode({{"phone", "+1 5551111"}, {"mobile", "+1 5554444"}});
  TEST_EQUAL(ApplyFieldEdit(uploaded, "phone", "+1 5552222", "+1 5553333").m_status, FieldWriteStatus::Ambiguous, ());
}

UNIT_TEST(OsmTagPolicy_WriteAgreesWithTheGenerator)
{
  classificator::Load();

  // The whole model rests on the editor and the generator agreeing on what a value means: write an
  // edit into a raw OSM value, canonicalize the result, and the map must show what the user asked
  // for.
  struct Edit
  {
    std::string_view m_key;
    std::string_view m_raw;
    std::string_view m_base;
    std::string_view m_newValue;
  };

  Edit const kEdits[] = {
      {"cuisine", "BBQ", "barbecue", "pizza"},
      {"cuisine", "Ice Cream;pizza", "ice_cream;pizza", "ice_cream;pizza;burger"},
      {"cuisine", "Pizza, BBQ", "barbecue;pizza", "barbecue;pizza;burger"},
      {"phone", "+1 5551234;+1 5555678", "+1 5551234;+1 5555678", "+1 5551234;+1 5559999"},
      {"website", "https://a.com/", "https://a.com", "https://b.com/"},
      {"internet_access", "wifi", "wlan", "wired"},
      {"height", "40'", "12.2", "15"},
      {"stars", "3", "3", "5"},
  };

  for (auto const & [key, raw, base, newValue] : kEdits)
  {
    auto feature = MakeNode({{std::string{key}, std::string{raw}}});
    TEST_EQUAL(ApplyFieldEdit(feature, key, base, newValue).m_status, FieldWriteStatus::Written, (key, raw));
    TEST(ValuesEquivalent(key, TagOrNone(feature, key), newValue), (key, raw, TagOrNone(feature, key)));
  }
}
}  // namespace osm_tag_policy_test
