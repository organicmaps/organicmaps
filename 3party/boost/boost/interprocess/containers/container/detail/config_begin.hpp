//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#ifndef BOOST_CONTAINERS_CONTAINER_DETAIL_CONFIG_INCLUDED
#define BOOST_CONTAINERS_CONTAINER_DETAIL_CONFIG_INCLUDED
#include <boost/config.hpp>

#define BOOST_CONTAINER_IN_INTERPROCESS
#define BOOST_MOVE_IN_INTERPROCESS

#ifdef BOOST_MOVE_IN_INTERPROCESS

#define INCLUDE_BOOST_CONTAINER_MOVE_HPP <boost/interprocess/detail/move.hpp>
#define BOOST_CONTAINER_MOVE_NAMESPACE boost::interprocess

#else

#define INCLUDE_BOOST_CONTAINER_MOVE_HPP <boost/move/move.hpp>
#define BOOST_CONTAINER_MOVE_NAMESPACE boost

#endif

#ifdef BOOST_CONTAINER_IN_INTERPROCESS

#define INCLUDE_BOOST_CONTAINER_CONTAINER_FWD_HPP                   <boost/interprocess/containers/container/container_fwd.hpp>
#define INCLUDE_BOOST_CONTAINER_DEQUE_HPP                           <boost/interprocess/containers/container/deque.hpp>
#define INCLUDE_BOOST_CONTAINER_FLAT_MAP_HPP                        <boost/interprocess/containers/container/flat_map.hpp>
#define INCLUDE_BOOST_CONTAINER_FLAT_SET_HPP                        <boost/interprocess/containers/container/flat_set.hpp>
#define INCLUDE_BOOST_CONTAINER_LIST_HPP                            <boost/interprocess/containers/container/list.hpp>
#define INCLUDE_BOOST_CONTAINER_MAP_HPP                             <boost/interprocess/containers/container/map.hpp>
#define INCLUDE_BOOST_CONTAINER_SET_HPP                             <boost/interprocess/containers/container/set.hpp>
#define INCLUDE_BOOST_CONTAINER_SLIST_HPP                           <boost/interprocess/containers/container/slist.hpp>
#define INCLUDE_BOOST_CONTAINER_STABLE_VECTOR_HPP                   <boost/interprocess/containers/container/stable_vector.hpp>
#define INCLUDE_BOOST_CONTAINER_STRING_HPP                          <boost/interprocess/containers/container/string.hpp>
#define INCLUDE_BOOST_CONTAINER_VECTOR_HPP                          <boost/interprocess/containers/container/vector.hpp>
//detail
#define INCLUDE_BOOST_CONTAINER_DETAIL_ADAPTIVE_NODE_POOL_IMPL_HPP  <boost/interprocess/containers/container/detail/adaptive_node_pool_impl.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ADVANCED_INSERT_INT_HPP      <boost/interprocess/containers/container/detail/advanced_insert_int.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ALGORITHMS_HPP               <boost/interprocess/containers/container/detail/algorithms.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ALLOCATION_TYPE_HPP          <boost/interprocess/containers/container/detail/allocation_type.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP               <boost/interprocess/containers/container/detail/config_end.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_DESTROYERS_HPP               <boost/interprocess/containers/container/detail/destroyers.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_FLAT_TREE_HPP                <boost/interprocess/containers/container/detail/flat_tree.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ITERATORS_HPP                <boost/interprocess/containers/container/detail/iterators.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_MATH_FUNCTIONS_HPP           <boost/interprocess/containers/container/detail/math_functions.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_MPL_HPP                      <boost/interprocess/containers/container/detail/mpl.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_MULTIALLOCATION_CHAIN_HPP    <boost/interprocess/containers/container/detail/multiallocation_chain.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_NODE_ALLOC_HOLDER_HPP        <boost/interprocess/containers/container/detail/node_alloc_holder.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_NODE_POOL_IMPL_HPP           <boost/interprocess/containers/container/detail/node_pool_impl.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_PAIR_HPP                     <boost/interprocess/containers/container/detail/pair.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_POOL_COMMON_HPP              <boost/interprocess/containers/container/detail/pool_common.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_PREPROCESSOR_HPP             <boost/interprocess/containers/container/detail/preprocessor.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_TRANSFORM_ITERATOR_HPP       <boost/interprocess/containers/container/detail/transform_iterator.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_TREE_HPP                     <boost/interprocess/containers/container/detail/tree.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_TYPE_TRAITS_HPP              <boost/interprocess/containers/container/detail/type_traits.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_UTILITIES_HPP                <boost/interprocess/containers/container/detail/utilities.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_VALUE_INIT_HPP               <boost/interprocess/containers/container/detail/value_init.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_VARIADIC_TEMPLATES_TOOLS_HPP <boost/interprocess/containers/container/detail/variadic_templates_tools.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_STORED_REF_HPP               <boost/interprocess/containers/container/detail/stored_ref.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_VERSION_TYPE_HPP             <boost/interprocess/containers/container/detail/version_type.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP               <boost/interprocess/containers/container/detail/workaround.hpp>

#else //BOOST_CONTAINER_IN_INTERPROCESS

