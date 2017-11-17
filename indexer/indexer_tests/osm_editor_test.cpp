#include "testing/testing.hpp"

#include "indexer/indexer_tests/osm_editor_test.hpp"

#include "search/editor_delegate.hpp"

#include "indexer/classificator.hpp"
#include "indexer/classificator_loader.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/index_helpers.hpp"
#include "indexer/indexer_tests_support/helpers.hpp"
#include "indexer/osm_editor.hpp"

#include "editor/editor_storage.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_name_utils.hpp"

#include "std/unique_ptr.hpp"

using namespace generator::tests_support;
using namespace indexer::tests_support;
using platform::tests_support::ScopedFile;

namespace
{
class IsCafeChecker : public ftypes::BaseChecker
{
public:
  static IsCafeChecker const & Instance()
  {
    static const IsCafeChecker instance;
    return instance;
  }

private:
  IsCafeChecker()
  {
    Classificator const & c = classif();
    m_types.push_back(c.GetTypeByPath({"amenity", "cafe"}));
  }
};

class TestCafe : public TestPOI
{
public:
  TestCafe(m2::PointD const & center, string const & name, string const & lang)
    : TestPOI(center, name, lang)
  {
    SetTypes({{"amenity", "cafe"}});
  }
};

template <typename TFn>
void ForEachCafeAtPoint(Index & index, m2::PointD const & mercator, TFn && fn)
{
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, 0.2 /* rect width */);

  auto const f = [&fn](FeatureType & ft)
  {
    if (IsCafeChecker::Instance()(ft))
    {
      fn(ft);
    }
  };

  index.ForEachInRect(f, rect, scales::GetUpperScale());
}

void FillEditableMapObject(osm::Editor const & editor, FeatureType const & ft, osm::EditableMapObject & emo)
{
  emo.SetFromFeatureType(ft);
  emo.SetHouseNumber(ft.GetHouseNumber());
  emo.SetEditableProperties(editor.GetEditableProperties(ft));
}

void SetBuildingLevelsToOne(FeatureType const & ft)
{
  EditFeature(ft, [](osm::EditableMapObject & emo)
  {
    emo.SetBuildingLevels("1"); // change something
  });
}

void CreateCafeAtPoint(m2::PointD const & point, MwmSet::MwmId const & mwmId, osm::EditableMapObject &emo)
{
  auto & editor = osm::Editor::Instance();

  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), point, mwmId, emo);
  emo.SetHouseNumber("12");
  TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
}

void GenerateUploadedFeature(MwmSet::MwmId const & mwmId,
                             osm::EditableMapObject const & emo,
                             pugi::xml_document & out)
{
  auto & editor = osm::Editor::Instance();

  pugi::xml_node root = out.append_child("mapsme");
  root.append_attribute("format_version") = 1;

  pugi::xml_node mwmNode = root.append_child("mwm");
  mwmNode.append_attribute("name") = mwmId.GetInfo()->GetCountryName().c_str();
  mwmNode.append_attribute("version") = static_cast<long long>(mwmId.GetInfo()->GetVersion());
  pugi::xml_node created = mwmNode.append_child("create");

  FeatureType ft;
  editor.GetEditedFeature(emo.GetID().m_mwmId, emo.GetID().m_index, ft);

  editor::XMLFeature xf = ft.ToXML(true);
  xf.SetMWMFeatureIndex(ft.GetID().m_index);
  xf.SetModificationTime(time(nullptr));
  xf.SetUploadTime(time(nullptr));
  xf.SetUploadStatus("Uploaded");
  xf.AttachToParentNode(created);
}

template <typename T>
uint32_t CountFeaturesInRect(MwmSet::MwmId const & mwmId, m2::RectD const & rect)
{
  auto & editor = osm::Editor::Instance();
  int unused = 0;
  uint32_t counter = 0;
  editor.ForEachFeatureInMwmRectAndScale(mwmId, [&counter](T const & ft)
  {
    ++counter;
  }, rect, unused);

  return counter;
}
}  // namespace

