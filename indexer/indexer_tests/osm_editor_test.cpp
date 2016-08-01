#include "indexer/indexer_tests/osm_editor_test.hpp"
#include "testing/testing.hpp"

#include "search/reverse_geocoder.hpp"

#include "indexer/classificator.hpp"
#include "indexer/feature_algo.hpp"
#include "indexer/ftypes_matcher.hpp"
#include "indexer/osm_editor.hpp"
#include "indexer/scales.hpp"

#include "editor/editor_storage.hpp"

using namespace generator::tests_support;

namespace
{
class IsCafeChecker : public ftypes::BaseChecker
{
  IsCafeChecker()
  {
    Classificator const & c = classif();
    m_types.push_back(c.GetTypeByPath({"amenity", "cafe"}));
  }

public:
  static IsCafeChecker const & Instance()
  {
    static const IsCafeChecker instance;
    return instance;
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

template <class func>
void ForEachCafeAtPoint(model::FeaturesFetcher & model, m2::PointD const & mercator, func && fn)
{
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, 0.2 /* rect width */);
  model.ForEachFeature(rect, [&](FeatureType & ft)
  {
    if (IsCafeChecker::Instance()(ft))
    {
      fn(ft);
    }
  }, scales::GetUpperScale());
}

using TFeatureTypeFn = function<void(FeatureType &)>;
void ForEachFeatureAtPoint(model::FeaturesFetcher &  model, TFeatureTypeFn && fn, m2::PointD const & mercator)
{
  constexpr double kSelectRectWidthInMeters = 1.1;
  constexpr double kMetersToLinearFeature = 3;
  constexpr int kScale = scales::GetUpperScale();
  m2::RectD const rect = MercatorBounds::RectByCenterXYAndSizeInMeters(mercator, kSelectRectWidthInMeters);
  model.ForEachFeature(rect, [&](FeatureType & ft)
  {
   switch (ft.GetFeatureType())
   {
     case feature::GEOM_POINT:
       if (rect.IsPointInside(ft.GetCenter()))
         fn(ft);
       break;
     case feature::GEOM_LINE:
       if (feature::GetMinDistanceMeters(ft, mercator) < kMetersToLinearFeature)
         fn(ft);
       break;
     case feature::GEOM_AREA:
     {
       auto limitRect = ft.GetLimitRect(kScale);
       // Be a little more tolerant. When used by editor mercator is given
       // with some error, so we must extend limit rect a bit.
       limitRect.Inflate(MercatorBounds::GetCellID2PointAbsEpsilon(),
                         MercatorBounds::GetCellID2PointAbsEpsilon());
       // Due to floating points accuracy issues (geometry is saved in editor up to 7 digits
       // after dicimal poin) some feature vertexes are threated as external to a given feature.
       constexpr double kFeatureDistanceToleranceInMeters = 1e-2;
       if (limitRect.IsPointInside(mercator) &&
           feature::GetMinDistanceMeters(ft, mercator) <= kFeatureDistanceToleranceInMeters)
       {
         fn(ft);
       }
     }
       break;
     case feature::GEOM_UNDEFINED:
       ASSERT(false, ("case feature::GEOM_UNDEFINED"));
       break;
   }
  }, kScale);
}

void FillEditableMapObject(osm::Editor const & editor, FeatureType const & ft, osm::EditableMapObject & emo)
{
  emo.SetFromFeatureType(ft);
  emo.SetHouseNumber(ft.GetHouseNumber());
  emo.SetEditableProperties(editor.GetEditableProperties(ft));
}
}  // namespace

namespace tests
{
EditorTest::EditorTest()
{
  m_model.InitClassificator();
  m_infoGetter = make_unique<storage::CountryInfoGetterForTesting>();
  InitEditorForTest();
}

EditorTest::~EditorTest()
{
  for (auto const & file : m_files)
    Cleanup(file);
}

void EditorTest::GetFeatureTypeInfoTest()
{
  auto & editor = osm::Editor::Instance();

  {
    MwmSet::MwmId mwmId;
    TEST(!editor.GetFeatureTypeInfo(mwmId, 0), ());
  }

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
  });
  ASSERT(mwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    TEST(!editor.GetFeatureTypeInfo(mwmId, ft.GetID().m_index), ());

    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    editor.SaveEditedFeature(emo);

    auto const fti = editor.GetFeatureTypeInfo(mwmId, ft.GetID().m_index);
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

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
  });
  ASSERT(mwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    FeatureType featureType;
    TEST(!editor.GetEditedFeature(ft.GetID(), featureType), ());

    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    editor.SaveEditedFeature(emo);

    TEST(editor.GetEditedFeature(ft.GetID(), featureType), ());
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

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
  });
  ASSERT(mwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    string street;
    TEST(!editor.GetEditedFeatureStreet(ft.GetID(), street), ());

    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    osm::LocalizedStreet ls{"some street", ""};
    emo.SetStreet(ls);
    editor.SaveEditedFeature(emo);

    TEST(editor.GetEditedFeatureStreet(ft.GetID(), street), ());
    TEST_EQUAL(street, ls.m_defaultName, ());
  });
}

