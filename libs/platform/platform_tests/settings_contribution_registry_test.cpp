#include "testing/testing.hpp"

#include "platform/settings_contribution/settings_contribution_registry.hpp"

#include <string>
#include <vector>

namespace
{
using namespace settings_contribution;

class FakeContribution : public SettingsContribution
{
public:
  FakeContribution(std::string id, std::string title, std::string section)
    : m_id(std::move(id))
    , m_title(std::move(title))
    , m_section(std::move(section))
  {}

  std::string GetId() const override { return m_id; }
  std::string GetTitle() const override { return m_title; }
  std::string GetSectionId() const override { return m_section; }

  void OnSelected() override { ++m_selectedCount; }
  void OnFilePicked(std::string const & path) override
  {
    ++m_pickedCount;
    m_lastPath = path;
  }

  int m_selectedCount = 0;
  int m_pickedCount = 0;
  std::string m_lastPath;

private:
  std::string m_id;
  std::string m_title;
  std::string m_section;
};
}  // namespace

UNIT_TEST(SettingsContributionRegistry_EmptyByDefault)
{
  SettingsContributionRegistry registry;
  TEST(registry.Contributions().empty(), ());
}

UNIT_TEST(SettingsContributionRegistry_RegisterPreservesOrder)
{
  SettingsContributionRegistry registry;
  FakeContribution a("a", "A", "debug");
  FakeContribution b("b", "B", "debug");
  registry.Register(&a);
  registry.Register(&b);

  TEST_EQUAL(registry.Contributions().size(), 2, ());
  TEST_EQUAL(registry.Contributions()[0]->GetId(), "a", ());
  TEST_EQUAL(registry.Contributions()[1]->GetId(), "b", ());
}

UNIT_TEST(SettingsContributionRegistry_OnSelectedAndOnFilePicked)
{
  FakeContribution c("c", "C", "debug");
  c.OnSelected();
  c.OnFilePicked("/tmp/track.gpx");
  TEST_EQUAL(c.m_selectedCount, 1, ());
  TEST_EQUAL(c.m_pickedCount, 1, ());
  TEST_EQUAL(c.m_lastPath, "/tmp/track.gpx", ());
}