namespace editor
{
namespace testing
{
EditorTest::EditorTest()
{
  try
  {
    classificator::Load();
  }
  catch (RootException const & e)
  {
    LOG(LERROR, ("Classificator read error: ", e.what()));
  }

  indexer::tests_support::SetUpEditorForTesting(make_unique<search::EditorDelegate>(m_index));
}

EditorTest::~EditorTest()
{
  for (auto const & file : m_mwmFiles)
    Cleanup(file);
}

void EditorTest::GetFeatureTypeInfoTest()
{
  auto & editor = osm::Editor::Instance();

  {
    MwmSet::MwmId mwmId;
    TEST(!editor.GetFeatureTypeInfo(mwmId, 0), ());
  }

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor, &mwmId](FeatureType & ft)
  {
    TEST(!editor.GetFeatureTypeInfo(ft.GetID().m_mwmId, ft.GetID().m_index), ());

    SetBuildingLevelsToOne(ft);

    auto const fti = editor.GetFeatureTypeInfo(ft.GetID().m_mwmId, ft.GetID().m_index);
    TEST_NOT_EQUAL(fti, 0, ());
    TEST_EQUAL(fti->m_feature.GetID(), ft.GetID(), ());
  });
}

void EditorTest::GetEditedFeatureTest()
{
  auto & editor = osm::Editor::Instance();

  {
    FeatureID feature;
    FeatureType ft;
    TEST(!editor.GetEditedFeature(feature, ft), ());
  }

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    FeatureType featureType;
    TEST(!editor.GetEditedFeature(ft.GetID(), featureType), ());

    SetBuildingLevelsToOne(ft);

    TEST(editor.GetEditedFeature(ft.GetID(), featureType), ());

    osm::EditableMapObject savedEmo;
    FillEditableMapObject(editor, featureType, savedEmo);

    TEST_EQUAL(savedEmo.GetBuildingLevels(), "1", ());

    TEST_EQUAL(ft.GetID(), featureType.GetID(), ());
  });
}

void EditorTest::SetIndexTest()
{
  // osm::Editor::SetIndex was called in constructor.
  auto & editor = osm::Editor::Instance();

  auto const gbMwmId = BuildMwm("GB", [](TestMwmBuilder & builder)
  {
    TestCafe cafe({1.0, 1.0}, "London Cafe", "en");
    TestStreet street({{0.0, 0.0}, {1.0, 1.0}, {2.0, 2.0}}, "Test street", "en");
    cafe.SetStreet(street);
    builder.Add(street);
    builder.Add(cafe);
    builder.Add(TestCafe({3.0, 3.0}, "London Cafe", "en"));
    builder.Add(TestCafe({4.0, 4.0}, "London Cafe", "en"));
    builder.Add(TestCafe({4.0, 4.0}, "London Cafe", "en"));
    builder.Add(TestCafe({4.0, 4.0}, "London Cafe", "en"));
  });

  auto const mwmId = editor.GetMwmIdByMapName("GB");

  TEST_EQUAL(gbMwmId, mwmId, ());

  osm::EditableMapObject emo;
  CreateCafeAtPoint({2.0, 2.0}, gbMwmId, emo);

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    auto const firstPtr = editor.GetOriginalFeature(ft.GetID());
    TEST(firstPtr, ());
    SetBuildingLevelsToOne(ft);
    auto const secondPtr = editor.GetOriginalFeature(ft.GetID());
    TEST(secondPtr, ());
    TEST_EQUAL(firstPtr->GetID(), secondPtr->GetID(), ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    TEST_EQUAL(editor.GetOriginalFeatureStreet(ft), "Test street", ());

    EditFeature(ft, [](osm::EditableMapObject & emo)
    {
      osm::LocalizedStreet ls{"Some street", ""};
      emo.SetStreet(ls);
    });
    TEST_EQUAL(editor.GetOriginalFeatureStreet(ft), "Test street", ());
  });

  uint32_t counter = 0;
  editor.ForEachFeatureAtPoint([&counter](FeatureType & ft)
  {
    ++counter;
  }, {100.0, 100.0});

  TEST_EQUAL(counter, 0, ());

  counter = 0;
  editor.ForEachFeatureAtPoint([&counter](FeatureType & ft)
  {
    ++counter;
  }, {3.0, 3.0});

  TEST_EQUAL(counter, 1, ());

  counter = 0;
  editor.ForEachFeatureAtPoint([&counter](FeatureType & ft)
  {
    ++counter;
  }, {1.0, 1.0});

  TEST_EQUAL(counter, 2, ());

  counter = 0;
  editor.ForEachFeatureAtPoint([&counter](FeatureType & ft)
  {
    ++counter;
  }, {4.0, 4.0});

  TEST_EQUAL(counter, 3, ());
}

