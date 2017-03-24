#pragma once
#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/checked_cast.hpp"
#include "base/exception.hpp"
#include "std/algorithm.hpp"
#include "std/shared_ptr.hpp"
#include "std/cstring.hpp"
#include "std/string.hpp"
#include "std/vector.hpp"

// Generic Writer. Not thread-safe.
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
  virtual void Seek(uint64_t pos) = 0;
  virtual uint64_t Pos() const = 0;
  virtual void Write(void const * p, size_t size) = 0;
};

template <typename ContainerT>
class MemWriter : public Writer
{
public:
  inline explicit MemWriter(ContainerT & data) : m_Data(data), m_Pos(0)
  {
    static_assert(sizeof(typename ContainerT::value_type) == 1, "");
  }

  inline void Seek(uint64_t pos) override
  {
    m_Pos = base::asserted_cast<uintptr_t>(pos);
  }

  inline uint64_t Pos() const override
  {
    return m_Pos;
  }

  inline void Write(void const * p, size_t size) override
  {
    intptr_t freeSize = m_Data.size() - m_Pos;
    if (freeSize < 0)
    {
      m_Data.resize(m_Pos + size);
      freeSize = size;
    }

    memcpy(&m_Data[m_Pos], p, min(size, static_cast<size_t>(freeSize)));

    if (size > static_cast<size_t>(freeSize))
    {
      uint8_t const * it = reinterpret_cast<uint8_t const *>(p);
      m_Data.insert(m_Data.end(), it + freeSize, it + size);
    }

    m_Pos += size;
  }

private:
  ContainerT & m_Data;
  uint64_t m_Pos;
};

// Original writer should not be used when SubWriter is active!
// In destructor, SubWriter calls Seek() of original writer to the end of what has been written.
template <typename WriterT>
class SubWriter : public Writer
{
public:
  inline explicit SubWriter(WriterT & writer)
    : m_writer(writer), m_pos(0), m_maxPos(0)
#ifdef DEBUG
    , m_offset(GetOffset())
#endif
  {
  }

  ~SubWriter() override
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    if (m_pos != m_maxPos)
      Seek(m_maxPos);
  }

  inline void Seek(uint64_t pos) override
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    m_writer.Seek(GetOffset() + pos);

    m_pos = pos;
    m_maxPos = max(m_maxPos, m_pos);
  }

  inline uint64_t Pos() const override
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    return m_pos;
  }

  inline void Write(void const * p, size_t size) override
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
  uint64_t m_pos;
  uint64_t m_maxPos;
#ifdef DEBUG
  uint64_t const m_offset;
#endif
};

template<typename WriterT>
class WriterPtr : public Writer
{
public:
  explicit WriterPtr(WriterT * p = 0) : m_p(p) {}

  void Seek(uint64_t pos) override
  {
    m_p->Seek(pos);
  }

  uint64_t Pos() const override
  {
    return m_p->Pos();
  }

  void Write(void const * p, size_t size) override
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
  inline explicit WriterSink(WriterT & writer) : m_writer(writer), m_pos(0) {}

  inline void Write(void const * p, size_t size)
  {
    m_writer.Write(p, size);
    m_pos += size;
  }

private:
  WriterT & m_writer;
  uint64_t m_pos;
};
