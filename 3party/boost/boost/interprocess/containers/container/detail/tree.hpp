//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// This file comes from SGI's stl_tree file. Modified by Ion Gaztanaga 2005.
// Renaming, isolating and porting to generic algorithms. Pointer typedef 
// set to allocator::pointer to allow placing it in shared memory.
//
///////////////////////////////////////////////////////////////////////////////
/*
 *
 * Copyright (c) 1994
 * Hewlett-Packard Company
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Hewlett-Packard Company makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 *
 * Copyright (c) 1996
 * Silicon Graphics Computer Systems, Inc.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  Silicon Graphics makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 */
#ifndef BOOST_CONTAINERS_TREE_HPP
#define BOOST_CONTAINERS_TREE_HPP

#include <boost/interprocess/containers/container/detail/config_begin.hpp>
#include <boost/interprocess/containers/container/detail/workaround.hpp>
#include <boost/interprocess/containers/container/container_fwd.hpp>

#include <boost/interprocess/detail/move.hpp>
#include <boost/pointer_to_other.hpp>
#include <boost/type_traits/has_trivial_destructor.hpp>
#include <boost/detail/no_exceptions_support.hpp>
#include <boost/intrusive/rbtree.hpp>

#include <boost/interprocess/containers/container/detail/utilities.hpp>
#include <boost/interprocess/containers/container/detail/algorithms.hpp>
#include <boost/interprocess/containers/container/detail/node_alloc_holder.hpp>
#include <boost/interprocess/containers/container/detail/destroyers.hpp>
#include <boost/interprocess/containers/container/detail/pair.hpp>
#ifndef BOOST_CONTAINERS_PERFECT_FORWARDING
#include <boost/interprocess/containers/container/detail/preprocessor.hpp>
#endif

#include <utility>   //std::pair
#include <iterator>
#include <algorithm>

namespace boost {
namespace container {
namespace containers_detail {

template<class Key, class Value, class KeyCompare, class KeyOfValue>
struct value_compare_impl
   :  public KeyCompare
{
   typedef Value        value_type;
   typedef KeyCompare   key_compare; 
   typedef KeyOfValue   key_of_value;
   typedef Key          key_type;

   value_compare_impl(key_compare kcomp)
      :  key_compare(kcomp)
   {}

   const key_compare &key_comp() const
   {  return static_cast<const key_compare &>(*this);  }

   key_compare &key_comp()
   {  return static_cast<key_compare &>(*this);  }

   template<class A, class B>
   bool operator()(const A &a, const B &b) const
   {  return key_compare::operator()(KeyOfValue()(a), KeyOfValue()(b)); }
};

template<class VoidPointer>
struct rbtree_hook
{
   typedef typename containers_detail::bi::make_set_base_hook
      < containers_detail::bi::void_pointer<VoidPointer>
      , containers_detail::bi::link_mode<containers_detail::bi::normal_link>
      , containers_detail::bi::optimize_size<true>
      >::type  type;
};

template<class T>
struct rbtree_type
{
   typedef T type;
};

template<class T1, class T2>
struct rbtree_type< std::pair<T1, T2> >
{
   typedef pair<T1, T2> type;
};

template <class T, class VoidPointer>
struct rbtree_node
   :  public rbtree_hook<VoidPointer>::type
{
   typedef typename rbtree_hook<VoidPointer>::type hook_type;

   typedef T value_type;
   typedef typename rbtree_type<T>::type internal_type;

   typedef rbtree_node<T, VoidPointer> node_type;

   #ifndef BOOST_CONTAINERS_PERFECT_FORWARDING

   rbtree_node()
      : m_data()
   {}

   rbtree_node(const rbtree_node &other)
      : m_data(other.m_data)
   {}

   #define BOOST_PP_LOCAL_MACRO(n)                                                           \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                \
   rbtree_node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                        \
      : m_data(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _))                     \
   {}                                                                                        \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #else //#ifndef BOOST_CONTAINERS_PERFECT_FORWARDING

