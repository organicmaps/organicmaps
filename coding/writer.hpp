#pragma once

#include "base/assert.hpp"
#include "base/base.hpp"
#include "base/checked_cast.hpp"
#include "base/exception.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

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

  virtual void Seek(uint64_t pos) = 0;
  virtual uint64_t Pos() const = 0;
  virtual void Write(void const * p, size_t size) = 0;

  // Disable deletion via this interface, because some dtors in derived classes are noexcept(false).
protected:
  ~Writer() = default;
};

// 'MemWriter<std::vector<char>>' has virtual functions but non-virtual destructor
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-virtual-dtor"
#endif  // __clang__
template <typename ContainerT> class MemWriter : public Writer
#ifdef __clang__
#pragma clang diagnostic pop
#endif  // __clang__
{
public:
  explicit MemWriter(ContainerT & data) : m_Data(data), m_Pos(0)
  {
    static_assert(sizeof(typename ContainerT::value_type) == 1, "");
  }

  void Seek(uint64_t pos) override { m_Pos = base::asserted_cast<uintptr_t>(pos); }

  uint64_t Pos() const override { return m_Pos; }

  void Write(void const * p, size_t size) override
  {
    auto freeSize = static_cast<intptr_t>(m_Data.size() - m_Pos);
    if (freeSize < 0)
    {
      m_Data.resize(static_cast<size_t>(m_Pos + size));
      freeSize = size;
    }

    memcpy(&m_Data[static_cast<size_t>(m_Pos)], p, std::min(size, static_cast<size_t>(freeSize)));

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
  explicit SubWriter(WriterT & writer)
    : m_writer(writer)
    , m_pos(0)
    , m_maxPos(0)
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

  void Seek(uint64_t pos) override
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    m_writer.Seek(GetOffset() + pos);

    m_pos = pos;
    m_maxPos = std::max(m_maxPos, m_pos);
  }

  uint64_t Pos() const override
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    return m_pos;
  }

  void Write(void const * p, size_t size) override
  {
    ASSERT_EQUAL(m_offset, GetOffset(), ());
    m_writer.Write(p, size);

    m_pos += size;
    m_maxPos = std::max(m_maxPos, m_pos);
  }

  uint64_t Size() const { return m_maxPos; }

private:
  uint64_t GetOffset() const { return m_writer.Pos() - m_pos; }

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
  std::shared_ptr<WriterT> m_p;
};

template <typename WriterT>
class WriterSink
{
public:
  explicit WriterSink(WriterT & writer) : m_writer(writer), m_pos(0) {}

  void Write(void const * p, size_t size)
  {
    m_writer.Write(p, size);
    m_pos += size;
  }

private:
  WriterT & m_writer;
  uint64_t m_pos;
};
