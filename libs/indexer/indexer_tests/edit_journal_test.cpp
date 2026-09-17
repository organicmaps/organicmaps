#include "testing/testing.hpp"

#include "indexer/edit_journal.hpp"

#include <ctime>
#include <string>
#include <vector>

namespace edit_journal_test
{
using osm::CollapseTagChanges;
using osm::EditJournal;
using osm::JournalEntry;
using osm::TagModData;

std::vector<JournalEntry> MakeJournal(std::vector<TagModData> const & changes)
{
  EditJournal journal;
  for (auto const & change : changes)
    journal.AddTagChange(change.key, change.old_value, change.new_value);
  return journal.GetJournal();
}

UNIT_TEST(EditJournal_CollapseTagChangesKeepsTheNetChange)
{
  // The upload has to apply what the user ended up with, not every step they took: the value the map
  // showed them before the first edit, and the value they left the field at.
  auto const collapsed =
      CollapseTagChanges(MakeJournal({{"phone", "+1 5551111", "+1 5552222"}, {"phone", "+1 5552222", "+1 5553333"}}));

  TEST_EQUAL(collapsed.size(), 1, ());
  TEST_EQUAL(collapsed[0].key, "phone", ());
  TEST_EQUAL(collapsed[0].old_value, "+1 5551111", ());
  TEST_EQUAL(collapsed[0].new_value, "+1 5553333", ());
}

UNIT_TEST(EditJournal_CollapseTagChangesDropsAnUndoneEdit)
{
  auto const collapsed =
      CollapseTagChanges(MakeJournal({{"phone", "+1 5551111", "+1 5552222"}, {"phone", "+1 5552222", "+1 5551111"}}));

  TEST(collapsed.empty(), ("A field left as it was found is not an edit."));
}

UNIT_TEST(EditJournal_CollapseTagChangesKeepsFirstAppearanceOrder)
{
  auto const collapsed = CollapseTagChanges(MakeJournal(
      {{"phone", "+1 5551111", "+1 5552222"}, {"website", "a.com", "b.com"}, {"phone", "+1 5552222", "+1 5553333"}}));

  TEST_EQUAL(collapsed.size(), 2, ());
  TEST_EQUAL(collapsed[0].key, "phone", ());
  TEST_EQUAL(collapsed[0].new_value, "+1 5553333", ());
  TEST_EQUAL(collapsed[1].key, "website", ());
  TEST_EQUAL(collapsed[1].new_value, "b.com", ());
}

UNIT_TEST(EditJournal_CollapseTagChangesIgnoresOtherEntries)
{
  EditJournal journal;
  journal.AddJournalEntry(
      {osm::JournalEntryType::ObjectCreated, std::time(nullptr), osm::ObjCreateData{0, feature::GeomType::Point, {}}});
  journal.AddTagChange("phone", "", "+1 5551111");

  auto const collapsed = CollapseTagChanges(journal.GetJournal());

  TEST_EQUAL(collapsed.size(), 1, ());
  TEST_EQUAL(collapsed[0].key, "phone", ());
}
}  // namespace edit_journal_test
