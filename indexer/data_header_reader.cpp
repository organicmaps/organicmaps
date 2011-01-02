#include "../base/SRC_FIRST.hpp"

#include "data_header_reader.hpp"
#include "data_header.hpp"

#include "../storage/defines.hpp"

#include "../coding/file_container.hpp"

#include "../base/start_mem_debug.hpp"

namespace feature
{
  uint64_t GetSkipHeaderSize(Reader const & reader)
  {
    uint64_t const headerSize = ReadPrimitiveFromPos<uint64_t>(reader, 0);
    return headerSize + sizeof(uint64_t);
  }

  uint64_t ReadDataHeader(string const & datFileName, feature::DataHeader & outHeader)
  {
    try
    {
      FileReader reader = FilesContainerR(datFileName).GetReader(DATA_FILE_TAG);

      uint64_t const toSkip = GetSkipHeaderSize(reader);

      ReaderSource<FileReader> src(reader);
      src.Skip(sizeof(uint64_t));

      outHeader.Load(src);

      return toSkip;
    }
    catch (Reader::Exception const & e)
    {
      ASSERT(false, ("Error reading header from dat file", e.what()));
      return 0;
    }
  }

  void WriteDataHeader(Writer & writer, feature::DataHeader const & header)
  {
    typedef vector<unsigned char> TBuffer;
    TBuffer buffer;
    MemWriter<TBuffer> w(buffer);

    header.Save(w);

    uint64_t const sz = buffer.size();
    WriteToSink(writer, sz);

    if (sz > 0)
      writer.Write(&buffer[0], buffer.size());
  }
}
