#include "country.hpp"

#include "../defines.hpp"

#include "../version/version.hpp"

#include "../platform/platform.hpp"

#include "../indexer/data_header.hpp"

#include "../coding/streams_sink.hpp"
#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"
#include "../coding/file_container.hpp"

#include "../base/logging.hpp"
#include "../base/std_serialization.hpp"
#include "../base/string_utils.hpp"
#include "../base/timer.hpp"


namespace storage
{
  /// Simple check - compare url size with real file size on disk
  bool IsFileDownloaded(CountryFile const & file)
  {
    uint64_t size = 0;
    if (!GetPlatform().GetFileSize(GetPlatform().WritablePathForFile(file.first), size))
      return false;
    return true;//tile.second == size;
  }

  struct CountryBoundsCalculator
  {
    m2::RectD & m_bounds;
    CountryBoundsCalculator(m2::RectD & bounds) : m_bounds(bounds) {}
    void operator()(CountryFile const & file)
    {
      feature::DataHeader header;
      FilesContainerR reader(GetPlatform().WritablePathForFile(file.first));
      header.Load(reader.GetReader(HEADER_FILE_TAG));
      m_bounds.Add(header.GetBounds());
    }
  };

  m2::RectD Country::Bounds() const
  {
    m2::RectD bounds;
    std::for_each(m_files.begin(), m_files.end(), CountryBoundsCalculator(bounds));
    return bounds;
  }

  struct SizeCalculator
  {
    uint64_t & m_localSize;
    uint64_t & m_remoteSize;
    SizeCalculator(uint64_t & localSize, uint64_t & remoteSize)
      : m_localSize(localSize), m_remoteSize(remoteSize) {}
    void operator()(CountryFile const & file)
    {
      if (IsFileDownloaded(file))
        m_localSize += file.second;
      m_remoteSize += file.second;
    }
  };

  LocalAndRemoteSizeT Country::Size() const
  {
    uint64_t localSize = 0;
    uint64_t remoteSize = 0;
    std::for_each(m_files.begin(), m_files.end(), SizeCalculator(localSize, remoteSize));
    return LocalAndRemoteSizeT(localSize, remoteSize);
  }

  void Country::AddFile(CountryFile const & file)
  {
    m_files.push_back(file);
  }

  ////////////////////////////////////////////////////////////////////////

  bool LoadCountries(file_t const & file, FilesContainerT const & sortedFiles,
                     TCountriesContainer & countries)
  {
    countries.Clear();

    string buffer;
    file.ReadAsString(buffer);
    istringstream stream(buffer);

    std::string line;
    Country * currentCountry = &countries.Value();
    while (stream.good())
    {
      std::getline(stream, line);
      if (line.empty())
        continue;

      // calculate spaces - depth inside the tree
      int spaces = 0;
      for (size_t i = 0; i < line.size(); ++i)
      {
        if (line[i] == ' ')
          ++spaces;
        else
          break;
      }
      switch (spaces)
      {
      case 0:
        CHECK(false, ("We should never be here"));
        break;
      case 1: // country group
      case 2: // country name
      case 3: // region
        {
          line = line.substr(spaces);
          // country can have a defined flag in the beginning, like by:Belarus
          size_t const flagIndex = line.find(':');
          string flag;
          if (flagIndex != string::npos)
          {
            flag = line.substr(0, flagIndex);
            line = line.substr(flagIndex + 1);
          }
          strings::SimpleTokenizer tokIt(line, "|");
          // first string is country name, not always equal to country file name
          currentCountry = &countries.AddAtDepth(spaces - 1, Country(*tokIt, flag));
          // skip if > 1 names in the list - first name never corresponds to tile file
          if (!tokIt.IsLast())
            ++tokIt;
          while (tokIt)
          {
            FilesContainerT::const_iterator const first = sortedFiles.begin();
            FilesContainerT::const_iterator const last = sortedFiles.end();
            string const nameWithExt = *tokIt + DATA_FILE_EXTENSION;
            FilesContainerT::const_iterator const found = lower_bound(
                first, last, CountryFile(nameWithExt, 0));
            if (found != last && !(nameWithExt < found->first))
              currentCountry->AddFile(*found);
            ++tokIt;
          }
        }
        break;
      default:
        return false;
      }
    }
    return countries.SiblingsCount() > 0;
  }

  void SaveFiles(string const & file, CommonFilesT const & commonFiles)
  {
    FileWriter writer(file);
    stream::SinkWriterStream<Writer> wStream(writer);

    // save version - it's equal to current date in GMT
    wStream << my::TodayAsYYMMDD();
    wStream << commonFiles;
  }

  bool LoadFiles(file_t const & file, FilesContainerT & files, uint32_t & dataVersion)
  {
    files.clear();

    try
    {
      ReaderSource<file_t> source(file);
      stream::SinkReaderStream<ReaderSource<file_t> > stream(source);

      CommonFilesT commonFiles;

      stream >> dataVersion;
      stream >> commonFiles;

      files.reserve(commonFiles.size());

      for (CommonFilesT::iterator it = commonFiles.begin(); it != commonFiles.end(); ++it)
        files.push_back(CountryFile(it->first, it->second));

      sort(files.begin(), files.end());
    }
    catch (RootException const & e)
    {
      LOG(LWARNING, ("Can't read tiles file", e.what()));
      return false;
    }

    return true;
  }
}
