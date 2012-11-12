// Boost.Bimap
//
// Copyright (c) 2006-2007 Matias Capeletto
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

/// \file detail/map_view_iterator.hpp
/// \brief Iterator adaptors from multi-index to bimap.

#ifndef BOOST_BIMAP_DETAIL_MAP_VIEW_ITERATOR_HPP
#define BOOST_BIMAP_DETAIL_MAP_VIEW_ITERATOR_HPP

#if defined(_MSC_VER) && (_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp>

// Boost

#ifndef BOOST_BIMAP_DISABLE_SERIALIZATION
  #include <boost/serialization/nvp.hpp>
#endif // BOOST_BIMAP_DISABLE_SERIALIZATION

#include <boost/iterator/detail/enable_if.hpp>
#include <boost/iterator/iterator_adaptor.hpp>
#include <boost/bimap/relation/support/pair_by.hpp>

namespace boost {
namespace bimaps {
namespace detail {

#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Tag, class Relation, class CoreIterator > struct map_view_iterator;

template< class Tag, class Relation, class CoreIterator >
struct map_view_iterator_base
{
    typedef iterator_adaptor
    <
        map_view_iterator< Tag, Relation, CoreIterator >,
        CoreIterator,
        BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
            pair_type_by<Tag,Relation>::type

    > type;
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

/** \brief Map View Iterator adaptor from multi index to bimap.

This is class is based on transform iterator from Boost.Iterator that is
modified to allow serialization. It has been specialized for this
library, and EBO optimization was applied to the functor.

                                                                      **/

template< class Tag, class Relation, class CoreIterator >
struct map_view_iterator : public map_view_iterator_base<Tag,Relation,CoreIterator>::type
{
    typedef BOOST_DEDUCED_TYPENAME
        map_view_iterator_base<Tag,Relation,CoreIterator>::type base_;

    public:

    map_view_iterator() {}

    map_view_iterator(CoreIterator const& iter)
      : base_(iter) {}

    map_view_iterator(map_view_iterator const & iter)
      : base_(iter.base()) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(
            *const_cast<BOOST_DEDUCED_TYPENAME base_::base_type::value_type*>(
                &(*this->base())
            )
        );
    }

    private:

    friend class iterator_core_access;

    #ifndef BOOST_BIMAP_DISABLE_SERIALIZATION

    // Serialization support

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    friend class ::boost::serialization::access;

    template< class Archive >
    void save(Archive & ar, const unsigned int) const
    {
        ar << ::boost::serialization::make_nvp("mi_iterator",this->base());
    }

    template< class Archive >
    void load(Archive & ar, const unsigned int)
    {
        CoreIterator iter;
        ar >> ::boost::serialization::make_nvp("mi_iterator",iter);
        this->base_reference() = iter;
    }

    #endif // BOOST_BIMAP_DISABLE_SERIALIZATION
};


#ifndef BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES

template< class Tag, class Relation, class CoreIterator > struct const_map_view_iterator;

template< class Tag, class Relation, class CoreIterator >
struct const_map_view_iterator_base
{
    typedef iterator_adaptor
    <
        const_map_view_iterator< Tag, Relation, CoreIterator >,
        CoreIterator,
        const BOOST_DEDUCED_TYPENAME ::boost::bimaps::relation::support::
             pair_type_by<Tag,Relation>::type

    > type;
};

#endif // BOOST_BIMAP_DOXYGEN_WILL_NOT_PROCESS_THE_FOLLOWING_LINES


/** \brief Const Map View Iterator adaptor from multi index to bimap.

See also map_view_iterator.
                                                                      **/

template< class Tag, class Relation, class CoreIterator >
struct const_map_view_iterator :

    public const_map_view_iterator_base<Tag,Relation,CoreIterator>::type
{
    typedef BOOST_DEDUCED_TYPENAME
        const_map_view_iterator_base<Tag,Relation,CoreIterator>::type base_;

    public:

    const_map_view_iterator() {}

    const_map_view_iterator(CoreIterator const& iter)
      : base_(iter) {}

    const_map_view_iterator(const_map_view_iterator const & iter)
      : base_(iter.base()) {}

    const_map_view_iterator(map_view_iterator<Tag,Relation,CoreIterator> i)
      : base_(i.base()) {}

    BOOST_DEDUCED_TYPENAME base_::reference dereference() const
    {
        return ::boost::bimaps::relation::support::pair_by<Tag>(*this->base());
    }

    private:

    friend class iterator_core_access;

    #ifndef BOOST_BIMAP_DISABLE_SERIALIZATION

    // Serialization support

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    friend class ::boost::serialization::access;

    template< class Archive >
    void save(Archive & ar, const unsigned int) const
    {
        ar << ::boost::serialization::make_nvp("mi_iterator",this->base());
    }

    template< class Archive >
    void load(Archive & ar, const unsigned int)
    {
        CoreIterator iter;
        ar >> ::boost::serialization::make_nvp("mi_iterator",iter);
        this->base_reference() = iter;
    }

    #endif // BOOST_BIMAP_DISABLE_SERIALIZATION
};


} // namespace detail
} // namespace bimaps
} // namespace boost

#endif // BOOST_BIMAP_DETAIL_MAP_VIEW_ITERATOR_HPP


