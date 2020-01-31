#include "generator/isolines_section_builder.hpp"

#include "indexer/isolines_info.hpp"

#include "topography_generator/isolines_utils.hpp"
#include "topography_generator/utils/contours.hpp"
#include "topography_generator/utils/contours_serdes.hpp"

namespace generator
{
using namespace topography_generator;

void BuildIsolinesInfoSection(std::string const & isolinesPath, std::string const & country,
                              std::string const & mwmFile)
{
  Contours<Altitude> isolines;
  auto const countryIsolinesPath = GetIsolinesFilePath(country, isolinesPath);
  if (!LoadContours(countryIsolinesPath, isolines) || isolines.m_contours.empty())
  {
    LOG(LWARNING, ("Section", ISOLINES_INFO_FILE_TAG, "was not created for", mwmFile));
    return;
  }

  isolines::IsolinesInfo info(isolines.m_minValue, isolines.m_maxValue, isolines.m_valueStep);

  FilesContainerW cont(mwmFile, FileWriter::OP_WRITE_EXISTING);
  auto writer = cont.GetWriter(ISOLINES_INFO_FILE_TAG);
  isolines::Serializer serializer(std::move(info));
  serializer.Serialize(*writer);

  LOG(LINFO, ("Section", ISOLINES_INFO_FILE_TAG, "is built for", mwmFile));
}
}  // namespace generator