   rbtree_node()
      : m_data()
   {}

   template<class ...Args>
   rbtree_node(Args &&...args)
      : m_data(boost::interprocess::forward<Args>(args)...)
   {}
   #endif//#ifndef BOOST_CONTAINERS_PERFECT_FORWARDING

   rbtree_node &operator=(const rbtree_node &other)
   {  do_assign(other.m_data);   return *this;  }

   T &get_data()
   {
      T* ptr = reinterpret_cast<T*>(&this->m_data);
      return *ptr;
   }

   const T &get_data() const
   {
      const T* ptr = reinterpret_cast<const T*>(&this->m_data);
      return *ptr;
   }

   private:
   internal_type m_data;

   template<class A, class B>
   void do_assign(const std::pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = p.first;
      m_data.second  = p.second;
   }

   template<class A, class B>
   void do_assign(const pair<const A, B> &p)
   {
      const_cast<A&>(m_data.first) = p.first;
      m_data.second  = p.second;
   }

   template<class V>
   void do_assign(const V &v)
   {  m_data = v; }

   public:
   template<class Convertible>
   static void construct(node_type *ptr, BOOST_INTERPROCESS_FWD_REF(Convertible) convertible)
   {  new(ptr) node_type(boost::interprocess::forward<Convertible>(convertible));  }
};

}//namespace containers_detail {
#if !defined(BOOST_HAS_RVALUE_REFS)
template<class T, class VoidPointer>
struct has_own_construct_from_it
   < boost::container::containers_detail::rbtree_node<T, VoidPointer> >
{
   static const bool value = true;
};
#endif
namespace containers_detail {

template<class A, class ValueCompare>
struct intrusive_rbtree_type
{
   typedef typename A::value_type                  value_type;
   typedef typename boost::pointer_to_other
      <typename A::pointer, void>::type            void_pointer;
   typedef typename containers_detail::rbtree_node
         <value_type, void_pointer>                node_type;
   typedef node_compare<ValueCompare, node_type>   node_compare_type;
   typedef typename containers_detail::bi::make_rbtree
      <node_type
      ,containers_detail::bi::compare<node_compare_type>
      ,containers_detail::bi::base_hook<typename rbtree_hook<void_pointer>::type>
      ,containers_detail::bi::constant_time_size<true>
      ,containers_detail::bi::size_type<typename A::size_type>
      >::type                                      container_type;
   typedef container_type                          type ;
};

}  //namespace containers_detail {

namespace containers_detail {

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
class rbtree
   : protected containers_detail::node_alloc_holder
      <A, typename containers_detail::intrusive_rbtree_type
         <A, value_compare_impl<Key, Value, KeyCompare, KeyOfValue>  
         >::type
      >
{
   typedef typename containers_detail::intrusive_rbtree_type
         <A, value_compare_impl
            <Key, Value, KeyCompare, KeyOfValue>
         >::type                                      Icont;
   typedef containers_detail::node_alloc_holder<A, Icont>        AllocHolder;
   typedef typename AllocHolder::NodePtr              NodePtr;
   typedef rbtree < Key, Value, KeyOfValue
                  , KeyCompare, A>                    ThisType;
   typedef typename AllocHolder::NodeAlloc            NodeAlloc;
   typedef typename AllocHolder::ValAlloc             ValAlloc;
   typedef typename AllocHolder::Node                 Node;
   typedef typename Icont::iterator                   iiterator;
   typedef typename Icont::const_iterator             iconst_iterator;
   typedef containers_detail::allocator_destroyer<NodeAlloc>     Destroyer;
   typedef typename AllocHolder::allocator_v1         allocator_v1;
   typedef typename AllocHolder::allocator_v2         allocator_v2;
   typedef typename AllocHolder::alloc_version        alloc_version;

   class RecyclingCloner;
   friend class RecyclingCloner;
   
   class RecyclingCloner
   {
      public:
      RecyclingCloner(AllocHolder &holder, Icont &irbtree)
         :  m_holder(holder), m_icont(irbtree)
      {}

      NodePtr operator()(const Node &other) const
      {
//         if(!m_icont.empty()){
         if(NodePtr p = m_icont.unlink_leftmost_without_rebalance()){
            //First recycle a node (this can't throw)
            //NodePtr p = m_icont.unlink_leftmost_without_rebalance();
            try{
               //This can throw
               *p = other;
               return p;
            }
            catch(...){
               //If there is an exception destroy the whole source
               m_holder.destroy_node(p);
               while((p = m_icont.unlink_leftmost_without_rebalance())){
                  m_holder.destroy_node(p);
               }
               throw;
            }
         }
         else{
            return m_holder.create_node(other);
         }
      }

      AllocHolder &m_holder;
      Icont &m_icont;
   };
   BOOST_COPYABLE_AND_MOVABLE(rbtree)

   public:

   typedef Key                                        key_type;
   typedef Value                                      value_type;
   typedef A                                          allocator_type;
   typedef KeyCompare                                 key_compare;
   typedef value_compare_impl< Key, Value
                        , KeyCompare, KeyOfValue>     value_compare;
   typedef typename A::pointer                        pointer;
   typedef typename A::const_pointer                  const_pointer;
   typedef typename A::reference                      reference;
   typedef typename A::const_reference                const_reference;
   typedef typename A::size_type                      size_type;
   typedef typename A::difference_type                difference_type;
   typedef difference_type                            rbtree_difference_type;
   typedef pointer                                    rbtree_pointer;
   typedef const_pointer                              rbtree_const_pointer;
   typedef reference                                  rbtree_reference;
   typedef const_reference                            rbtree_const_reference;
   typedef NodeAlloc                                  stored_allocator_type;

   private:

   template<class KeyValueCompare>
   struct key_node_compare
      :  private KeyValueCompare
   {
      key_node_compare(KeyValueCompare comp)
         :  KeyValueCompare(comp)
      {}
      
      template<class KeyType>
      bool operator()(const Node &n, const KeyType &k) const
      {  return KeyValueCompare::operator()(n.get_data(), k);  }

      template<class KeyType>
      bool operator()(const KeyType &k, const Node &n) const
      {  return KeyValueCompare::operator()(k, n.get_data());  }
   };

   typedef key_node_compare<value_compare>  KeyNodeCompare;

   public:
   //rbtree const_iterator
   class const_iterator
      : public std::iterator
         < std::bidirectional_iterator_tag
         , value_type            , rbtree_difference_type
         , rbtree_const_pointer  , rbtree_const_reference>
   {
      protected:
      typedef typename Icont::iterator  iiterator;
      iiterator m_it;
      explicit const_iterator(iiterator it)  : m_it(it){}
      void prot_incr() { ++m_it; }
      void prot_decr() { --m_it; }

      private:
      iiterator get()
      {  return this->m_it;   }

      public:
      friend class rbtree <Key, Value, KeyOfValue, KeyCompare, A>;
      typedef rbtree_difference_type        difference_type;

      //Constructors
      const_iterator()
         :  m_it()
      {}

      //Pointer like operators
      const_reference operator*()  const 
      { return  m_it->get_data();  }

      const_pointer   operator->() const 
      { return  const_pointer(&m_it->get_data()); }

      //Increment / Decrement
      const_iterator& operator++()       
      { prot_incr();  return *this; }

      const_iterator operator++(int)      
      { iiterator tmp = m_it; ++*this; return const_iterator(tmp);  }

      const_iterator& operator--()
      {   prot_decr(); return *this;   }

      const_iterator operator--(int)
      {  iiterator tmp = m_it; --*this; return const_iterator(tmp); }

      //Comparison operators
      bool operator==   (const const_iterator& r)  const
      {  return m_it == r.m_it;  }

      bool operator!=   (const const_iterator& r)  const
      {  return m_it != r.m_it;  }
   };

   //rbtree iterator
   class iterator : public const_iterator
   {
      private:
      explicit iterator(iiterator it)
         :  const_iterator(it)
      {}
   
      iiterator get()
      {  return this->m_it;   }

      public:
      friend class rbtree <Key, Value, KeyOfValue, KeyCompare, A>;
      typedef rbtree_pointer       pointer;
      typedef rbtree_reference     reference;

      //Constructors
      iterator(){}

      //Pointer like operators
      reference operator*()  const {  return  this->m_it->get_data();  }
      pointer   operator->() const {  return  pointer(&this->m_it->get_data());  }

      //Increment / Decrement
      iterator& operator++()  
         { this->prot_incr(); return *this;  }

      iterator operator++(int)
         { iiterator tmp = this->m_it; ++*this; return iterator(tmp); }
      
      iterator& operator--()
         {  this->prot_decr(); return *this;  }

      iterator operator--(int)
         {  iterator tmp = *this; --*this; return tmp; }
   };

   typedef std::reverse_iterator<iterator>        reverse_iterator;
   typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

   rbtree(const key_compare& comp = key_compare(), 
           const allocator_type& a = allocator_type())
      : AllocHolder(a, comp)
   {}

   template <class InputIterator>
   rbtree(InputIterator first, InputIterator last, const key_compare& comp,
          const allocator_type& a, bool unique_insertion)
      : AllocHolder(a, comp)
   {
      typedef typename std::iterator_traits<InputIterator>::iterator_category ItCat;
      priv_create_and_insert_nodes(first, last, unique_insertion, alloc_version(), ItCat());
   }

   template <class InputIterator>
   rbtree( ordered_range_t, InputIterator first, InputIterator last
         , const key_compare& comp = key_compare(), const allocator_type& a = allocator_type())
      : AllocHolder(a, comp)
   {
      typedef typename std::iterator_traits<InputIterator>::iterator_category ItCat;
      priv_create_and_insert_ordered_nodes(first, last, alloc_version(), ItCat());
   }

   rbtree(const rbtree& x) 
      :  AllocHolder(x, x.key_comp())
   {
      this->icont().clone_from
         (x.icont(), typename AllocHolder::cloner(*this), Destroyer(this->node_alloc()));
   }

   rbtree(BOOST_INTERPROCESS_RV_REF(rbtree) x) 
      :  AllocHolder(x, x.key_comp())
   {  this->swap(x);  }

   ~rbtree()
   {} //AllocHolder clears the tree

   rbtree& operator=(BOOST_INTERPROCESS_COPY_ASSIGN_REF(rbtree) x)
   {
      if (this != &x) {
         //Transfer all the nodes to a temporary tree
         //If anything goes wrong, all the nodes will be destroyed
         //automatically
         Icont other_tree(this->icont().value_comp());
         other_tree.swap(this->icont());

         //Now recreate the source tree reusing nodes stored by other_tree
         this->icont().clone_from
            (x.icont()
            , RecyclingCloner(*this, other_tree)
            //, AllocHolder::cloner(*this)
            , Destroyer(this->node_alloc()));

         //If there are remaining nodes, destroy them
         NodePtr p;
         while((p = other_tree.unlink_leftmost_without_rebalance())){
            AllocHolder::destroy_node(p);
         }
      }
      return *this;
   }

   rbtree& operator=(BOOST_INTERPROCESS_RV_REF(rbtree) mx)
   {  this->clear(); this->swap(mx);   return *this;  }

   public:    
   // accessors:
   value_compare value_comp() const 
   {  return this->icont().value_comp().value_comp(); }

   key_compare key_comp() const 
   {  return this->icont().value_comp().value_comp().key_comp(); }

   allocator_type get_allocator() const 
   {  return allocator_type(this->node_alloc()); }

   const stored_allocator_type &get_stored_allocator() const 
   {  return this->node_alloc(); }

   stored_allocator_type &get_stored_allocator()
   {  return this->node_alloc(); }

   iterator begin()
   { return iterator(this->icont().begin()); }

   const_iterator begin() const
   {  return this->cbegin();  }

   iterator end()
   {  return iterator(this->icont().end());  }

   const_iterator end() const
   {  return this->cend();  }

   reverse_iterator rbegin()
   {  return reverse_iterator(end());  }

   const_reverse_iterator rbegin() const
   {  return this->crbegin();  }

   reverse_iterator rend()
   {  return reverse_iterator(begin());   }

   const_reverse_iterator rend() const
   {  return this->crend();   }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator cbegin() const 
   { return const_iterator(this->non_const_icont().begin()); }

   //! <b>Effects</b>: Returns a const_iterator to the end of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator cend() const 
   { return const_iterator(this->non_const_icont().end()); }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning 
   //! of the reversed container. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crbegin() const 
   { return const_reverse_iterator(cend()); } 

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed container. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crend() const 
   { return const_reverse_iterator(cbegin()); }

   bool empty() const 
   {  return !this->size();  }

   size_type size() const 
   {  return this->icont().size();   }

   size_type max_size() const 
   {  return AllocHolder::max_size();  }

   void swap(ThisType& x)
   {  AllocHolder::swap(x);   }

   public:

   typedef typename Icont::insert_commit_data insert_commit_data;

   // insert/erase
   std::pair<iterator,bool> insert_unique_check
      (const key_type& key, insert_commit_data &data)
   {
      std::pair<iiterator, bool> ret = 
         this->icont().insert_unique_check(key, KeyNodeCompare(value_comp()), data);
      return std::pair<iterator, bool>(iterator(ret.first), ret.second);
   }

   std::pair<iterator,bool> insert_unique_check
      (const_iterator hint, const key_type& key, insert_commit_data &data)
   {
      std::pair<iiterator, bool> ret = 
         this->icont().insert_unique_check(hint.get(), key, KeyNodeCompare(value_comp()), data);
      return std::pair<iterator, bool>(iterator(ret.first), ret.second);
   }

   iterator insert_unique_commit(const value_type& v, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(v);
      iiterator it(this->icont().insert_unique_commit(*tmp, data));
      return iterator(it);
   }

   template<class MovableConvertible>
   iterator insert_unique_commit
      (BOOST_INTERPROCESS_FWD_REF(MovableConvertible) mv, insert_commit_data &data)
   {
      NodePtr tmp = AllocHolder::create_node(boost::interprocess::forward<MovableConvertible>(mv));
      iiterator it(this->icont().insert_unique_commit(*tmp, data));
      return iterator(it);
   }

   std::pair<iterator,bool> insert_unique(const value_type& v)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(v), data);
      if(!ret.second)
         return ret;
      return std::pair<iterator,bool>
         (this->insert_unique_commit(v, data), true);
   }

