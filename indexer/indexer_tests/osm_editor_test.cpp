#include "testing/testing.hpp"

#include "indexer/indexer_tests/osm_editor_test.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/classificator_loader.hpp"
#include "indexer/classificator.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/index_helpers.hpp"

#include "editor/editor_storage.hpp"

#include "platform/platform_tests_support/scoped_file.hpp"

#include "coding/file_name_utils.hpp"

using namespace generator::tests_support;

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

void EditFeature(FeatureType const & ft)
{
  auto & editor = osm::Editor::Instance();

  osm::EditableMapObject emo;
  FillEditableMapObject(editor, ft, emo);
  emo.SetBuildingLevels("1");
  TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
}

void CreateCafeAtPoint(m2::PointD const & point, MwmSet::MwmId const & mwmId, osm::EditableMapObject &emo)
{
  auto & editor = osm::Editor::Instance();

  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), point, mwmId, emo);
  emo.SetHouseNumber("12");
  TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());
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

  auto & editor = osm::Editor::Instance();
  editor.SetIndex(m_index);
  editor.m_storage = make_unique<editor::InMemoryStorage>();
  editor.ClearAllLocalEdits();
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
  ASSERT(mwmId.IsAlive(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor, &mwmId](FeatureType & ft)
  {
    TEST(!editor.GetFeatureTypeInfo(ft.GetID().m_mwmId, ft.GetID().m_index), ());

    EditFeature(ft);

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
  ASSERT(mwmId.IsAlive(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    FeatureType featureType;
    TEST(!editor.GetEditedFeature(ft.GetID(), featureType), ());

    EditFeature(ft);

    TEST(editor.GetEditedFeature(ft.GetID(), featureType), ());

    osm::EditableMapObject savedEmo;
    FillEditableMapObject(editor, featureType, savedEmo);

    TEST_EQUAL(savedEmo.GetBuildingLevels(), "1", ());

    TEST_EQUAL(ft.GetID(), featureType.GetID(), ());
  });
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
  ASSERT(mwmId.IsAlive(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    string street;
    TEST(!editor.GetEditedFeatureStreet(ft.GetID(), street), ());

    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    osm::LocalizedStreet ls{"some street", ""};
    emo.SetStreet(ls);
    TEST_EQUAL(editor.SaveEditedFeature(emo), osm::Editor::SaveResult::SavedSuccessfully, ());

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
  ASSERT(mwmId.IsAlive(), ());

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

  ASSERT(mwmId.IsAlive(), ());

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
    editor.DeleteFeature(ft);
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Deleted, ());
  });

  osm::EditableMapObject emo;  
  CreateCafeAtPoint({3.0, 3.0}, mwmId, emo);

  TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Created, ());
}

void EditorTest::IsFeatureUploadedTest()
{
  auto & editor = osm::Editor::Instance();

  auto const mwmId = ConstructTestMwm([](TestMwmBuilder & builder)
  {
    TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
    builder.Add(cafe);
  });

  ASSERT(mwmId.IsAlive(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    TEST(!editor.IsFeatureUploaded(ft.GetID().m_mwmId, ft.GetID().m_index), ());
  });

  osm::EditableMapObject emo;
  CreateCafeAtPoint({3.0, 3.0}, mwmId, emo);

  TEST(!editor.IsFeatureUploaded(emo.GetID().m_mwmId, emo.GetID().m_index), ());

  pugi::xml_document doc;
  pugi::xml_node root = doc.append_child("mapsme");
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
  xf.SetUploadStatus("Uploaded");
  xf.AttachToParentNode(created);
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
  });

  ASSERT(mwmId.IsAlive(), ());

  osm::EditableMapObject emo;
  CreateCafeAtPoint({3.0, 3.0}, mwmId, emo);

  FeatureType ft;
  editor.GetEditedFeature(emo.GetID().m_mwmId, emo.GetID().m_index, ft);
  editor.DeleteFeature(ft);

  TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    editor.DeleteFeature(ft);
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
  });
  ASSERT(mwmId.IsAlive(), ());

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
  });

  ASSERT(mwmId.IsAlive(), ());

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
    editor.DeleteFeature(ft);
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
  ASSERT(gbMwmId.IsAlive(), ());

  auto const rfMwmId = BuildMwm("RF", [](TestMwmBuilder & builder)
  {
    TestCafe cafeMoscow(m2::PointD(2.0, 2.0), "Moscow Cafe", "en");
    builder.Add(cafeMoscow);
  });
  ASSERT(rfMwmId.IsAlive(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    EditFeature(ft);
  });

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [](FeatureType & ft)
  {
    EditFeature(ft);
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
  ASSERT(mwmId.IsAlive(), ());

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

  ASSERT(mwmId.IsAlive(), ());

  TEST(!editor.HaveMapEditsOrNotesToUpload(), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    EditFeature(ft);
  });

  TEST(editor.HaveMapEditsOrNotesToUpload(), ());
  editor.ClearAllLocalEdits();
  TEST(!editor.HaveMapEditsOrNotesToUpload(), ());

  platform::tests_support::ScopedFile sf("test_notes.xml");

  editor.m_notes = Notes::MakeNotes(sf.GetFullPath(), true);

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [&editor](FeatureType & ft)
  {
    using NoteType = osm::Editor::NoteProblemType;
    editor.CreateNote({1.0, 1.0}, ft.GetID(), NoteType::PlaceDoesNotExist, "exploded");
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
  ASSERT(gbMwmId.IsAlive(), ());

  auto const rfMwmId = BuildMwm("RF", [](TestMwmBuilder & builder)
  {
    TestCafe cafeMoscow(m2::PointD(2.0, 2.0), "Moscow Cafe", "ru");
    builder.Add(cafeMoscow);
  });
  ASSERT(rfMwmId.IsAlive(), ());

  TEST(!editor.HaveMapEditsToUpload(gbMwmId), ());
  TEST(!editor.HaveMapEditsToUpload(rfMwmId), ());

  ForEachCafeAtPoint(m_index, m2::PointD(1.0, 1.0), [](FeatureType & ft)
  {
    EditFeature(ft);
  });

  TEST(editor.HaveMapEditsToUpload(gbMwmId), ());
  TEST(!editor.HaveMapEditsToUpload(rfMwmId), ());

  ForEachCafeAtPoint(m_index, m2::PointD(2.0, 2.0), [](FeatureType & ft)
  {
    EditFeature(ft);
  });

  TEST(editor.HaveMapEditsToUpload(gbMwmId), ());
  TEST(editor.HaveMapEditsToUpload(rfMwmId), ());
}

void EditorTest::Cleanup(platform::LocalCountryFile const & map)
{
  platform::CountryIndexes::DeleteFromDisk(map);
  map.DeleteFromDisk(MapOptions::Map);
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
}  // namespace
