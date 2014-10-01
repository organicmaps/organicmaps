#include "check_model.hpp"

#include "../defines.hpp"

#include "../indexer/features_vector.hpp"
#include "../indexer/classificator.hpp"
#include "../indexer/feature_visibility.hpp"

#include "../base/logging.hpp"


using namespace feature;

namespace check_model
{
  void ReadFeatures(string const & fName)
  {
    Classificator const & c = classif();

    FilesContainerR cont(fName);

    DataHeader header;
    header.Load(cont.GetReader(HEADER_FILE_TAG));

    FeaturesVector vec(cont, header);
    vec.ForEachOffset([&] (FeatureType const & ft, uint32_t)
    {
      TypesHolder types(ft);

      size_t const count = types.Size();
      vector<uint32_t> vTypes(count);
      for (size_t i = 0; i < count; ++i)
      {
        CHECK_EQUAL(c.GetTypeForIndex(c.GetIndexForType(types[i])), types[i], ());
        vTypes[i] = types[i];
      }

      sort(vTypes.begin(), vTypes.end());
      CHECK(unique(vTypes.begin(), vTypes.end()) == vTypes.end(), ());

      m2::RectD const r = ft.GetLimitRect(FeatureType::BEST_GEOMETRY);
      CHECK(r.IsValid(), ());

      EGeomType const type = ft.GetFeatureType();
      if (type == GEOM_LINE)
        CHECK_GREATER(ft.GetPointsCount(), 1, ());

      IsDrawableLike(vTypes, ft.GetFeatureType());
    });

    LOG(LINFO, ("OK"));
  }
}
