#include "editor/osm_tag_policy.hpp"

#include "editor/xml_feature.hpp"

#include "indexer/classificator.hpp"
#include "indexer/osm_value_format.hpp"
#include "indexer/validate_and_format_contacts.hpp"

#include "opening_hours/opening_hours.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <span>
#include <vector>

namespace editor
{
namespace
{
std::string_view constexpr kOpeningHoursKey = "opening_hours";
std::string_view constexpr kCuisineKey = "cuisine";
std::string_view constexpr kHouseNumberKey = "addr:housenumber";
std::string_view constexpr kLocalNamePrefix = "name:";

// OSM separates the values of a multi-valued tag with ';'. A cuisine is also split on ',' - by the
// generator (osm2type.cpp), so the editor has to do the same or a cuisine listed after a comma would
// not be a token of its own and could not be taken away.
constexpr char const * kTokenSeparators = ";";
constexpr char const * kCuisineTokenSeparators = ",;";

using Canonicalizer = std::string (*)(std::string const &);

// Identity: the generator stores this tag verbatim today (osm2meta.cpp).
// If import-time normalization is ever added for this field - trimming,
// case folding, Unicode normalization, reformatting - add the same function
// here. The upload guard compares old_value, which is in the MWM value
// domain, against the canonicalized server value; if the two stop agreeing,
// ordinary edits start being reported as third-party conflicts.
std::string Identity(std::string const & v)
{
  return v;
}

// The generator normalizes every cuisine token (osm::NormalizeCuisineToken) and OM keeps the ones it
// has a classifier type for, losing the rest, so the MWM value is a set rather than a string. Sorted
// instead of following the classifier's type order: reproducing that order is not needed to tell an
// edit from a reformatting, and both sides are canonicalized.
std::string CanonicalizeCuisines(std::string const & v)
{
  Classificator const & cl = classif();
  std::vector<std::string> known;
  for (std::string_view const token : strings::Tokenize(v, kCuisineTokenSeparators))
  {
    auto normalized = osm::NormalizeCuisineToken(std::string{token});
    if (!normalized.empty() && cl.GetTypeByPathSafe({kCuisineKey, normalized}) != Classificator::INVALID_TYPE)
      known.push_back(std::move(normalized));
  }
  base::SortUnique(known);
  return strings::JoinStrings(known, kTokenSeparators);
}

// One of the OSM keys that can hold a field.
struct FieldAlias
{
  std::string_view m_key;

  // OM never writes back into this key, because its value vocabulary is not the field's: "wifi=wired"
  // is nonsense, and data/replaced_tags.txt rewrites "wifi=yes|free|no" into "internet_access" before
  // the generator sees the object. A value matched here is migrated instead - written to the field's
  // first key, with this key removed - which is the one place where OM touches a tag other than the
  // one it edited.
  bool m_migrateOnWrite = false;
};

struct FieldPolicy
{
  // Raw OSM value -> MWM value, for the whole tag value. See Identity above for the fields the
  // generator stores verbatim.
  Canonicalizer m_canonicalize = &Identity;

  // Raw token -> canonical token, for a field whose OSM value is a set of independent values. Null
  // means the value is one indivisible unit: it is matched and replaced whole. An empty result means
  // OM cannot represent the token, so it never matches anything the user saw and is therefore never
  // removed.
  Canonicalizer m_canonicalizeToken = nullptr;

  // What the raw value's tokens are separated by. Must be what the field's canonicalizer splits on,
  // or a token it sees would not be a token here and could not be taken away.
  char const * m_tokenSeparators = kTokenSeparators;

  // The field's values add up, so a value that is already there and was never shown to the user - it
  // appeared after the map snapshot, or OM cannot read it - is joined rather than replaced: a place
  // really can serve several cuisines. Where the field states one fact instead, a phone number or an
  // email address, a second value is a third-party conflict and the edit is refused.
  bool m_additive = false;

