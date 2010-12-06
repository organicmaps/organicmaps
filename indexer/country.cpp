#include "country.hpp"

#include "../base/logging.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../platform/platform.hpp"

#include "../indexer/data_header.hpp"
#include "../indexer/data_header_reader.hpp"

namespace mapinfo
{
  string FileNameFromUrl(string const & url)
  {
    size_t lastSlashPos = url.find_last_of('/');
    if (lastSlashPos != string::npos && (lastSlashPos + 1) < url.size())
      return url.substr(lastSlashPos + 1);
    ASSERT( false, ("Url should be valid") );
    return string();
  }

  bool IsFileSizeEqualTo(string const & fileName, uint64_t size)
  {
    uint64_t diskSize = 0;
    if (GetPlatform().GetFileSize(GetPlatform().WritablePathForFile(fileName), diskSize)
        && diskSize == size)
      return true;
    else
      return false;
  }

  /// Simple check - compare url size with real file size on disk
  bool IsFileDownloaded(TUrl const & url)
  {
    string fileName = FileNameFromUrl(url.first);
    return IsFileSizeEqualTo(fileName, url.second);
  }

  struct CountryBoundsCalculator
  {
    m2::RectD & m_bounds;
    CountryBoundsCalculator(m2::RectD & bounds) : m_bounds(bounds) {}
    void operator()(TUrl const & url)
    {
      string fileName = FileNameFromUrl(url.first);
      if (IsFileSizeEqualTo(fileName, url.second) && IsDatFile(fileName))
      {
        feature::DataHeader header;
        if (feature::ReadDataHeader(GetPlatform().WritablePathForFile(fileName), header))
          m_bounds.Add(header.Bounds());
      }
    }
  };

  m2::RectD Country::Bounds() const
  {
    m2::RectD bounds;
    std::for_each(m_urls.begin(), m_urls.end(), CountryBoundsCalculator(bounds));
    return bounds;
  }

  struct LocalSizeCalculator
  {
    uint64_t & m_size;
    LocalSizeCalculator(uint64_t & size) : m_size(size) {}
    void operator()(TUrl const & url)
    {
      if (IsFileDownloaded(url))
        m_size += url.second;
    }
  };

  uint64_t Country::LocalSize() const
  {
    uint64_t size = 0;
    std::for_each(m_urls.begin(), m_urls.end(), LocalSizeCalculator(size));
    return size;
  }

  struct RemoteSizeCalculator
  {
    uint64_t & m_size;
    RemoteSizeCalculator(uint64_t & size) : m_size(size) {}
    void operator()(TUrl const & url)
    {
      if (!IsFileDownloaded(url))
        m_size += url.second;
    }
  };

  uint64_t Country::RemoteSize() const
  {
    uint64_t size = 0;
    std::for_each(m_urls.begin(), m_urls.end(), RemoteSizeCalculator(size));
    return size;
  }

  void Country::AddUrl(TUrl const & url)
  {
    m_urls.push_back(url);
  }

  ////////////////////////////////////////////////////////////////////////

  template <class TArchive> TArchive & operator << (TArchive & ar, mapinfo::Country const & country)
  {
    ar << country.m_group;
    ar << country.m_country;
    ar << country.m_region;
    ar << country.m_urls;
    return ar;
  }

  bool LoadCountries(TCountriesContainer & countries, string const & updateFile)
  {
    countries.clear();
    try
    {
      FileReader file(updateFile.c_str());
      ReaderSource<FileReader> source(file);
      stream::SinkReaderStream<ReaderSource<FileReader> > rStream(source);
      uint32_t version;
      rStream >> version;
      if (version > MAPS_MAJOR_VERSION_BINARY_FORMAT)
        return false;
      rStream >> countries;
      return true;
    }
    catch (RootException const & e)
    {
      LOG(LERROR, ("LoadCountries exception", e.what()));
    }
    return false;
  }

  void SaveCountries(TCountriesContainer const & countries, Writer & writer)
  {
    stream::SinkWriterStream<Writer> wStream(writer);
    wStream << MAPS_MAJOR_VERSION_BINARY_FORMAT;
    wStream << countries;
  }
}
