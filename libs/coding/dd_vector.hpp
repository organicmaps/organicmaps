#pragma once

#include "coding/reader.hpp"

#include "base/assert.hpp"
#include "base/exception.hpp"

#include <type_traits>

#include <boost/iterator/iterator_facade.hpp>

// Disk-driven vector.
template <typename T, class TReader, typename TSize = uint32_t>
class DDVector
{
public:
  typedef T value_type;
  typedef TSize size_type;
  typedef std::make_signed_t<size_type> difference_type;
  typedef TReader ReaderType;

  DECLARE_EXCEPTION(OpenException, RootException);

  DDVector() : m_Size(0) {}

  explicit DDVector(TReader const & reader) : m_reader(reader)
  {
    InitSize();
  }

  void Init(TReader const & reader)
  {
    m_reader = reader;
    InitSize();
  }

  size_type size() const
  {
    return m_Size;
  }

  T const operator [] (size_type i) const
  {
    return ReadPrimitiveFromPos<T>(m_reader, static_cast<uint64_t>(i) * sizeof(T));
  }

  class const_iterator : public boost::iterator_facade<
      const_iterator,
      value_type const,
      boost::random_access_traversal_tag,
      value_type const &,
      difference_type>
  {
  public:
#ifdef DEBUG
    const_iterator(ReaderType const * pReader, size_type i, size_type size)
      : m_pReader(pReader), m_I(i), m_bValueRead(false), m_Size(size)
    {
      ASSERT(static_cast<difference_type>(m_Size) >= 0, ());
    }
#else
    const_iterator(ReaderType const * pReader, size_type i)
      : m_pReader(pReader), m_I(i), m_bValueRead(false)
    {
    }
#endif

    T const & dereference() const
    {
      ASSERT_LESS(m_I, m_Size, (m_bValueRead));
      if (!m_bValueRead)
      {
        m_Value = ReadPrimitiveFromPos<T>(*m_pReader, static_cast<uint64_t>(m_I) * sizeof(T));
        m_bValueRead = true;
      }
      return m_Value;
    }

    void advance(difference_type n)
    {
      ASSERT_LESS_OR_EQUAL(m_I, m_Size, (m_bValueRead));
      m_I += n;
      ASSERT_LESS_OR_EQUAL(m_I, m_Size, (m_bValueRead));
      m_bValueRead = false;
    }

    difference_type distance_to(const_iterator const & it) const
    {
      ASSERT_LESS_OR_EQUAL(m_I, m_Size, (m_bValueRead));
      ASSERT_LESS_OR_EQUAL(it.m_I, it.m_Size, (it.m_bValueRead));
      ASSERT_EQUAL(m_Size, it.m_Size, (m_I, it.m_I, m_bValueRead, it.m_bValueRead));
      ASSERT(m_pReader == it.m_pReader, (m_I, m_Size, it.m_I, it.m_Size));
      return (static_cast<difference_type>(it.m_I) -
              static_cast<difference_type>(m_I));
    }

    void increment()
    {
      ++m_I;
      m_bValueRead = false;
      ASSERT_LESS_OR_EQUAL(m_I, m_Size, (m_bValueRead));
    }

    void decrement()
    {
      --m_I;
      m_bValueRead = false;
      ASSERT_LESS_OR_EQUAL(m_I, m_Size, (m_bValueRead));
    }

    bool equal(const_iterator const & it) const
    {
      ASSERT_LESS_OR_EQUAL(m_I, m_Size, (m_bValueRead));
      ASSERT_LESS_OR_EQUAL(it.m_I, it.m_Size, (it.m_bValueRead));
      ASSERT_EQUAL(m_Size, it.m_Size, (m_I, it.m_I, m_bValueRead, it.m_bValueRead));
      ASSERT(m_pReader == it.m_pReader, (m_I, m_Size, it.m_I, it.m_Size));
      return m_I == it.m_I;
    }

  private:
    ReaderType const * m_pReader;
    size_type m_I;
    mutable T m_Value = {};
    mutable bool m_bValueRead;
#ifdef DEBUG
    size_type m_Size;
#endif
  };

  const_iterator begin() const
  {
#ifdef DEBUG
    return const_iterator(&m_reader, 0, m_Size);
#else
    return const_iterator(&m_reader, 0);
#endif
  }

  const_iterator end() const
  {
#ifdef DEBUG
    return const_iterator(&m_reader, m_Size, m_Size);
#else
    return const_iterator(&m_reader, m_Size);
#endif
  }

  void Read(size_type i, T & result) const
  {
    ASSERT_LESS(i, m_Size, ());
    ReadFromPos(m_reader, i * sizeof(T), &result, sizeof(T));
  }

  void Read(size_type i, T * result, size_t count)
  {
    ASSERT_LESS(i + count, m_Size, (i, count));
    ReadFromPos(m_reader, i * sizeof(T), result, count * sizeof(T));
  }

private:
  void InitSize()
  {
    uint64_t const sz = m_reader.Size();
    if ((sz % sizeof(T)) != 0)
      MYTHROW(OpenException, ("Element size", sizeof(T), "does not divide total size", sz));

    m_Size = static_cast<size_type>(sz / sizeof(T));
  }

  // TODO: Refactor me to use Reader by pointer.
  ReaderType m_reader;
  size_type m_Size;
};
