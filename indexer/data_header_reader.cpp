#include "data_header_reader.hpp"
#include "data_header.hpp"

#include "../coding/file_reader.hpp"
#include "../coding/file_writer.hpp"

#include "../base/start_mem_debug.hpp"

namespace feature
{
  uint64_t ReadDataHeader(string const & datFileName, feature::DataHeader & outHeader)
  {
    try
    {
      FileReader datReader(datFileName);
      // read header size
      uint64_t const headerSize = ReadPrimitiveFromPos<uint64_t>(datReader, 0);

      FileReader subReader = datReader.SubReader(sizeof(uint64_t), headerSize);
      ReaderSource<FileReader> src(subReader);
      outHeader.Load(src);

      return (headerSize + sizeof(uint64_t));
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