#define INCLUDE_BOOST_CONTAINER_CONTAINER_FWD_HPP                   <boost/container/container_fwd.hpp>
#define INCLUDE_BOOST_CONTAINER_DEQUE_HPP                           <boost/container/deque.hpp>
#define INCLUDE_BOOST_CONTAINER_FLAT_MAP_HPP                        <boost/container/flat_map.hpp>
#define INCLUDE_BOOST_CONTAINER_FLAT_SET_HPP                        <boost/container/flat_set.hpp>
#define INCLUDE_BOOST_CONTAINER_LIST_HPP                            <boost/container/list.hpp>
#define INCLUDE_BOOST_CONTAINER_MAP_HPP                             <boost/container/map.hpp>
#define INCLUDE_BOOST_CONTAINER_SET_HPP                             <boost/container/set.hpp>
#define INCLUDE_BOOST_CONTAINER_SLIST_HPP                           <boost/container/slist.hpp>
#define INCLUDE_BOOST_CONTAINER_STABLE_VECTOR_HPP                   <boost/container/stable_vector.hpp>
#define INCLUDE_BOOST_CONTAINER_STRING_HPP                          <boost/container/string.hpp>
#define INCLUDE_BOOST_CONTAINER_VECTOR_HPP                          <boost/container/vector.hpp>
//detail
#define INCLUDE_BOOST_CONTAINER_DETAIL_ADAPTIVE_NODE_POOL_IMPL_HPP  <boost/container/detail/adaptive_node_pool_impl.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ADVANCED_INSERT_INT_HPP      <boost/container/detail/advanced_insert_int.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ALGORITHMS_HPP               <boost/container/detail/algorithms.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ALLOCATION_TYPE_HPP          <boost/container/detail/allocation_type.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_BEGIN_HPP             <boost/container/detail/config_begin.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP               <boost/container/detail/config_end.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_DESTROYERS_HPP               <boost/container/detail/destroyers.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_FLAT_TREE_HPP                <boost/container/detail/flat_tree.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_ITERATORS_HPP                <boost/container/detail/iterators.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_MATH_FUNCTIONS_HPP           <boost/container/detail/math_functions.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_MPL_HPP                      <boost/container/detail/mpl.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_MULTIALLOCATION_CHAIN_HPP    <boost/container/detail/multiallocation_chain.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_NODE_ALLOC_HOLDER_HPP        <boost/container/detail/node_alloc_holder.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_NODE_POOL_IMPL_HPP           <boost/container/detail/node_pool_impl.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_PAIR_HPP                     <boost/container/detail/pair.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_POOL_COMMON_HPP              <boost/container/detail/pool_common.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_PREPROCESSOR_HPP             <boost/container/detail/preprocessor.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_TRANSFORM_ITERATOR_HPP       <boost/container/detail/transform_iterator.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_TREE_HPP                     <boost/container/detail/tree.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_TYPE_TRAITS_HPP              <boost/container/detail/type_traits.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_UTILITIES_HPP                <boost/container/detail/utilities.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_VALUE_INIT_HPP               <boost/container/detail/value_init.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_VARIADIC_TEMPLATES_TOOLS_HPP <boost/container/detail/variadic_templates_tools.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_VERSION_TYPE_HPP             <boost/container/detail/version_type.hpp>
#define INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP               <boost/container/detail/workaround.hpp>

#endif   //BOOST_CONTAINER_IN_INTERPROCESS

#endif

#ifdef BOOST_MSVC
   #ifndef _CRT_SECURE_NO_DEPRECATE
   #define  BOOST_CONTAINERS_DETAIL_CRT_SECURE_NO_DEPRECATE
   #define _CRT_SECURE_NO_DEPRECATE
   #endif
   #pragma warning (push)
   #pragma warning (disable : 4702) // unreachable code
   #pragma warning (disable : 4706) // assignment within conditional expression
   #pragma warning (disable : 4127) // conditional expression is constant
   #pragma warning (disable : 4146) // unary minus operator applied to unsigned type, result still unsigned
   #pragma warning (disable : 4284) // odd return type for operator->
   #pragma warning (disable : 4244) // possible loss of data
   #pragma warning (disable : 4251) // "identifier" : class "type" needs to have dll-interface to be used by clients of class "type2"
   #pragma warning (disable : 4267) // conversion from "X" to "Y", possible loss of data
   #pragma warning (disable : 4275) // non DLL-interface classkey "identifier" used as base for DLL-interface classkey "identifier"
   #pragma warning (disable : 4355) // "this" : used in base member initializer list
   #pragma warning (disable : 4503) // "identifier" : decorated name length exceeded, name was truncated
   #pragma warning (disable : 4511) // copy constructor could not be generated
   #pragma warning (disable : 4512) // assignment operator could not be generated
   #pragma warning (disable : 4514) // unreferenced inline removed
   #pragma warning (disable : 4521) // Disable "multiple copy constructors specified"
   #pragma warning (disable : 4522) // "class" : multiple assignment operators specified
   #pragma warning (disable : 4675) // "method" should be declared "static" and have exactly one parameter
   #pragma warning (disable : 4710) // function not inlined
   #pragma warning (disable : 4711) // function selected for automatic inline expansion
   #pragma warning (disable : 4786) // identifier truncated in debug info
   #pragma warning (disable : 4996) // "function": was declared deprecated
   #pragma warning (disable : 4197) // top-level volatile in cast is ignored
   #pragma warning (disable : 4541) // 'typeid' used on polymorphic type 'boost::exception'
                                    //    with /GR-; unpredictable behavior may result
   #pragma warning (disable : 4673) //  throwing '' the following types will not be considered at the catch site
   #pragma warning (disable : 4671) //  the copy constructor is inaccessible
#endif