void EditorTest::GetEditedFeatureStreetTest()
{
  auto & editor = osm::Editor::Instance();

  {
    FeatureID feature;
    string street;
    TEST(!editor.GetEditedFeatureStreet(feature, street), ());
  }

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    string street;
    TEST(!editor.GetEditedFeatureStreet(ft.GetID(), street), ());

    osm::LocalizedStreet ls{"some street", ""};
    EditFeature(ft, [&ls](osm::EditableMapObject & emo)
    {
      emo.SetStreet(ls);
    });

    TEST(editor.GetEditedFeatureStreet(ft.GetID(), street), ());
    TEST_EQUAL(street, ls.m_defaultName, ());
  });
}

void EditorTest::OriginalFeatureHasDefaultNameTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    TestCafe unnamedCafe(m2::PointD(2.0, 2.0), "", "en");
    TestCafe secondUnnamedCafe(m2::PointD(3.0, 3.0), "", "en");

    builder.Add(cafe);
    builder.Add(unnamedCafe);
    builder.Add(secondUnnamedCafe);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    TEST(editor.OriginalFeatureHasDefaultName(ft.GetID()), ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [&editor](FeatureType & ft)
  {
    TEST(!editor.OriginalFeatureHasDefaultName(ft.GetID()), ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(3.0, 3.0), [&editor](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);

    StringUtf8Multilang names;
    names.AddString(StringUtf8Multilang::GetLangIndex("en"), "Eng name");
    names.AddString(StringUtf8Multilang::GetLangIndex("default"), "Default name");
    emo.SetName(names);

    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());

    TEST(!editor.OriginalFeatureHasDefaultName(ft.GetID()), ());
  });
}

void EditorTest::GetFeatureStatusTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    TestCafe unnamedCafe(m2::PointD(2.0, 2.0), "", "en");

    builder.Add(cafe);
    builder.Add(unnamedCafe);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());

    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());

    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Modified, ());
    editor.MarkFeatureAsObsolete(emo.GetID());
    TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Obsolete, ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [&editor](FeatureType & ft)
  {
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());
    editor.DeleteFeature(ft.GetID());
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Deleted, ());
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({1.5, 1.5}, mwmId, emo);

  TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Created, ());
}

void EditorTest::IsFeatureUploadedTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    TEST(!editor.IsFeatureUploaded(ft.GetID().m_mwmId, ft.GetID().m_index), ());
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({3.0, 3.0}, mwmId, emo);

  TEST(!editor.IsFeatureUploaded(emo.GetID().m_mwmId, emo.GetID().m_index), ());

  pugi::xml_document doc;
  GenerateUploadedFeature(mwmId, emo, doc);
  editor.m_storage->Save(doc);
  editor.LoadMapEdits();

  TEST(editor.IsFeatureUploaded(emo.GetID().m_mwmId, emo.GetID().m_index), ());
}

void EditorTest::DeleteFeatureTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({3.0, 3.0}, mwmId, emo);

  FeatureType ft;
  editor.GetEditedFeature(emo.GetID().m_mwmId, emo.GetID().m_index, ft);
  editor.DeleteFeature(ft.GetID());

  TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    editor.DeleteFeature(ft.GetID());
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Deleted, ());
  });
}

void EditorTest::ClearAllLocalEditsTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({3.0, 3.0}, mwmId, emo);

  TEST(!editor.m_features.empty(), ());
  editor.ClearAllLocalEdits();
  TEST(editor.m_features.empty(), ());
}

