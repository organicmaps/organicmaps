#include "check_model.hpp"

#include "../defines.hpp"

#include "../indexer/features_vector.hpp"

#include "../base/logging.hpp"


namespace check_model
{
  class DoFullRead
  {
  public:
    void operator() (FeatureType const & ft, uint32_t /*pos*/)
    {
      m2::RectD const r = ft.GetLimitRect(FeatureType::BEST_GEOMETRY);
      CHECK(r.IsValid(), ());
    }
  };

  void ReadFeatures(string const & fName)
  {
    try
    {
      FilesContainerR cont(fName);

      feature::DataHeader header;
      header.Load(cont.GetReader(HEADER_FILE_TAG));

      FeaturesVector vec(cont, header);
      vec.ForEachOffset(DoFullRead());
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("Can't open or read file", fName));
    }
  }
}