void EditorTest::OriginalFeatureHasDefaultNameTest()
{
  auto & editor = osm::Editor::Instance();

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
  TestCafe unnamedCafe(m2::PointD(2.0, 2.0), "", "en");
  TestCafe secondUnnamedCafe(m2::PointD(3.0, 3.0), "", "en");

  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
    builder.Add(unnamedCafe);
    builder.Add(secondUnnamedCafe);
  });
  ASSERT(mwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    TEST(editor.OriginalFeatureHasDefaultName(ft.GetID()), ());
  });

  ForEachCafeAtPoint(m_model, m2::PointD(2.0, 2.0), [&](FeatureType & ft)
  {
    TEST(!editor.OriginalFeatureHasDefaultName(ft.GetID()), ());
  });

  ForEachCafeAtPoint(m_model, m2::PointD(3.0, 3.0), [&](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);

    StringUtf8Multilang names;
    names.AddString(StringUtf8Multilang::GetLangIndex("en"), "Eng name");
    names.AddString(StringUtf8Multilang::GetLangIndex("default"), "Default name");
    emo.SetName(names);

    editor.SaveEditedFeature(emo);

    TEST(!editor.OriginalFeatureHasDefaultName(ft.GetID()), ());
  });
}

void EditorTest::GetFeatureStatusTest()
{
  auto & editor = osm::Editor::Instance();

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
  TestCafe unnamedCafe(m2::PointD(2.0, 2.0), "", "en");

  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
    builder.Add(unnamedCafe);
  });

  ASSERT(mwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());

    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    editor.SaveEditedFeature(emo);

    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Modified, ());
    editor.MarkFeatureAsObsolete(emo.GetID());
    TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Obsolete, ());
  });

  ForEachCafeAtPoint(m_model, m2::PointD(2.0, 2.0), [&](FeatureType & ft)
  {
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());
    editor.DeleteFeature(ft);
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Deleted, ());
  });

  osm::EditableMapObject emo;
  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), {3.0, 3.0}, mwmId, emo);
  emo.SetHouseNumber("12");
  editor.SaveEditedFeature(emo);

  TEST_EQUAL(editor.GetFeatureStatus(emo.GetID()), osm::Editor::FeatureStatus::Created, ());
}

void EditorTest::IsFeatureUploadedTest()
{
  auto & editor = osm::Editor::Instance();

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");

  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
  });

  ASSERT(mwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    TEST(!editor.IsFeatureUploaded(ft.GetID().m_mwmId, ft.GetID().m_index), ());
  });

  osm::EditableMapObject emo;
  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), {3.0, 3.0}, mwmId, emo);
  emo.SetHouseNumber("12");
  editor.SaveEditedFeature(emo);

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
  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");

  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
  });

  ASSERT(mwmId.GetInfo(), ());

  osm::EditableMapObject emo;
  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), {3.0, 3.0}, mwmId, emo);
  emo.SetHouseNumber("12");
  editor.SaveEditedFeature(emo);

  FeatureType ft;
  editor.GetEditedFeature(emo.GetID().m_mwmId, emo.GetID().m_index, ft);
  editor.DeleteFeature(ft);

  TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Untouched, ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    editor.DeleteFeature(ft);
    TEST_EQUAL(editor.GetFeatureStatus(ft.GetID()), osm::Editor::FeatureStatus::Deleted, ());
  });
}

