#include "generator/descriptions_section_builder.hpp"

#include "platform/platform.hpp"

#include "base/string_utils.hpp"

#include <fstream>
#include <iterator>
#include <limits>
#include <sstream>
#include <utility>

namespace
{
bool IsValidDir(std::string const & path)
{
  return Platform::IsFileExistsByFullPath(path) && Platform::IsDirectory(path);
}

std::string GetFileName(std::string path)
{
  base::GetNameFromFullPath(path);
  return path;
}
}  // namespace

namespace generator
{
std::string DescriptionsCollectionBuilderStat::LangStatisticsToString() const
{
  std::stringstream stream;
  stream << "Language statistics - ";
  if (m_langsStat.empty())
    stream << "(empty)";

  for (size_t code = 0; code < m_langsStat.size(); ++code)
  {
    if (m_langsStat[code] == 0)
      continue;

    stream << StringUtf8Multilang::GetLangByCode(static_cast<int8_t>(code))
           << ":" << m_langsStat[code] << " ";
  }

  return stream.str();
}

DescriptionsCollectionBuilder::DescriptionsCollectionBuilder(std::string const & wikipediaDir,
                                                             std::string const & mwmFile)
  : m_wikipediaDir(wikipediaDir), m_mwmFile(mwmFile) {}

// static
std::string DescriptionsCollectionBuilder::MakePath(std::string const & wikipediaDir, std::string wikipediaUrl)
{
  strings::Trim(wikipediaUrl);
  strings::ReplaceFirst(wikipediaUrl, "http://", "");
  strings::ReplaceFirst(wikipediaUrl, "https://", "");
  if (strings::EndsWith(wikipediaUrl, "/"))
    wikipediaUrl.pop_back();

  return base::JoinPath(wikipediaDir, wikipediaUrl);
}

// static
size_t DescriptionsCollectionBuilder::FillStringFromFile(std::string const & fullPath, int8_t code,
                                                         StringUtf8Multilang & str)
{
  std::fstream stream;
  stream.exceptions(std::fstream::failbit | std::fstream::badbit);
  stream.open(fullPath);
  std::string content((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
  auto const contentSize = content.size();
  if (contentSize != 0)
    str.AddString(code, std::move(content));

  return contentSize;
}

boost::optional<size_t> DescriptionsCollectionBuilder::FindPageAndFill(std::string wikipediaUrl,
                                                                       StringUtf8Multilang & str)
{
  auto const path = MakePath(m_wikipediaDir, wikipediaUrl);
  if (!IsValidDir(path))
  {
    LOG(LWARNING, ("Directory", path, "not found."));
    return {};
  }

  size_t size = 0;
  Platform::FilesList filelist;
  Platform::GetFilesByExt(path, ".html", filelist);
  for (auto const & entry : filelist)
  {
    auto const filename = GetFileName(entry);
    auto const lang = base::FilenameWithoutExt(filename);
    auto const code = StringUtf8Multilang::GetLangIndex(lang);
    if (code == StringUtf8Multilang::kUnsupportedLanguageCode)
    {
      LOG(LWARNING, (lang, "is an unsupported language."));
      continue;
    }

    m_stat.IncCode(code);
    auto const fullPath = base::JoinPath(path, filename);
    size += FillStringFromFile(fullPath, code, str);
  }

  return size;
}

size_t DescriptionsCollectionBuilder::GetFeatureDescription(std::string const & wikiUrl, uint32_t featureId,
                                                            descriptions::FeatureDescription & description)
{
  if (wikiUrl.empty())
    return 0;

  StringUtf8Multilang string;
  auto const ret = FindPageAndFill(wikiUrl, string);
  if (!ret || *ret == 0)
    return 0;

  description = descriptions::FeatureDescription(featureId, std::move(string));
  return *ret;
}

void BuildDescriptionsSection(std::string const & wikipediaDir, std::string const & mwmFile)
{
  DescriptionsSectionBuilder<FeatureType>::Build(wikipediaDir, mwmFile);
}
}  // namespace generator