void EditorTest::GetFeaturesByStatusTest()
{
  auto & editor = osm::Editor::Instance();

  {
    MwmSet::MwmId mwmId;
    auto const features = editor.GetFeaturesByStatus(mwmId, osm::Editor::FeatureStatus::Untouched);
    TEST(features.empty(), ());
  }

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    TestCafe unnamedCafe(m2::PointD(2.0, 2.0), "", "en");
    TestCafe someCafe(m2::PointD(3.0, 3.0), "Some cafe", "en");

    builder.Add(cafe);
    builder.Add(unnamedCafe);
    builder.Add(someCafe);

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));
  });

  FeatureID modifiedId, deletedId, obsoleteId, createdId;

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor, &modifiedId](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());

    modifiedId = emo.GetID();
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [&editor, &deletedId](FeatureType & ft)
  {
    editor.DeleteFeature(ft.GetID());
    deletedId = ft.GetID();
  });

  ForEachCafeAtPoint(m_index, m2::PointD(3.0, 3.0), [&editor, &obsoleteId](FeatureType & ft)
  {
    editor.MarkFeatureAsObsolete(ft.GetID());
    obsoleteId = ft.GetID();
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({4.0, 4.0}, mwmId, emo);
  createdId = emo.GetID();

  auto const modified = editor.GetFeaturesByStatus(mwmId, osm::Editor::FeatureStatus::Modified);
  auto const deleted = editor.GetFeaturesByStatus(mwmId, osm::Editor::FeatureStatus::Deleted);
  auto const obsolete = editor.GetFeaturesByStatus(mwmId, osm::Editor::FeatureStatus::Obsolete);
  auto const created = editor.GetFeaturesByStatus(mwmId, osm::Editor::FeatureStatus::Created);

  TEST_EQUAL(modified.size(), 1, ());
  TEST_EQUAL(deleted.size(), 1, ());
  TEST_EQUAL(obsolete.size(), 1, ());
  TEST_EQUAL(created.size(), 1, ());

  TEST_EQUAL(modified.front(), modifiedId.m_index, ());
  TEST_EQUAL(deleted.front(), deletedId.m_index, ());
  TEST_EQUAL(obsolete.front(), obsoleteId.m_index, ());
  TEST_EQUAL(created.front(), createdId.m_index, ());
}

void EditorTest::OnMapDeregisteredTest()
{
  auto & editor = osm::Editor::Instance();

  auto const gbMwmId = BuildMwm("GB", [](TestMwmBuilder & builder)
  {
    TestCafe cafeLondon(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafeLondon);
  });

  auto const rfMwmId = BuildMwm("RF", [](TestMwmBuilder & builder)
  {
    TestCafe cafeMoscow(m2::PointD(2.0, 2.0), "Moscow Cafe", "en");
    builder.Add(cafeMoscow);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  TEST(!editor.m_features.empty(), ());
  TEST_EQUAL(editor.m_features.size(), 2, ());

  editor.OnMapDeregistered(gbMwmId.GetInfo()->GetLocalFile());

  TEST_EQUAL(editor.m_features.size(), 1, ());
  auto const editedMwmId = editor.m_features.find(rfMwmId);
  bool result = (editedMwmId != editor.m_features.end());
  TEST(result, ());
}

void EditorTest::RollBackChangesTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);
  });

  const string houseNumber = "4a";

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor, &houseNumber](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetHouseNumber(houseNumber);
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&houseNumber](FeatureType & ft)
  {
    TEST_EQUAL(ft.GetHouseNumber(), houseNumber, ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    editor.RollBackChanges(ft.GetID());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    TEST_EQUAL(ft.GetHouseNumber(), "", ());
  });
}

void EditorTest::HaveMapEditsOrNotesToUploadTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);
  });

  TEST(!editor.HaveMapEditsOrNotesToUpload(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  TEST(editor.HaveMapEditsOrNotesToUpload(), ());
  editor.ClearAllLocalEdits();
  TEST(!editor.HaveMapEditsOrNotesToUpload(), ());

  ScopedFile sf("test_notes.xml", ScopedFile::Mode::DoNotCreate);

  editor.m_notes = Notes::MakeNotes(sf.GetFullPath(), true);

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    using NoteType = osm::Editor::NoteProblemType;
    feature::TypesHolder typesHolder;
    string defaultName;
    editor.CreateNote({1.0, 1.0}, ft.GetID(), typesHolder, defaultName, NoteType::PlaceDoesNotExist,
                      "exploded");
  });

  TEST(editor.HaveMapEditsOrNotesToUpload(), ());
}

