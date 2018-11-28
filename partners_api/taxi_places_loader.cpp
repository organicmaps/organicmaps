#include "partners_api/taxi_places_loader.hpp"

#include "platform/platform.hpp"

namespace taxi
{
namespace places
{
SupportedPlaces Loader::LoadFor(Provider::Type const type)
{
  auto const filename = GetFileNameByProvider(type);

  std::string fileData;
  try
  {
    auto const fileReader = GetPlatform().GetReader(filename);
    fileReader->ReadAsString(fileData);
  }
  catch (FileAbsentException const & ex)
  {
    LOG(LERROR, ("Exception while get reader for file:", filename, "reason:", ex.what()));
    return {};
  }
  catch (FileReader::Exception const & ex)
  {
    LOG(LERROR, ("Exception while reading file:", filename, "reason:", ex.what()));
    return {};
  }

  ASSERT(!fileData.empty(), ());

  SupportedPlaces places;

  try
  {
    coding::DeserializerJson des(fileData);
    des(places);
  }
  catch (base::Json::Exception const & ex)
  {
    LOG(LWARNING,
        ("Exception while parsing file:", filename, "reason:", ex.what(), "json:", fileData));
    return {};
  }

  return places;
}

std::string Loader::GetFileNameByProvider(Provider::Type const type)
{
  switch (type)
  {
  case Provider::Type::Maxim: return "taxi_places/maxim.json";
  case Provider::Type::Rutaxi: return "taxi_places/rutaxi.json";
  case Provider::Type::Uber: return "taxi_places/uber.json";
  case Provider::Type::Yandex: return "taxi_places/yandex.json";
  case Provider::Type::Count: LOG(LERROR, ("Incorrect taxi provider")); return "";
  }
  UNREACHABLE();
}
}  // namespace places
}  // namespace taxi
