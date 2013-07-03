/*
 *          Copyright Andrey Semashev 2007 - 2013.
 * Distributed under the Boost Software License, Version 1.0.
 *    (See accompanying file LICENSE_1_0.txt or copy at
 *          http://www.boost.org/LICENSE_1_0.txt)
 */
/*!
 * \file   intrusive_ref_counter.hpp
 * \author Andrey Semashev
 * \date   12.03.2009
 *
 * This header contains a reference counter class for \c intrusive_ptr.
 */

#ifndef BOOST_LOG_UTILITY_INTRUSIVE_REF_COUNTER_HPP_INCLUDED_
#define BOOST_LOG_UTILITY_INTRUSIVE_REF_COUNTER_HPP_INCLUDED_

#include <boost/intrusive_ptr.hpp>
#include <boost/log/detail/config.hpp>
#ifndef BOOST_LOG_NO_THREADS
#include <boost/detail/atomic_count.hpp>
#endif // BOOST_LOG_NO_THREADS
#include <boost/log/detail/header.hpp>

#ifdef BOOST_LOG_HAS_PRAGMA_ONCE
#pragma once
#endif

namespace boost {

BOOST_LOG_OPEN_NAMESPACE

class intrusive_ref_counter;

#ifndef BOOST_LOG_DOXYGEN_PASS
void intrusive_ptr_add_ref(const intrusive_ref_counter* p);
void intrusive_ptr_release(const intrusive_ref_counter* p);
#endif // BOOST_LOG_DOXYGEN_PASS

/*!
 * \brief A reference counter base class
 *
 * This base class can be used with user-defined classes to add support
 * for \c intrusive_ptr. The class contains a thread-safe reference counter
 * and a virtual destructor, which makes the derived class polymorphic.
 * Upon releasing the last \c intrusive_ptr referencing the object
 * derived from the \c intrusive_ref_counter class, operator \c delete
 * is automatically called on the pointer to the object.
 */
class intrusive_ref_counter
{
private:
    //! Reference counter
#ifndef BOOST_LOG_NO_THREADS
    mutable boost::detail::atomic_count m_RefCounter;
#else
    mutable unsigned long m_RefCounter;
#endif // BOOST_LOG_NO_THREADS

public:
    /*!
     * Default constructor
     *
     * \post <tt>use_count() == 0</tt>
     */
    intrusive_ref_counter() : m_RefCounter(0)
    {
    }
    /*!
     * Copy constructor
     *
     * \post <tt>use_count() == 0</tt>
     */
    intrusive_ref_counter(intrusive_ref_counter const&) : m_RefCounter(0)
    {
    }

    /*!
     * Virtual destructor
     */
    virtual ~intrusive_ref_counter() {}

    /*!
     * Assignment
     *
     * \post The reference counter is not modified after assignment
     */
    intrusive_ref_counter& operator= (intrusive_ref_counter const&) { return *this; }

    /*!
     * \return The reference counter
     */
    unsigned long use_count() const
    {
        return static_cast< unsigned long >(static_cast< long >(m_RefCounter));
    }

#ifndef BOOST_LOG_DOXYGEN_PASS
    friend void intrusive_ptr_add_ref(const intrusive_ref_counter* p);
    friend void intrusive_ptr_release(const intrusive_ref_counter* p);
#endif // BOOST_LOG_DOXYGEN_PASS
};

#ifndef BOOST_LOG_DOXYGEN_PASS
inline void intrusive_ptr_add_ref(const intrusive_ref_counter* p)
{
    ++p->m_RefCounter;
}
inline void intrusive_ptr_release(const intrusive_ref_counter* p)
{
    if (--p->m_RefCounter == 0)
        delete p;
}
#endif // BOOST_LOG_DOXYGEN_PASS

BOOST_LOG_CLOSE_NAMESPACE // namespace log

} // namespace boost

#include <boost/log/detail/footer.hpp>

#endif // BOOST_LOG_UTILITY_INTRUSIVE_REF_COUNTER_HPP_INCLUDED_
