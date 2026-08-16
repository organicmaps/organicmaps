#pragma once

#include <string>
#include <string_view>

namespace editor
{
class XMLFeature;

// Per-field knowledge about the OSM tags the editor writes: which keys can hold a field (its
// aliases), and how a raw OSM value maps into the MWM value domain.
//
// The editor's journal stores values as they appear in the MWM, which is not always what OSM holds:
// data/replaced_tags.txt rewrites whole tags, generator/osm2type.cpp normalizes and composes some of
// them, and generator/osm2meta.cpp strips a trailing slash from a website, turns "40'" into "12.2"
// and drops what it cannot parse. Comparing the two domains directly reports ordinary edits as
// third-party conflicts, so everything that has to compare them goes through Canonicalize() here.

/// Maps a raw OSM value of the field named by @a journalKey into the MWM value domain: the value the
/// map ends up showing, not the output of any single stage of the import. Idempotent. Returns an
/// empty string for a value the map does not keep, and the value verbatim for a key with no policy
/// entry.
std::string Canonicalize(std::string_view journalKey, std::string_view rawValue);

/// True if two raw OSM values mean the same thing for this field. opening_hours is compared
/// semantically ("8:00" == "08:00"), a field whose value is a ';'-separated set (a phone list,
/// cuisines) as a set, so that a reordering is not a change, and every other field as canonicalized
/// strings.
bool ValuesEquivalent(std::string_view journalKey, std::string_view a, std::string_view b);

/// True if the field has an explicit policy entry. Every editable field must have one, otherwise
/// its values are compared in the wrong domain - see the completeness test in editor_tests.
bool HasFieldPolicy(std::string_view journalKey);

/// True if the MWM value of the field is what a single OSM tag holds, so that Canonicalize() can map
/// a server value into the same domain. Where it is not - a composed housenumber, a street name that
/// comes from OM's street matching, a validator gated on the feature type - there is nothing to match
/// an edit against, so ApplyFieldEdit() applies it instead of refusing: a difference it cannot
/// classify is far more likely to be a formatting variant than a third-party edit.
bool HasCanonicalForm(std::string_view journalKey);

enum class FieldWriteStatus
{
  Written,      // a tag was added, changed or removed; the upload is worth sending
  NothingToDo,  // the object already says what the user asked for: a replay, or an absent field cleared
  // The field was cleared and another tag still holds a value for it - one OM never showed the user
  // and therefore must not delete. The write went through, but the field will not look cleared: the
  // value left behind is what the map shows on the next import.
  ClearedButStillStated,
  Ambiguous,        // the field's sources disagree and none of them holds the value the user saw
  Unrepresentable,  // no tag holds the value the user saw, the write would destroy something OM never
                    // showed, or the generator would compose the value back over it
};

struct FieldWriteResult
{
  FieldWriteStatus m_status = FieldWriteStatus::NothingToDo;
  /// Canonical value of the field as the feature holds it, for the upload guard to reconcile a
  /// conflict against - and, for ClearedButStillStated, the value the clear left behind. Empty when
  /// the field is absent, when the sources disagree (Ambiguous), and when the value is composed
  /// rather than held by a tag.
  std::string m_serverValue;
};

/// Applies one field edit to @a feature by changing the tag - or the token inside a tag - that holds
/// @a base, the value OM showed the user. Every other tag is left untouched: OM never moves a value
/// between alias keys and never deletes an alias it did not edit. Two exceptions the import forces:
/// an alias whose value vocabulary is not the field's is migrated to the field's own key, and a
/// housenumber the object composes from tags of its own is written by splitting it back into the very
/// tags it was composed from - never into tags the object does not already carry. An empty
/// @a newValue clears the field, taking away what the user saw and leaving whatever OM's canonical
/// view does not represent. Writes nothing and reports why when the source cannot be located, see
/// FieldWriteStatus.
FieldWriteResult ApplyFieldEdit(XMLFeature & feature, std::string_view journalKey, std::string_view base,
                                std::string_view newValue);

std::string DebugPrint(FieldWriteStatus status);
}  // namespace editor