   template<class MovableConvertible>
   std::pair<iterator,bool> insert_unique(BOOST_INTERPROCESS_FWD_REF(MovableConvertible) mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(mv), data);
      if(!ret.second)
         return ret;
      return std::pair<iterator,bool>
         (this->insert_unique_commit(boost::interprocess::forward<MovableConvertible>(mv), data), true);
   }

   private:
   iterator emplace_unique_impl(NodePtr p)
   {
      value_type &v = p->get_data();
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(KeyOfValue()(v), data);
      if(!ret.second){
         Destroyer(this->node_alloc())(p);
         return ret.first;
      }
      return iterator(iiterator(this->icont().insert_unique_commit(*p, data)));
   }

   iterator emplace_unique_hint_impl(const_iterator hint, NodePtr p)
   {
      value_type &v = p->get_data();
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(v), data);
      if(!ret.second){
         Destroyer(this->node_alloc())(p);
         return ret.first;
      }
      return iterator(iiterator(this->icont().insert_unique_commit(*p, data)));
   }

   public:

   #ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   template <class... Args>
   iterator emplace_unique(Args&&... args)
   {  return this->emplace_unique_impl(AllocHolder::create_node(boost::interprocess::forward<Args>(args)...));   }

   template <class... Args>
   iterator emplace_hint_unique(const_iterator hint, Args&&... args)
   {  return this->emplace_unique_hint_impl(hint, AllocHolder::create_node(boost::interprocess::forward<Args>(args)...));   }

   template <class... Args>
   iterator emplace_equal(Args&&... args)
   {
      NodePtr p(AllocHolder::create_node(boost::interprocess::forward<Args>(args)...));
      return iterator(this->icont().insert_equal(this->icont().end(), *p));
   }

   template <class... Args>
   iterator emplace_hint_equal(const_iterator hint, Args&&... args)
   {
      NodePtr p(AllocHolder::create_node(boost::interprocess::forward<Args>(args)...));
      return iterator(this->icont().insert_equal(hint.get(), *p));
   }

   #else //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   iterator emplace_unique()
   {  return this->emplace_unique_impl(AllocHolder::create_node());   }

   iterator emplace_hint_unique(const_iterator hint)
   {  return this->emplace_unique_hint_impl(hint, AllocHolder::create_node());   }

   iterator emplace_equal()
   {
      NodePtr p(AllocHolder::create_node());
      return iterator(this->icont().insert_equal(this->icont().end(), *p));
   }

   iterator emplace_hint_equal(const_iterator hint)
   {
      NodePtr p(AllocHolder::create_node());
      return iterator(this->icont().insert_equal(hint.get(), *p));
   }

   #define BOOST_PP_LOCAL_MACRO(n)                                                                          \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                               \
   iterator emplace_unique(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                           \
   {                                                                                                        \
      return this->emplace_unique_impl                                                                      \
         (AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _)));              \
   }                                                                                                        \
                                                                                                            \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                               \
   iterator emplace_hint_unique(const_iterator hint, BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _)) \
   {                                                                                                        \
      return this->emplace_unique_hint_impl                                                                 \
         (hint, AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _)));        \
   }                                                                                                        \
                                                                                                            \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                               \
   iterator emplace_equal(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                            \
   {                                                                                                        \
      NodePtr p(AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _)));        \
      return iterator(this->icont().insert_equal(this->icont().end(), *p));                                 \
   }                                                                                                        \
                                                                                                            \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                               \
   iterator emplace_hint_equal(const_iterator hint, BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))  \
   {                                                                                                        \
      NodePtr p(AllocHolder::create_node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _)));        \
      return iterator(this->icont().insert_equal(hint.get(), *p));                                          \
   }                                                                                                        \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif   //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   iterator insert_unique(const_iterator hint, const value_type& v)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(v), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(v, data);
   }

   template<class MovableConvertible>
   iterator insert_unique(const_iterator hint, BOOST_INTERPROCESS_FWD_REF(MovableConvertible) mv)
   {
      insert_commit_data data;
      std::pair<iterator,bool> ret =
         this->insert_unique_check(hint, KeyOfValue()(mv), data);
      if(!ret.second)
         return ret.first;
      return this->insert_unique_commit(boost::interprocess::forward<MovableConvertible>(mv), data);
   }

   template <class InputIterator>
   void insert_unique(InputIterator first, InputIterator last)
   {
      if(this->empty()){
         //Insert with end hint, to achieve linear
         //complexity if [first, last) is ordered
         const_iterator end(this->end());
         for( ; first != last; ++first)
            this->insert_unique(end, *first);
      }
      else{
         for( ; first != last; ++first)
            this->insert_unique(*first);
      }
   }

   iterator insert_equal(const value_type& v)
   {
      NodePtr p(AllocHolder::create_node(v));
      return iterator(this->icont().insert_equal(this->icont().end(), *p));
   }

   template<class MovableConvertible>
   iterator insert_equal(BOOST_INTERPROCESS_FWD_REF(MovableConvertible) mv)
   {
      NodePtr p(AllocHolder::create_node(boost::interprocess::forward<MovableConvertible>(mv)));
      return iterator(this->icont().insert_equal(this->icont().end(), *p));
   }

   iterator insert_equal(const_iterator hint, const value_type& v)
   {
      NodePtr p(AllocHolder::create_node(v));
      return iterator(this->icont().insert_equal(hint.get(), *p));
   }

   template<class MovableConvertible>
   iterator insert_equal(const_iterator hint, BOOST_INTERPROCESS_FWD_REF(MovableConvertible) mv)
   {
      NodePtr p(AllocHolder::create_node(boost::interprocess::forward<MovableConvertible>(mv)));
      return iterator(this->icont().insert_equal(hint.get(), *p));
   }

   template <class InputIterator>
   void insert_equal(InputIterator first, InputIterator last)
   {
      //Insert with end hint, to achieve linear
      //complexity if [first, last) is ordered
      const_iterator end(this->cend());
      for( ; first != last; ++first)
         this->insert_equal(end, *first);
   }

   iterator erase(const_iterator position)
   {  return iterator(this->icont().erase_and_dispose(position.get(), Destroyer(this->node_alloc()))); }

   size_type erase(const key_type& k)
   {  return AllocHolder::erase_key(k, KeyNodeCompare(value_comp()), alloc_version()); }

   iterator erase(const_iterator first, const_iterator last)
   {  return iterator(AllocHolder::erase_range(first.get(), last.get(), alloc_version())); }

   void clear() 
   {  AllocHolder::clear(alloc_version());  }

   // set operations:
   iterator find(const key_type& k)
   {  return iterator(this->icont().find(k, KeyNodeCompare(value_comp())));  }

   const_iterator find(const key_type& k) const
   {  return const_iterator(this->non_const_icont().find(k, KeyNodeCompare(value_comp())));  }

   size_type count(const key_type& k) const
   {  return size_type(this->icont().count(k, KeyNodeCompare(value_comp()))); }

   iterator lower_bound(const key_type& k)
   {  return iterator(this->icont().lower_bound(k, KeyNodeCompare(value_comp())));  }

   const_iterator lower_bound(const key_type& k) const
   {  return const_iterator(this->non_const_icont().lower_bound(k, KeyNodeCompare(value_comp())));  }

   iterator upper_bound(const key_type& k)
   {  return iterator(this->icont().upper_bound(k, KeyNodeCompare(value_comp())));   }

   const_iterator upper_bound(const key_type& k) const
   {  return const_iterator(this->non_const_icont().upper_bound(k, KeyNodeCompare(value_comp())));  }

   std::pair<iterator,iterator> equal_range(const key_type& k)
   {  
      std::pair<iiterator, iiterator> ret =
         this->icont().equal_range(k, KeyNodeCompare(value_comp()));
      return std::pair<iterator,iterator>(iterator(ret.first), iterator(ret.second));
   }

   std::pair<const_iterator, const_iterator> equal_range(const key_type& k) const
   {  
      std::pair<iiterator, iiterator> ret =
         this->non_const_icont().equal_range(k, KeyNodeCompare(value_comp()));
      return std::pair<const_iterator,const_iterator>
         (const_iterator(ret.first), const_iterator(ret.second));
   }

   private:
   //Iterator range version
   template<class InpIterator>
   void priv_create_and_insert_nodes
      (InpIterator beg, InpIterator end, bool unique, allocator_v1, std::input_iterator_tag)
   {
      if(unique){
         for (; beg != end; ++beg){
            this->insert_unique(*beg);
         }
      }
      else{
         for (; beg != end; ++beg){
            this->insert_equal(*beg);
         }
      }
   }

   template<class InpIterator>
   void priv_create_and_insert_nodes
      (InpIterator beg, InpIterator end, bool unique, allocator_v2, std::input_iterator_tag)
   {  //Just forward to the default one
      priv_create_and_insert_nodes(beg, end, unique, allocator_v1(), std::input_iterator_tag());
   }

   class insertion_functor;
   friend class insertion_functor;

   class insertion_functor
   {
      Icont &icont_;

      public:
      insertion_functor(Icont &icont)
         :  icont_(icont)
      {}

      void operator()(Node &n)
      {  this->icont_.insert_equal(this->icont_.cend(), n); }
   };


   template<class FwdIterator>
   void priv_create_and_insert_nodes
      (FwdIterator beg, FwdIterator end, bool unique, allocator_v2, std::forward_iterator_tag)
   {
      if(beg != end){
         if(unique){
            priv_create_and_insert_nodes(beg, end, unique, allocator_v2(), std::input_iterator_tag());
         }
         else{
            //Optimized allocation and construction
            this->allocate_many_and_construct
               (beg, std::distance(beg, end), insertion_functor(this->icont()));
         }
      }
   }

   //Iterator range version
   template<class InpIterator>
   void priv_create_and_insert_ordered_nodes
      (InpIterator beg, InpIterator end, allocator_v1, std::input_iterator_tag)
   {
      const_iterator cend_n(this->cend());
      for (; beg != end; ++beg){
         this->insert_before(cend_n, *beg);
      }
   }

   template<class InpIterator>
   void priv_create_and_insert_ordered_nodes
      (InpIterator beg, InpIterator end, allocator_v2, std::input_iterator_tag)
   {  //Just forward to the default one
      priv_create_and_insert_ordered_nodes(beg, end, allocator_v1(), std::input_iterator_tag());
   }

   class back_insertion_functor;
   friend class back_insertion_functor;

   class back_insertion_functor
   {
      Icont &icont_;

      public:
      back_insertion_functor(Icont &icont)
         :  icont_(icont)
      {}

      void operator()(Node &n)
      {  this->icont_.push_back(n); }
   };


   template<class FwdIterator>
   void priv_create_and_insert_ordered_nodes
      (FwdIterator beg, FwdIterator end, allocator_v2, std::forward_iterator_tag)
   {
      if(beg != end){
         //Optimized allocation and construction
         this->allocate_many_and_construct
            (beg, std::distance(beg, end), back_insertion_functor(this->icont()));
      }
   }
};

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator==(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  return x.size() == y.size() &&
         std::equal(x.begin(), x.end(), y.begin());
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator<(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
          const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  return std::lexicographical_compare(x.begin(), x.end(), 
                                      y.begin(), y.end());
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator!=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(x == y);
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator>(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
          const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return y < x;
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator<=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(y < x);
}

template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline bool 
operator>=(const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
           const rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y) {
  return !(x < y);
}


template <class Key, class Value, class KeyOfValue, 
          class KeyCompare, class A>
inline void 
swap(rbtree<Key,Value,KeyOfValue,KeyCompare,A>& x, 
     rbtree<Key,Value,KeyOfValue,KeyCompare,A>& y)
{
  x.swap(y);
}

} //namespace containers_detail {
} //namespace container {
/*
//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class K, class V, class KOV, 
class C, class A>
struct has_trivial_destructor_after_move
   <boost::container::containers_detail::rbtree<K, V, KOV, C, A> >
{
   static const bool value = has_trivial_destructor<A>::value && has_trivial_destructor<C>::value;
};
*/
} //namespace boost  {

#include <boost/interprocess/containers/container/detail/config_end.hpp>

#endif //BOOST_CONTAINERS_TREE_HPP