  // Every OSM key that can hold this field. OM edits whichever of them holds the value the user saw
  // and leaves the rest alone; the first entry is used only to create the tag when the field is
  // absent from the object entirely. Empty means the field lives in the journal key only. Which keys
  // can hold a field is fixed by what folds them on import (the metadata reader, or the generator for
  // the addr:* groups).
  std::span<FieldAlias const> m_aliases;

  // The MWM value is not what one OSM tag holds - a composed housenumber, a street name taken from
  // OM's street matching, a validator gated on the feature type. There is nothing to match an edit
  // against, so the writer falls back to the journal key, see HasCanonicalForm().
  bool m_noCanonicalForm = false;
};

// These aliases come from the generator (osm2type.cpp), not from Metadata::TypeFromString, which
// folds none of them.
FieldAlias constexpr kStreetAliases[] = {{"addr:street"}, {"contact:street"}};
FieldAlias constexpr kHouseNumberAliases[] = {{"addr:housenumber"}, {"contact:housenumber"}};
FieldAlias constexpr kPostcodeAliases[] = {{"addr:postcode"}, {"contact:postcode"}, {"postal_code"}};

FieldAlias constexpr kPhoneAliases[] = {{"phone"}, {"contact:phone"}, {"contact:mobile"}, {"mobile"}};
FieldAlias constexpr kFaxAliases[] = {{"fax"}, {"contact:fax"}};
FieldAlias constexpr kEmailAliases[] = {{"email"}, {"contact:email"}};
FieldAlias constexpr kWebsiteAliases[] = {{"website"}, {"contact:website"}, {"url"}};
FieldAlias constexpr kFacebookAliases[] = {{"contact:facebook"}, {"facebook"}};
FieldAlias constexpr kInstagramAliases[] = {{"contact:instagram"}, {"instagram"}};
FieldAlias constexpr kTwitterAliases[] = {{"contact:twitter"}, {"twitter"}};
FieldAlias constexpr kVkAliases[] = {{"contact:vk"}, {"vk"}};
FieldAlias constexpr kInternetAliases[] = {{"internet_access"}, {.m_key = "wifi", .m_migrateOnWrite = true}};

// The generator composes a housenumber from these (osm2type.cpp): with both a conscription (or
// provisional) number and a street number present it overrides an addr:housenumber that holds no
// '/', and with only one of them present it fills an empty addr:housenumber. They are components of
// the value, not spellings of it, so they must never be aliases: reading one as the housenumber
// would show half an address, and deleting one on write would corrupt it. What OM does write is the
// composition it can invert, see ClassifyHouseNumberWrite().
std::string_view constexpr kConscriptionNumberKey = "addr:conscriptionnumber";
std::string_view constexpr kProvisionalNumberKey = "addr:provisionalnumber";
std::string_view constexpr kStreetNumberKey = "addr:streetnumber";

struct FieldEntry
{
  std::string_view m_key;
  FieldPolicy m_policy;
};

// Every editable field needs an entry, including the ones stored verbatim - see Identity above and
// the completeness test in editor_tests. A field that is not editable has no entry and falls back to
// the identity policy: its value is compared and written under the key that names it.
FieldEntry constexpr kFieldPolicies[] = {
    // Names. "name:<lang>" is resolved by prefix, see FindFieldPolicy().
    {"name", {}},
    {"int_name", {}},
    {"alt_name", {}},
    {"old_name", {}},

    // Address.
    //
    // A housenumber is composed on import, so the MWM value "123/45" can be held by no tag at all -
    // no canonical form. See kConscriptionNumberKey for the components and ClassifyHouseNumberWrite()
    // for how a write is split back into them, or refused where it cannot be.
    {kHouseNumberKey, {.m_aliases = kHouseNumberAliases, .m_noCanonicalForm = true}},
    // A street name does not come from the tag at all: it is the default name of the street feature
    // the object was matched to (ReverseGeocoder::GetFeatureStreetName), which exists even when the
    // object carries no addr:street and can differ from the tag text.
    {"addr:street", {.m_aliases = kStreetAliases, .m_noCanonicalForm = true}},
    // "postal_code" means more than an address postcode elsewhere in OSM, but not on anything OM can
    // edit: every type that offers a postcode field is address-bearing, no boundary, place or highway
    // type is among them, and settlements are disabled in data/editor.config outright.
    {"addr:postcode", {.m_aliases = kPostcodeAliases}},
    {"addr:flats", {}},

    // Verbatim on import, but compared semantically by ValuesEquivalent().
    {kOpeningHoursKey, {}},

    // OSM lists several numbers or addresses in one tag, separated by ';', and each of them is a
    // value in its own right: the editor changes the one the user changed and leaves the rest alone.
    // Not additive, though: the user filling in a phone number meant this is the number, so a second
    // one that appeared meanwhile is a conflict rather than something to co-list.
    {"phone", {.m_canonicalizeToken = &Identity, .m_aliases = kPhoneAliases}},
    // Not editable today (data/editor.config marks it editable="no"), listed for parity with phone.
    {"fax", {.m_canonicalizeToken = &Identity, .m_aliases = kFaxAliases}},
    {"email", {.m_canonicalizeToken = &Identity, .m_aliases = kEmailAliases}},

    {"denomination", {}},
    // The generator's validator for these two is gated on the feature type (osm2meta.cpp), so there
    // is no canonical form a pure function can produce.
    {"operator", {.m_noCanonicalForm = true}},
    {"ele", {.m_noCanonicalForm = true}},
    // The generator rewrites a Wikipedia URL into "lang:Title", but the field is not editable
    // (data/editor.config keeps it commented out) so it cannot appear in a journal. If it becomes
    // editable, move ValidateAndFormat_wikipedia into indexer/osm_value_format.hpp and use it here.
    {"wikipedia", {}},

    {"website", {.m_canonicalize = &osm::ValidateAndFormat_url, .m_aliases = kWebsiteAliases}},
    {"website:menu", {.m_canonicalize = &osm::ValidateAndFormat_url}},

    {"contact:facebook", {.m_canonicalize = &osm::ValidateAndFormat_facebook, .m_aliases = kFacebookAliases}},
    {"contact:instagram", {.m_canonicalize = &osm::ValidateAndFormat_instagram, .m_aliases = kInstagramAliases}},
    {"contact:twitter", {.m_canonicalize = &osm::ValidateAndFormat_twitter, .m_aliases = kTwitterAliases}},
    {"contact:vk", {.m_canonicalize = &osm::ValidateAndFormat_vk, .m_aliases = kVkAliases}},
    {"contact:line", {.m_canonicalize = &osm::ValidateAndFormat_contactLine}},

    // The generator drops a multi-value outright, so the whole-value canonicalizer stays a whitelist
    // of single values and the map shows nothing for "internet_access=terminal;wlan". The tag still
    // legally holds a list, so the writer adds to it rather than overwriting claims OM cannot read.
    {"internet_access",
     {.m_canonicalize = &osm::ValidateAndFormat_internet,
      .m_canonicalizeToken = &osm::ValidateAndFormat_internet,
      .m_additive = true,
      .m_aliases = kInternetAliases}},

    {"stars", {.m_canonicalize = &osm::ValidateAndFormat_stars}},
    {"height", {.m_canonicalize = &osm::ValidateAndFormat_height}},
    {"building:levels", {.m_canonicalize = &osm::ValidateAndFormat_building_levels}},
    // A level holds "1;2" or "3-5", but the user edits it as one string and the generator normalizes
    // it as one string, so it is not a set of independent values.
    {"level", {.m_canonicalize = &osm::ValidateAndFormat_level}},
    {"drive_through", {.m_canonicalize = &osm::ValidateAndFormat_drive_through}},
    {"self_service", {.m_canonicalize = &osm::ValidateAndFormat_self_service}},
    {"outdoor_seating", {.m_canonicalize = &osm::ValidateAndFormat_outdoor_seating}},

    {kCuisineKey,
     {.m_canonicalize = &CanonicalizeCuisines,
      .m_canonicalizeToken = &osm::NormalizeCuisineToken,
      .m_tokenSeparators = kCuisineTokenSeparators,
      .m_additive = true}},
    // Derived from the vegetarian/vegan cuisine types, so the value is "yes" or nothing.
    {"diet:vegetarian", {}},
    {"diet:vegan", {}},
};

FieldPolicy const * FindFieldPolicy(std::string_view journalKey)
{
  if (journalKey.starts_with(kLocalNamePrefix))
    journalKey = "name";

  for (auto const & entry : kFieldPolicies)
    if (entry.m_key == journalKey)
      return &entry.m_policy;

  return nullptr;
}

FieldPolicy const & GetFieldPolicy(std::string_view journalKey)
{
  static FieldPolicy const kDefault{};
  auto const * policy = FindFieldPolicy(journalKey);
  return policy ? *policy : kDefault;
}

std::vector<std::string_view> SplitAndTrim(FieldPolicy const & policy, std::string_view value)
{
  std::vector<std::string_view> tokens;
  for (std::string_view token : strings::Tokenize(value, policy.m_tokenSeparators))
  {
    strings::Trim(token);
    if (!token.empty())
      tokens.push_back(token);
  }
  return tokens;
}

/// @returns the canonical form of one token, or the token itself where OM has no vocabulary for it.
/// An unrepresentable token is never equal to a value the user saw, so it is never taken away, and it
/// is still recognized as a duplicate of itself.
std::string TokenKey(FieldPolicy const & policy, std::string_view token)
{
  ASSERT(policy.m_canonicalizeToken, ("Only a field whose value is a set of values has tokens."));

  auto key = policy.m_canonicalizeToken(std::string{token});
  return key.empty() ? std::string{token} : key;
}

std::vector<std::string> TokenKeys(FieldPolicy const & policy, std::string_view value)
{
  std::vector<std::string> keys;
  for (auto const token : SplitAndTrim(policy, value))
    keys.push_back(TokenKey(policy, token));
  return keys;
}

/// @returns @a raw with the tokens the user took away removed and the ones they added appended. The
/// journal has no per-token entries, so the delta is derived: what @a base holds and @a newValue does
/// not is gone, what @a newValue holds and @a base does not is new. A token nobody touched keeps its
/// place and its spelling, and a token OM cannot represent is in neither set, so it survives both
/// directions. A replaced token loses its position, which is preferable to guessing which addition
/// belongs to which removal.
std::string ApplyTokenDelta(FieldPolicy const & policy, std::string_view raw, std::string_view base,
                            std::string_view newValue)
{
  auto const baseTokens = TokenKeys(policy, base);
  auto const newTokens = TokenKeys(policy, newValue);

  std::vector<std::string> out;
  std::vector<std::string> present;
  for (auto const token : SplitAndTrim(policy, raw))
  {
    auto key = TokenKey(policy, token);
    if (base::IsExist(baseTokens, key) && !base::IsExist(newTokens, key))
      continue;

    out.emplace_back(token);
    present.push_back(std::move(key));
  }

  for (auto const token : SplitAndTrim(policy, newValue))
  {
    auto key = TokenKey(policy, token);
    if (base::IsExist(baseTokens, key) || base::IsExist(present, key))
      continue;

    out.emplace_back(token);
    present.push_back(std::move(key));
  }

  // ';' is what every multi-valued OSM tag is read with, so a value that also allows ',' is written
  // back with the one separator both sides agree on.
  return strings::JoinStrings(out, kTokenSeparators);
}

/// Writes @a value into @a key, or removes the tag when @a value is empty.
/// @returns true if the feature changed - a write that changes nothing is not a write, which is what
/// makes a replay of an already uploaded edit report NothingToDo.
bool WriteTag(XMLFeature & feature, std::string_view key, std::string_view value)
{
  strings::Trim(value);  // XMLFeature::SetTagValue trims as well, so compare what would be stored.

  if (value.empty())
  {
    if (!feature.HasTag(key))
      return false;

    feature.RemoveTag(key);
    return true;
  }

  if (feature.HasTag(key) && feature.GetTagValue(key) == value)
    return false;

  feature.SetTagValue(key, value);
  return true;
}

/// Writes @a value into the tag @a source, or migrates it to the field's first key for an alias OM
/// must not write back into (FieldAlias::m_migrateOnWrite). @returns true if the feature changed.
bool WriteToSource(XMLFeature & feature, std::span<FieldAlias const> aliases, FieldAlias const & source,
                   std::string_view value)
{
  if (!source.m_migrateOnWrite)
    return WriteTag(feature, source.m_key, value);

  bool changed = WriteTag(feature, source.m_key, {});
  changed |= WriteTag(feature, aliases.front().m_key, value);
  return changed;
}

/// @returns a value the object still states for the field - what the map can show for it after the
/// write. Empty when no tag holds the field any more, or when what is left is not something OM can
/// read and therefore never appears on the map.
std::string StatedValue(XMLFeature const & feature, std::span<FieldAlias const> aliases, std::string_view journalKey)
{
  for (auto const & alias : aliases)
  {
    if (!feature.HasTag(alias.m_key))
      continue;

    auto canonical = Canonicalize(journalKey, feature.GetTagValue(alias.m_key));
    if (!canonical.empty())
      return canonical;
  }
  return {};
}

// What the generator's housenumber composition does to a write, for an object that carries the
// components it composes from.
struct HouseNumberWrite
{
  bool m_refuse = false;
  // The halves the new value is split back into, empty unless the object states both components.
  std::string_view m_conscriptionNumber;
  std::string_view m_streetNumber;
};

/// Decides how @a newValue can be written as the housenumber of @a feature, given what the generator
/// composes (osm2type.cpp): with both a conscription (or provisional) number and a street number
/// present it overrides an addr:housenumber that holds no '/', and with one of them present it fills
/// an empty one.
///
/// The components exist on the object or they do not. Where they do not, "addr:housenumber" is an
/// ordinary literal value whatever it looks like - "2/2" and "2/a" are house numbers in their own
/// right in Russia and elsewhere - and OM must never invent a component tag for it. Where they do,
/// the MWM value is the composition, so the only value OM can write is one that splits back into the
/// same two components, which are then updated with it; anything the composition would override or
/// refill is refused instead of being silently reverted on the next import.
///
/// "addr:provisionalnumber" holds a conscription number that has not been assigned yet. OM neither
/// invents nor changes one, and writing "addr:conscriptionnumber" next to it would leave the two
/// racing for the same half of the value, so such an object is refused.
HouseNumberWrite ClassifyHouseNumberWrite(XMLFeature const & feature, std::string_view newValue)
{
  bool const hasProvisionalNumber = feature.HasTag(kProvisionalNumberKey);
  bool const hasConscriptionNumber = feature.HasTag(kConscriptionNumberKey) || hasProvisionalNumber;
  bool const hasStreetNumber = feature.HasTag(kStreetNumberKey);

  if (!hasConscriptionNumber && !hasStreetNumber)
    return {};

  if (!hasConscriptionNumber || !hasStreetNumber)
  {
    // One component fills an empty housenumber, so a cleared value comes straight back. The value
    // itself is not composed, so everything else is an ordinary write, and the lone component is not
    // OM's to complete.
    return {.m_refuse = newValue.empty()};
  }

  if (hasProvisionalNumber)
    return {.m_refuse = true};

  // Anything that is not two halves is overridden by the composition on the next import.
  auto const slash = newValue.find('/');
  if (slash == std::string_view::npos || newValue.find('/', slash + 1) != std::string_view::npos)
    return {.m_refuse = true};

  auto conscriptionNumber = newValue.substr(0, slash);
  auto streetNumber = newValue.substr(slash + 1);
  strings::Trim(conscriptionNumber);
  strings::Trim(streetNumber);
  if (conscriptionNumber.empty() || streetNumber.empty())
    return {.m_refuse = true};

  return {.m_conscriptionNumber = conscriptionNumber, .m_streetNumber = streetNumber};
}
}  // namespace

std::string Canonicalize(std::string_view journalKey, std::string_view rawValue)
{
  return GetFieldPolicy(journalKey).m_canonicalize(std::string{rawValue});
}

bool ValuesEquivalent(std::string_view journalKey, std::string_view a, std::string_view b)
{
  if (journalKey == kOpeningHoursKey)
  {
    osmoh::OpeningHours const lhs{a}, rhs{b};
    // Rule order is significant in the OSM grammar, so "Sa off; Mo-Fr 8-17" is not equal to
    // "Mo-Fr 8-17; Sa off". Reporting such a rewrite as a change is conservative and intended.
    if (lhs.IsValid() && rhs.IsValid())
      return lhs == rhs;
    return a == b;
  }

  auto const canonicalA = Canonicalize(journalKey, a);
  auto const canonicalB = Canonicalize(journalKey, b);
  if (canonicalA == canonicalB)
    return true;

  // The value is a set of independent values, so the order the server lists them in is not a change.
  auto const & policy = GetFieldPolicy(journalKey);
  if (!policy.m_canonicalizeToken)
    return false;

  auto tokensA = SplitAndTrim(policy, canonicalA);
  auto tokensB = SplitAndTrim(policy, canonicalB);
  base::SortUnique(tokensA);
  base::SortUnique(tokensB);
  return tokensA == tokensB;
}

bool HasFieldPolicy(std::string_view journalKey)
{
  return FindFieldPolicy(journalKey) != nullptr;
}

bool HasCanonicalForm(std::string_view journalKey)
{
  return !GetFieldPolicy(journalKey).m_noCanonicalForm;
}

FieldWriteResult ApplyFieldEdit(XMLFeature & feature, std::string_view journalKey, std::string_view base,
                                std::string_view newValue)
{
  ASSERT(!journalKey.empty(), ());

  auto const & policy = GetFieldPolicy(journalKey);
  // Every OSM key that can hold this field; the first one is where the tag is created if none does.
  FieldAlias const journalKeyAlias{journalKey};
  std::span<FieldAlias const> aliases{&journalKeyAlias, 1};
  if (!policy.m_aliases.empty())
  {
    ASSERT_EQUAL(policy.m_aliases.front().m_key, journalKey, ("The journal names a field by the key OM creates."));
    aliases = policy.m_aliases;
  }

  // Where this object states the field, and what it states, for the upload guard to reconcile
  // against. Two aliases holding different values leave the guard nothing to reconcile to.
  std::vector<FieldAlias> sources;
  std::string serverValue;
  bool sourcesDisagree = false;
  for (auto const & alias : aliases)
  {
    if (!feature.HasTag(alias.m_key))
      continue;
    sources.push_back(alias);

    auto canonical = Canonicalize(journalKey, feature.GetTagValue(alias.m_key));
    if (canonical.empty())
      continue;

    if (serverValue.empty())
      serverValue = std::move(canonical);
    else if (!ValuesEquivalent(journalKey, serverValue, canonical))
      sourcesDisagree = true;
  }

  // A housenumber the object composes from tags of its own is written as those tags too, or refused
  // where the composition would take the write back. This is a fact read off the generator, not
  // doubt, so it overrules the fallback below.
  HouseNumberWrite composition;
  if (journalKey == kHouseNumberKey)
  {
    composition = ClassifyHouseNumberWrite(feature, newValue);
    if (composition.m_refuse)
      return {FieldWriteStatus::Unrepresentable, std::move(serverValue)};
  }

  // The components are written with the value they were split from: leaving them behind would keep
  // the object stating the old address, and the composition would put it back as soon as the new
  // housenumber lost its '/'.
  auto const writeComponents = [&feature, &composition]
  {
    if (composition.m_conscriptionNumber.empty())
      return false;

    ASSERT(feature.HasTag(kConscriptionNumberKey) && feature.HasTag(kStreetNumberKey),
           ("A component is only ever updated, never invented: an object without them holds a plain "
            "housenumber, whatever it looks like."));

    bool changed = WriteTag(feature, kConscriptionNumberKey, composition.m_conscriptionNumber);
    changed |= WriteTag(feature, kStreetNumberKey, composition.m_streetNumber);
    return changed;
  };

  // Turns "the feature changed" into the field's verdict. A clear that another alias survives is not
  // a plain success: OM took away what the map showed the user, and the next import shows what is
  // left, so the caller can tell them the field did not go away.
  auto const finish = [&](bool changed) -> FieldWriteResult
  {
    if (!changed)
      return {FieldWriteStatus::NothingToDo, std::move(serverValue)};

    if (newValue.empty())
      if (auto stated = StatedValue(feature, aliases, journalKey); !stated.empty())
        return {FieldWriteStatus::ClearedButStillStated, std::move(stated)};

    return {FieldWriteStatus::Written, std::move(serverValue)};
  };

  // A replay of our own upload, or somebody who made the same edit: the object already says it.
  // Checked before the source is located, because after a successful upload nothing holds `base` any
  // more and the field would look like a third-party conflict forever.
  if (!newValue.empty())
  {
    for (auto const & source : sources)
      if (ValuesEquivalent(journalKey, feature.GetTagValue(source.m_key), newValue))
        return {FieldWriteStatus::NothingToDo, std::move(serverValue)};
  }
  else if (sources.empty())
  {
    // Clearing a field the object does not state: the end state the user asked for already holds.
    return {FieldWriteStatus::NothingToDo, {}};
  }

  // The tags that hold the value OM showed the user - the ones the generator picked, since that is
  // where `base` came from. Empty when the user is adding where the map showed them nothing.
  std::vector<FieldAlias> matched;
  if (!base.empty())
    for (auto const & source : sources)
      if (ValuesEquivalent(journalKey, feature.GetTagValue(source.m_key), base))
        matched.push_back(source);

  if (matched.empty())
  {
    // Nothing holds the field: the one place where the order of the aliases decides anything.
    if (base.empty() && sources.empty())
    {
      bool changed = WriteTag(feature, aliases.front().m_key, newValue);
      changed |= writeComponents();
      return finish(changed);
    }

    // Something holds the field and the map did not show it: OM cannot read it, or it appeared after
    // the user's map snapshot. Where the field's values add up, a token joins the claims instead of
    // overwriting one that was never seen; where it states one fact, the two claims are a conflict.
    if (base.empty() && policy.m_canonicalizeToken && policy.m_additive)
    {
      auto const & source = sources.front();
      auto const out = ApplyTokenDelta(policy, feature.GetTagValue(source.m_key), base, newValue);
      return finish(WriteToSource(feature, aliases, source, out));
    }

    // The MWM value is not what a tag holds, so a missing source classifies nothing: apply the edit
    // to the journal key, which is what the editor did for every field before source matching.
    if (policy.m_noCanonicalForm)
    {
      bool changed = WriteTag(feature, journalKey, newValue);
      changed |= writeComponents();
      return finish(changed);
    }

    // Neither the value the user saw nor a single server value to reconcile it with.
    if (sourcesDisagree)
      return {FieldWriteStatus::Ambiguous, {}};

    return {FieldWriteStatus::Unrepresentable, std::move(serverValue)};
  }

  // Every matching source holds exactly the value the user was editing, so the same delta applies to
  // each of them. Updating only one would leave a stale copy that can win the next import race.
  bool changed = writeComponents();
  for (auto const & source : matched)
  {
    auto const out = policy.m_canonicalizeToken
                       ? ApplyTokenDelta(policy, feature.GetTagValue(source.m_key), base, newValue)
                       : std::string{newValue};
    changed |= WriteToSource(feature, aliases, source, out);
  }

  return finish(changed);
}

std::string DebugPrint(FieldWriteStatus status)
{
  switch (status)
  {
  case FieldWriteStatus::Written: return "Written";
  case FieldWriteStatus::NothingToDo: return "NothingToDo";
  case FieldWriteStatus::ClearedButStillStated: return "ClearedButStillStated";
  case FieldWriteStatus::Ambiguous: return "Ambiguous";
  case FieldWriteStatus::Unrepresentable: return "Unrepresentable";
  }
  UNREACHABLE();
}
}  // namespace editor
