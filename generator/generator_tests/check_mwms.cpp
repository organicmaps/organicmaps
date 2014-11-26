#include "../../testing/testing.hpp"

#include "../../map/feature_vec_model.hpp"

#include "../../platform/platform.hpp"


UNIT_TEST(CheckMWM_LoadAll)
{
  Platform & pl = GetPlatform();

  Platform::FilesList maps;
  pl.GetFilesByExt(pl.WritableDir(), DATA_FILE_EXTENSION, maps);

  model::FeaturesFetcher m;
  m.InitClassificator();

  for (string const & s : maps)
  {
    try
    {
      m.AddMap(s);
    }
    catch (RootException const & ex)
    {
      TEST(false, ("Bad mwm file:", s));
    }
  }
}