void EditorTest::HaveMapEditsToUploadTest()
{
  auto & editor = osm::Editor::Instance();

  auto const gbMwmId = BuildMwm("GB", [](TestMwmBuilder & builder)
  {
    TestCafe cafeLondon(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafeLondon);
  });

  auto const rfMwmId = BuildMwm("RF", [](TestMwmBuilder & builder)
  {
    TestCafe cafeMoscow(m2::PointD(2.0, 2.0), "Moscow Cafe", "ru");
    builder.Add(cafeMoscow);
  });

  TEST(!editor.HaveMapEditsToUpload(gbMwmId), ());
  TEST(!editor.HaveMapEditsToUpload(rfMwmId), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  TEST(editor.HaveMapEditsToUpload(gbMwmId), ());
  TEST(!editor.HaveMapEditsToUpload(rfMwmId), ());

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  TEST(editor.HaveMapEditsToUpload(gbMwmId), ());
  TEST(editor.HaveMapEditsToUpload(rfMwmId), ());
}

void EditorTest::GetStatsTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(2.0, 2.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(3.0, 3.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(4.0, 4.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(5.0, 5.0), "London Cafe", "en"));

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));
  });

  auto stats = editor.GetStats();

  TEST(stats.m_edits.empty(), ());
  TEST_EQUAL(stats.m_uploadedCount, 0, ());
  TEST_EQUAL(stats.m_lastUploadTimestamp, my::INVALID_TIME_STAMP, ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  stats = editor.GetStats();
  TEST_EQUAL(stats.m_edits.size(), 1, ());

  ForEachCafeAtPoint(m_index, m2::PointD(4.0, 4.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  stats = editor.GetStats();
  TEST_EQUAL(stats.m_edits.size(), 2, ());

  ForEachCafeAtPoint(m_index, m2::PointD(5.0, 5.0), [](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
  });

  stats = editor.GetStats();
  TEST_EQUAL(stats.m_edits.size(), 3, ());
  TEST_EQUAL(stats.m_uploadedCount, 0, ());
  TEST_EQUAL(stats.m_lastUploadTimestamp, my::INVALID_TIME_STAMP, ());

  osm::EditableMapObject emo;
  CreateCafeAtPoint({6.0, 6.0}, mwmId, emo);
  pugi::xml_document doc;
  GenerateUploadedFeature(mwmId, emo, doc);
  editor.m_storage->Save(doc);
  editor.LoadMapEdits();

  stats = editor.GetStats();
  TEST_EQUAL(stats.m_edits.size(), 1, ());
  TEST_EQUAL(stats.m_uploadedCount, 1, ());
  TEST_NOT_EQUAL(stats.m_lastUploadTimestamp, my::INVALID_TIME_STAMP, ());
}

void EditorTest::IsCreatedFeatureTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe", "en"));

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    TEST(!editor.IsCreatedFeature(ft.GetID()), ());
    SetBuildingLevelsToOne(ft);
    TEST(!editor.IsCreatedFeature(ft.GetID()), ());
  });

  osm::EditableMapObject emo;
  TEST(!editor.IsCreatedFeature(emo.GetID()), ());
  CreateCafeAtPoint({2.0, 2.0}, mwmId, emo);
  TEST(editor.IsCreatedFeature(emo.GetID()), ());
}

