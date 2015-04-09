#pragma once
#include "coding/file_reader.hpp"
#include "coding/file_writer.hpp"
#include "base/base.hpp"
#include "base/logging.hpp"
#include "base/exception.hpp"
#include "std/algorithm.hpp"
#include "std/cstdlib.hpp"
#include "std/functional.hpp"
#include "std/queue.hpp"
#include "std/unique_ptr.hpp"
#include "std/string.hpp"
#include "std/utility.hpp"
#include "std/vector.hpp"

template <typename LessT>
struct Sorter
{
  LessT m_Less;
  Sorter(LessT lessF) : m_Less(lessF) {}
  template <typename IterT> void operator() (IterT beg, IterT end) const
  {
    sort(beg, end, m_Less);
  }
};

template <
    typename T,                                       // Item type.
    class OutputSinkT = FileWriter,                   // Sink to output into result file.
    typename LessT = less<T>,                         // Item comparator.
    template <typename LessT1> class SorterT = Sorter // Item sorter.
>
class FileSorter
{
public:
  FileSorter(size_t bufferBytes,
             string const & tmpFileName,
             OutputSinkT & outputSink,
             LessT fLess = LessT()) :
  m_TmpFileName(tmpFileName),
  m_BufferCapacity(max(size_t(16), bufferBytes / sizeof(T))),
  m_OutputSink(outputSink),
  m_ItemCount(0),
  m_Less(fLess)
  {
    m_Buffer.reserve(m_BufferCapacity);
    m_pTmpWriter.reset(new FileWriter(tmpFileName));
  }

  void Add(T const & item)
  {
    if (m_Buffer.size() == m_BufferCapacity)
      FlushToTmpFile();
    m_Buffer.push_back(item);
    ++m_ItemCount;
  }

  void SortAndFinish()
  {
    ASSERT(m_pTmpWriter.get(), ());
    FlushToTmpFile();

    // Write output.
    {
      m_pTmpWriter.reset();
      FileReader reader(m_TmpFileName);
      ItemIndexPairGreater fGreater(m_Less);
      PriorityQueueType q(fGreater);
      for (uint32_t i = 0; i < m_ItemCount; i += m_BufferCapacity)
        Push(q, i, reader);

      while (!q.empty())
      {
        m_OutputSink(q.top().first);
        uint32_t const i = q.top().second + 1;
        q.pop();
        if (i % m_BufferCapacity != 0 && i < m_ItemCount)
          Push(q, i, reader);
      }
    }
    FileWriter::DeleteFileX(m_TmpFileName);
  }

  ~FileSorter()
  {
    if (m_pTmpWriter.get())
    {
      try
      {
        SortAndFinish();
      }
      catch(RootException const & e)
      {
        LOG(LERROR, (e.Msg()));
      }
      catch(std::exception const & e)
      {
        LOG(LERROR, (e.what()));
      }
    }
  }

private:
  struct ItemIndexPairGreater
  {
    explicit ItemIndexPairGreater(LessT fLess) : m_Less(fLess) {}
    inline bool operator() (pair<T, uint32_t> const & a, pair<T, uint32_t> const & b) const
    {
      return m_Less(b.first, a.first);
    }
    LessT m_Less;
  };

  typedef priority_queue<pair<T, uint32_t>, vector<pair<T, uint32_t> >, ItemIndexPairGreater>
      PriorityQueueType;

  void FlushToTmpFile()
  {
    if (m_Buffer.empty())
      return;
    SorterT<LessT> sorter(m_Less);
    sorter(m_Buffer.begin(), m_Buffer.end());
    m_pTmpWriter->Write(&m_Buffer[0], m_Buffer.size() * sizeof(T));
    m_Buffer.clear();
  }

  void Push(PriorityQueueType & q, uint32_t i, FileReader const & reader)
  {
    T item;
    reader.Read(static_cast<uint64_t>(i) * sizeof(T), &item, sizeof(T));
    q.push(pair<T, uint32_t>(item, i));
  }

  string const m_TmpFileName;
  size_t const m_BufferCapacity;
  OutputSinkT & m_OutputSink;
  unique_ptr<FileWriter> m_pTmpWriter;
  vector<T> m_Buffer;
  uint32_t m_ItemCount;
  LessT m_Less;
};