void EditorTest::ClearAllLocalEditsTest()
{
  auto & editor = osm::Editor::Instance();

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");

  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
  });
  ASSERT(mwmId.GetInfo(), ());

  osm::EditableMapObject emo;
  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), {3.0, 3.0}, mwmId, emo);

  editor.SaveEditedFeature(emo);

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

  TestCafe cafe(m2::PointD(1.0, 1.0), "London Cafe", "en");
  TestCafe unnamedCafe(m2::PointD(2.0, 2.0), "", "en");
  TestCafe someCafe(m2::PointD(3.0, 3.0), "Some cafe", "en");

  auto const mwmId = ConstructTestMwm([&](TestMwmBuilder & builder)
  {
    builder.Add(cafe);
    builder.Add(unnamedCafe);
    builder.Add(someCafe);
  });

  ASSERT(mwmId.GetInfo(), ());

  FeatureID modifiedId, deletedId, obsoleteId, createdId;

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    editor.SaveEditedFeature(emo);

    modifiedId = emo.GetID();
  });

  ForEachCafeAtPoint(m_model, m2::PointD(2.0, 2.0), [&](FeatureType & ft)
  {
    editor.DeleteFeature(ft);
    deletedId = ft.GetID();
  });

  ForEachCafeAtPoint(m_model, m2::PointD(3.0, 3.0), [&](FeatureType & ft)
  {
    editor.MarkFeatureAsObsolete(ft.GetID());
    obsoleteId = ft.GetID();
  });

  osm::EditableMapObject emo;
  editor.CreatePoint(classif().GetTypeByPath({"amenity", "cafe"}), {4.0, 4.0}, mwmId, emo);
  emo.SetHouseNumber("12");
  editor.SaveEditedFeature(emo);
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

  TestCity london(m2::PointD(1.0, 1.0), "London", "en", 100 /* rank */);
  TestCity moscow(m2::PointD(2.0, 2.0), "Moscow", "ru", 100 /* rank */);
  BuildMwm("TestWorld", feature::DataHeader::world,[&](TestMwmBuilder & builder)
  {
    builder.Add(london);
    builder.Add(moscow);
  });

  TestCafe cafeLondon(m2::PointD(1.0, 1.0), "London Cafe", "en");
  auto const gbMwmId = BuildMwm("GB", feature::DataHeader::country, [&](TestMwmBuilder & builder)
  {
    builder.Add(cafeLondon);
  });
  ASSERT(gbMwmId.GetInfo(), ());

  TestCafe cafeMoscow(m2::PointD(2.0, 2.0), "Moscow Cafe", "ru");
  auto const rfMwmId = BuildMwm("RF", feature::DataHeader::country, [&](TestMwmBuilder & builder)
  {
    builder.Add(cafeMoscow);
  });
  ASSERT(rfMwmId.GetInfo(), ());

  ForEachCafeAtPoint(m_model, m2::PointD(1.0, 1.0), [&](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    editor.SaveEditedFeature(emo);
  });

  ForEachCafeAtPoint(m_model, m2::PointD(2.0, 2.0), [&](FeatureType & ft)
  {
    osm::EditableMapObject emo;
    FillEditableMapObject(editor, ft, emo);
    emo.SetBuildingLevels("1");
    editor.SaveEditedFeature(emo);
  });

  TEST(!editor.m_features.empty(), ());
  TEST_EQUAL(editor.m_features.size(), 2, ());

  editor.OnMapDeregistered(gbMwmId.GetInfo()->GetLocalFile());

  TEST_EQUAL(editor.m_features.size(), 1, ());
  auto const editedMwmId = editor.m_features.find(rfMwmId);
  bool result = (editedMwmId != editor.m_features.end());
  TEST(result, ());
}

void EditorTest::Cleanup(platform::LocalCountryFile const & map)
{
  platform::CountryIndexes::DeleteFromDisk(map);
  map.DeleteFromDisk(MapOptions::Map);
}

void EditorTest::InitEditorForTest()
{
  auto & editor = osm::Editor::Instance();

  editor.m_storage = make_unique<editor::StorageMemory>();
  editor.ClearAllLocalEdits();

  editor.SetMwmIdByNameAndVersionFn([this](string const & name) -> MwmSet::MwmId
  {
    return m_model.GetIndex().GetMwmIdByCountryFile(platform::CountryFile(name));
  });

  editor.SetFeatureLoaderFn([this](FeatureID const & fid) -> unique_ptr<FeatureType>
  {
    unique_ptr<FeatureType> feature(new FeatureType());
    Index::FeaturesLoaderGuard const guard(m_model.GetIndex(), fid.m_mwmId);
    if (!guard.GetOriginalFeatureByIndex(fid.m_index, *feature))
      return nullptr;
    feature->ParseEverything();
    return feature;
  });

  editor.SetFeatureOriginalStreetFn([this](FeatureType & ft) -> string
  {
    search::ReverseGeocoder const coder(m_model.GetIndex());
    auto const streets = coder.GetNearbyFeatureStreets(ft);
    if (streets.second < streets.first.size())
      return streets.first[streets.second].m_name;
    return {};
  });

  editor.SetForEachFeatureAtPointFn(bind(ForEachFeatureAtPoint, std::ref(m_model), _1, _2));
}
}  // namespace tests

using tests::EditorTest;

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