void EditorTest::ForEachFeatureInMwmRectAndScaleTest()
{
  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe", "en"));

    builder.Add(TestPOI(m2::PointD(100, 100), "Corner Post", "default"));
  });

  {
    osm::EditableMapObject emo;
    CreateCafeAtPoint({10.0, 10.0}, mwmId, emo);
  }
  {
    osm::EditableMapObject emo;
    CreateCafeAtPoint({20.0, 20.0}, mwmId, emo);
  }
  {
    osm::EditableMapObject emo;
    CreateCafeAtPoint({22.0, 22.0}, mwmId, emo);
  }

  TEST_EQUAL(CountFeaturesInRect<FeatureType>(mwmId, {0.0, 0.0, 2.0, 2.0}), 0, ());
  TEST_EQUAL(CountFeaturesInRect<FeatureType>(mwmId, {9.0, 9.0, 21.0, 21.0}), 2, ());
  TEST_EQUAL(CountFeaturesInRect<FeatureType>(mwmId, {9.0, 9.0, 23.0, 23.0}), 3, ());

  TEST_EQUAL(CountFeaturesInRect<FeatureID>(mwmId, {0.0, 0.0, 2.0, 2.0}), 0, ());
  TEST_EQUAL(CountFeaturesInRect<FeatureID>(mwmId, {9.0, 9.0, 21.0, 21.0}), 2, ());
  TEST_EQUAL(CountFeaturesInRect<FeatureID>(mwmId, {9.0, 9.0, 23.0, 23.0}), 3, ());
}

void EditorTest::CreateNoteTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(2.0, 2.0), "Cafe", "en"));
  });

  auto const createAndCheckNote = [&editor](FeatureID const & fId, ms::LatLon const & pos,
                                            osm::Editor::NoteProblemType const noteType) {
    ScopedFile sf("test_notes.xml", ScopedFile::Mode::DoNotCreate);
    editor.m_notes = Notes::MakeNotes(sf.GetFullPath(), true);
    feature::TypesHolder holder;
    holder.Assign(classif().GetTypeByPath({"amenity", "restaurant"}));
    string defaultName = "Test name";
    editor.CreateNote(pos, fId, holder, defaultName, noteType, "with comment");

    auto notes = editor.m_notes->GetNotes();
    TEST_EQUAL(notes.size(), 1, ());
    TEST(notes.front().m_point.EqualDxDy(pos, 1e-10), ());
    TEST_NOT_EQUAL(notes.front().m_note.find("with comment"), string::npos, ());
    TEST_NOT_EQUAL(notes.front().m_note.find("OSM data version"), string::npos, ());
    TEST_NOT_EQUAL(notes.front().m_note.find("restaurant"), string::npos, ());
    TEST_NOT_EQUAL(notes.front().m_note.find("Test name"), string::npos, ());
  };

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor, &createAndCheckNote](FeatureType & ft)
  {
    createAndCheckNote(ft.GetID(), {1.0, 1.0}, osm::Editor::NoteProblemType::PlaceDoesNotExist);

    auto notes = editor.m_notes->GetNotes();
    TEST_NOT_EQUAL(notes.front().m_note.find(osm::Editor::kPlaceDoesNotExistMessage), string::npos, ());
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Obsolete, ());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [&editor, &createAndCheckNote](FeatureType & ft)
  {
    createAndCheckNote(ft.GetID(), {2.0, 2.0}, osm::Editor::NoteProblemType::General);

    TEST_NOT_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Obsolete, ());
    auto notes = editor.m_notes->GetNotes();
    TEST_EQUAL(notes.front().m_note.find(osm::Editor::kPlaceDoesNotExistMessage), string::npos, ());
  });
}

