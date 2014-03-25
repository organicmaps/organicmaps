#pragma once
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/exception.hpp"
#include "../std/algorithm.hpp"
#include "../std/shared_ptr.hpp"
#include "../std/cstring.hpp"
#include "../std/string.hpp"
#include "../std/vector.hpp"

// Generic Writer. Not thread-safe.
// When SubWriter is used, pos can negative, so int64_t is used to store pos.
class Writer
{
public:
  DECLARE_EXCEPTION(Exception, RootException);
  DECLARE_EXCEPTION(OpenException, Exception);
  DECLARE_EXCEPTION(WriteException, Exception);
  DECLARE_EXCEPTION(PosException, Exception);
  DECLARE_EXCEPTION(SeekException, Exception);

  virtual ~Writer() {}
  virtual void Seek(int64_t pos) = 0;
  virtual int64_t Pos() const = 0;
  virtual void Write(void const * p, size_t size) = 0;
};

template <typename ContainerT>
class MemWriter : public Writer
{
public:
  inline MemWriter(ContainerT & data) : m_Data(data), m_Pos(0)
  {
    STATIC_ASSERT(sizeof(typename ContainerT::value_type) == 1);
  }

  inline void Seek(int64_t pos)
  {
    ASSERT_EQUAL(pos, static_cast<intptr_t>(pos), ());
    ASSERT_GREATER_OR_EQUAL(pos, 0, ());
    m_Pos = static_cast<intptr_t>(pos);
  }

  inline int64_t Pos() const
  {
    return m_Pos;
  }

  inline void Write(void const * p, size_t size)
  {
    if (m_Pos + size > m_Data.size())
      m_Data.resize(m_Pos + size);
    memcpy(&m_Data[m_Pos], p, size);
    m_Pos += size;
  }

private:
  ContainerT & m_Data;
  size_t m_Pos;
};

// Original writer should not be used when SubWriter is active!
// In destructor, SubWriter calls Seek() of original writer to the end of what has been written.
template <typename WriterT>
class SubWriter
{
public:
  inline explicit SubWriter(WriterT & writer)
    : m_Writer(writer), m_Pos(0), m_MaxPos(0)
#ifdef DEBUG
    , m_Offset(m_Writer.Pos() - m_Pos)
#endif
  {
  }

  ~SubWriter()
  {
    ASSERT_EQUAL(m_Offset, m_Writer.Pos() - m_Pos, ());
    if (m_Pos != m_MaxPos)
      Seek(m_MaxPos);
  }

  inline void Seek(int64_t pos)
  {
    ASSERT_EQUAL(m_Offset, m_Writer.Pos() - m_Pos, ());
    m_MaxPos = max(m_MaxPos, pos);
    m_Writer.Seek(m_Writer.Pos() - m_Pos + pos);
    m_Pos = pos;
  }

  inline int64_t Pos() const
  {
    ASSERT_EQUAL(m_Offset, m_Writer.Pos() - m_Pos, ());
    return m_Pos;
  }

  inline void Write(void const * p, size_t size)
  {
    ASSERT_EQUAL(m_Offset, m_Writer.Pos() - m_Pos, ());
    m_MaxPos = max(m_MaxPos, static_cast<int64_t>(m_Pos + size));
    m_Writer.Write(p, size);
    m_Pos += size;
  }

  inline uint64_t Size() const
  {
    return m_MaxPos;
  }

private:
  WriterT & m_Writer;
  int64_t m_Pos;
  int64_t m_MaxPos;
#ifdef DEBUG
  int64_t const m_Offset;
#endif
};

template<typename WriterT>
class WriterPtr
{
public:
  WriterPtr(WriterT * p = 0) : m_p(p) {}

  void Seek(int64_t pos)
  {
    m_p->Seek(pos);
  }

  int64_t Pos() const
  {
    return m_p->Pos();
  }

  void Write(void const * p, size_t size)
  {
    m_p->Write(p, size);
  }

  WriterT * GetPtr() const { return m_p.get(); }

protected:
  shared_ptr<WriterT> m_p;
};

template <typename WriterT>
class WriterSink
{
public:
  inline WriterSink(WriterT & writer) : m_writer(writer), m_pos(0) {}

  inline void Write(void const * p, size_t size)
  {
    m_writer.Write(p, size);
    m_pos += size;
  }

private:
  WriterT & m_writer;
  int64_t m_pos;
};
