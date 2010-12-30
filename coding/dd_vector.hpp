#pragma once
#include "reader.hpp"
#include "varint.hpp"
#include "../base/assert.hpp"
#include "../base/base.hpp"
#include "../base/src_point.hpp"
#include "../std/type_traits.hpp"
#include <boost/iterator/iterator_facade.hpp>

#include "../base/start_mem_debug.hpp"

template <
    typename T,
    class TReader,
    typename TSize = uint32_t,
    typename TDifference = typename make_signed<TSize>::type
    > class DDVector
{
public:
  typedef T value_type;
  typedef TSize size_type;
  typedef TDifference difference_type;
  typedef TReader ReaderType;

  DDVector() : m_Size(0) {}

  DDVector(TReader const & reader, size_type size) : m_reader(reader), m_Size(size)
  {
  }

  size_type size() const
  {
    return m_Size;
  }

  T const operator [] (size_type i) const
  {
    ASSERT_LESS(i, m_Size, ());
    T result;
    this->Read(i, result);
    return result;
  }

  class const_iterator : public boost::iterator_facade<
      const_iterator,
      value_type const,
      boost::random_access_traversal_tag,
      value_type const &,
      difference_type>
  {
  public:
    const_iterator() : m_pReader(NULL), m_I(0), m_bValueRead(false)
    {
    }

#ifdef DEBUG
    const_iterator(ReaderType const * pReader, size_type i, size_type size)
      : m_pReader(pReader), m_I(i), m_bValueRead(false), m_Size(size)
    {
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
        ReadFromPos(*m_pReader, m_I * sizeof(T), &m_Value, sizeof(T));
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
      return it.m_I - m_I;
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
    mutable T m_Value;
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
  ReaderType m_reader;
  size_type m_Size;
};

#include "../base/stop_mem_debug.hpp"