void EditorTest::LoadMapEditsTest()
{
  auto & editor = osm::Editor::Instance();

  auto const gbMwmId = BuildMwm("GB", [](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(0.0, 0.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe", "en"));

    builder.Add(TestPOI(m2::PointD(100, 100), "Corner Post", "default"));
  });

  auto const rfMwmId = BuildMwm("RF", [](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(2.0, 2.0), "Moscow Cafe1", "en"));
    builder.Add(TestCafe(m2::PointD(7.0, 7.0), "Moscow Cafe2", "en"));
    builder.Add(TestCafe(m2::PointD(4.0, 4.0), "Moscow Cafe3", "en"));
    builder.Add(TestCafe(m2::PointD(6.0, 6.0), "Moscow Cafe4", "en"));

    builder.Add(TestPOI(m2::PointD(100, 100), "Corner Post", "default"));    
  });

  vector<FeatureID> features;

  ForEachCafeAtPoint(m_index, m2::PointD(0.0, 0.0), [&features](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
    features.emplace_back(ft.GetID());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor, &features](FeatureType & ft)
  {
    editor.DeleteFeature(ft.GetID());
    features.emplace_back(ft.GetID());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [&editor, &features](FeatureType & ft)
  {
    editor.MarkFeatureAsObsolete(ft.GetID());
    features.emplace_back(ft.GetID());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(7.0, 7.0), [&features](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
    features.emplace_back(ft.GetID());
  });

  ForEachCafeAtPoint(m_index, m2::PointD(4.0, 4.0), [&editor, &features](FeatureType & ft)
  {
    editor.DeleteFeature(ft.GetID());
    features.emplace_back(ft.GetID());
  });
  
  ForEachCafeAtPoint(m_index, m2::PointD(6.0, 6.0), [&features](FeatureType & ft)
  {
    SetBuildingLevelsToOne(ft);
    features.emplace_back(ft.GetID());
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({5.0, 5.0}, rfMwmId, emo);
  features.emplace_back(emo.GetID());

  editor.Save();
  editor.LoadMapEdits();

  auto const fillLoaded = [&editor](vector<FeatureID> & loadedFeatures)
  {
    loadedFeatures.clear();
    for (auto const & mwm : editor.m_features)
    {
      for (auto const & index : mwm.second)
      {
        loadedFeatures.emplace_back(index.second.m_feature.GetID());
      }
    }
  };

  vector<FeatureID> loadedFeatures;
  fillLoaded(loadedFeatures);

  sort(loadedFeatures.begin(), loadedFeatures.end());
  sort(features.begin(), features.end());
  TEST_EQUAL(features, loadedFeatures, ());

  TEST(RemoveMwm(rfMwmId), ());

  auto const newRfMwmId = BuildMwm("RF", [](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(2.0, 2.0), "Moscow Cafe1", "en"));
    builder.Add(TestCafe(m2::PointD(7.0, 7.0), "Moscow Cafe2", "en"));
    builder.Add(TestCafe(m2::PointD(4.0, 4.0), "Moscow Cafe3", "en"));
    builder.Add(TestCafe(m2::PointD(6.0, 6.0), "Moscow Cafe4", "en"));
  }, 1);

  editor.LoadMapEdits();
  fillLoaded(loadedFeatures);

  TEST_EQUAL(features.size(), loadedFeatures.size(), ());

  m_index.DeregisterMap(m_mwmFiles.back().GetCountryFile());
  TEST(RemoveMwm(newRfMwmId), ());

  TEST_EQUAL(editor.m_features.size(), 2, ());

  editor.LoadMapEdits();
  fillLoaded(loadedFeatures);

  TEST_EQUAL(editor.m_features.size(), 1, ());
  TEST_EQUAL(loadedFeatures.size(), 2, ());

  osm::EditableMapObject gbEmo;
  CreateCafeAtPoint({3.0, 3.0}, gbMwmId, gbEmo);

  pugi::xml_document doc;
  GenerateUploadedFeature(gbMwmId, gbEmo, doc);
  editor.m_storage->Save(doc);
  editor.LoadMapEdits();
  fillLoaded(loadedFeatures);

  TEST_EQUAL(editor.m_features.size(), 1, ());
  TEST_EQUAL(loadedFeatures.size(), 1, ());

  m_index.DeregisterMap(m_mwmFiles.back().GetCountryFile());
  TEST(RemoveMwm(gbMwmId), ());
  
  auto const newGbMwmId = BuildMwm("GB", [](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(0.0, 0.0), "London Cafe", "en"));
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe", "en"));
  }, 1);

  newGbMwmId.GetInfo()->m_version.SetSecondsSinceEpoch(time(nullptr) + 1);

  editor.LoadMapEdits();
  TEST(editor.m_features.empty(), ());
}

