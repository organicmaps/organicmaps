#pragma once
#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/exception.hpp"
#include "std/algorithm.hpp"
#include "std/shared_ptr.hpp"
#include "std/cstring.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

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
  DECLARE_EXCEPTION(CreateDirException, Exception);

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
    static_assert(sizeof(typename ContainerT::value_type) == 1, "");
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
    memcpy(((uint8_t*)m_Data.data()) + m_Pos, p, size);
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
    : m_writer(writer), m_pos(0), m_maxPos(0)
#ifdef DEBUG
    , m_offset(GetOffset())
#endif
  {
  }

  ~SubWriter()
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    if (m_pos != m_maxPos)
      Seek(m_maxPos);
  }

  inline void Seek(int64_t pos)
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    m_writer.Seek(GetOffset() + pos);

    m_pos = pos;
    m_maxPos = max(m_maxPos, m_pos);
  }

  inline int64_t Pos() const
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    return m_pos;
  }

  inline void Write(void const * p, size_t size)
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    m_writer.Write(p, size);

    m_pos += size;
    m_maxPos = max(m_maxPos, m_pos);
  }

  inline uint64_t Size() const { return m_maxPos; }

private:
  inline uint64_t GetOffset() const { return m_writer.Pos() - m_pos; }

private:
  WriterT & m_writer;
  int64_t m_pos;
  int64_t m_maxPos;
#ifdef DEBUG
  int64_t const m_offset;
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
