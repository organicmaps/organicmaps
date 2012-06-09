#include "../../testing/testing.hpp"

#include "../feature_builder.hpp"


UNIT_TEST(FBuilder_ManyTypes)
{
  FeatureBuilder1 fb1;

  FeatureParams params;
  params.AddType(70);
  params.AddType(4098);
  params.AddType(6339);
  params.AddType(5379);
  params.AddType(5451);
  params.AddType(5195);
  params.AddType(4122);
  params.AddType(4250);
  params.FinishAddingTypes();

  params.AddHouseNumber("75");
  params.AddHouseName("Best House");
  params.name.AddString(0, "Name");

  fb1.SetParams(params);
  fb1.SetCenter(m2::PointD(0, 0));

  TEST(fb1.DoCorrect(), ());
  TEST(fb1.CheckValid(), ());

  FeatureBuilder1::buffer_t buffer;
  TEST(fb1.PreSerialize(), ());
  fb1.Serialize(buffer);

  FeatureBuilder1 fb2;
  fb2.Deserialize(buffer);

  TEST(fb2.CheckValid(), ());
  TEST_EQUAL(fb1, fb2, ());
}