void EditorTest::SaveEditedFeatureTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    builder.Add(TestCafe(m2::PointD(1.0, 1.0), "London Cafe1", "en"));

    builder.Add(TestPOI(m2::PointD(10, 10), "Corner Post", "default"));    
  });

  osm::EditableMapObject emo;

  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), {4.0, 4.0}, mwmId, emo);
  emo.SetHouseNumber("12");
  TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Untouched, ());
  TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
  TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Created, ());
  TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::NothingWasChanged, ());
  TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Created, ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::NothingWasChanged, ());
    TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Untouched, ());
    emo.SetHouseNumber("4a");
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
    TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Modified, ());
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::NothingWasChanged, ());
    TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Modified, ());
    emo.SetHouseNumber("");
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
    TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Untouched, ());
  });
}

void EditorTest::Cleanup(platform::LocalCountryFile const & map)
{
  platform::CountryIndexes::DeleteFromDisk(map);
  map.DeleteFromDisk(MapOptions::Map);
}

bool EditorTest::RemoveMwm(MwmSet::MwmId const & mwmId)
{
  auto const & file = mwmId.GetInfo()->GetLocalFile();
  auto const it = find(m_mwmFiles.begin(), m_mwmFiles.end(), file);

  if (it == m_mwmFiles.end())
    return false;

  Cleanup(*it);
  m_mwmFiles.erase(it);
  return true;
}
}  // namespace testing
}  // namespace editor

namespace
{
using editor::testing::EditorTest;

UNIT_CLASS_TEST(EditorTest, GetFeatureTypeInfoTest)
{
  EditorTest::GetFeatureTypeInfoTest();
}

UNIT_CLASS_TEST(EditorTest, OriginalFeatureHasDefaultNameTest)
{ 
  EditorTest::OriginalFeatureHasDefaultNameTest();
}

UNIT_CLASS_TEST(EditorTest, SetIndexTest)
{
  EditorTest::SetIndexTest();
}

UNIT_CLASS_TEST(EditorTest, GetFeatureStatusTest)
{
  EditorTest::GetFeatureStatusTest();
}

UNIT_CLASS_TEST(EditorTest, IsFeatureUploadedTest)
{
  EditorTest::IsFeatureUploadedTest();
}

UNIT_CLASS_TEST(EditorTest, DeleteFeatureTest)
{
  EditorTest::DeleteFeatureTest();
}

UNIT_CLASS_TEST(EditorTest, ClearAllLocalEditsTest)
{
  EditorTest::ClearAllLocalEditsTest();
}

UNIT_CLASS_TEST(EditorTest, GetEditedFeatureStreetTest)
{
  EditorTest::GetEditedFeatureStreetTest();
}

UNIT_CLASS_TEST(EditorTest, GetEditedFeatureTest)
{
  EditorTest::GetEditedFeatureTest();
}

UNIT_CLASS_TEST(EditorTest, GetFeaturesByStatusTest)
{
  EditorTest::GetFeaturesByStatusTest();
}

UNIT_CLASS_TEST(EditorTest, OnMapDeregisteredTest)
{
  EditorTest::OnMapDeregisteredTest();
}

UNIT_CLASS_TEST(EditorTest, RollBackChangesTest)
{
  EditorTest::RollBackChangesTest();
}

UNIT_CLASS_TEST(EditorTest, HaveMapEditsOrNotesToUploadTest)
{
  EditorTest::HaveMapEditsOrNotesToUploadTest();
}

UNIT_CLASS_TEST(EditorTest, HaveMapEditsToUploadTest)
{
  EditorTest::HaveMapEditsToUploadTest();
}

UNIT_CLASS_TEST(EditorTest, GetStatsTest)
{
  EditorTest::GetStatsTest();
}

UNIT_CLASS_TEST(EditorTest, IsCreatedFeatureTest)
{
  EditorTest::IsCreatedFeatureTest();
}

UNIT_CLASS_TEST(EditorTest, ForEachFeatureInMwmRectAndScaleTest)
{
  EditorTest::ForEachFeatureInMwmRectAndScaleTest();
}

UNIT_CLASS_TEST(EditorTest, CreateNoteTest)
{
  EditorTest::CreateNoteTest();
}
UNIT_CLASS_TEST(EditorTest, LoadMapEditsTest) { EditorTest::LoadMapEditsTest(); }
UNIT_CLASS_TEST(EditorTest, SaveEditedFeatureTest)
{
  EditorTest::SaveEditedFeatureTest();
}
}  // namespace
