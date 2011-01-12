//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//

#ifndef BOOST_CONTAINERS_LIST_HPP_
#define BOOST_CONTAINERS_LIST_HPP_

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include "detail/config_begin.hpp"
#include INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP
#include INCLUDE_BOOST_CONTAINER_CONTAINER_FWD_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_VERSION_TYPE_HPP
#include INCLUDE_BOOST_CONTAINER_MOVE_HPP
#include <boost/pointer_to_other.hpp>
#include INCLUDE_BOOST_CONTAINER_DETAIL_UTILITIES_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_ALGORITHMS_HPP
#include <boost/type_traits/has_trivial_destructor.hpp>
#include INCLUDE_BOOST_CONTAINER_DETAIL_MPL_HPP
#include <boost/intrusive/list.hpp>
#include INCLUDE_BOOST_CONTAINER_DETAIL_NODE_ALLOC_HOLDER_HPP

#if defined(BOOST_CONTAINERS_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)
#else
//Preprocessor library to emulate perfect forwarding
#include INCLUDE_BOOST_CONTAINER_DETAIL_PREPROCESSOR_HPP 
#endif

#include <stdexcept>
#include <iterator>
#include <utility>
#include <memory>
#include <functional>
#include <algorithm>
#include <stdexcept>

#ifdef BOOST_CONTAINER_DOXYGEN_INVOKED
namespace boost {
namespace container {
#else
namespace boost {
namespace container {
#endif

/// @cond
namespace containers_detail {

template<class VoidPointer>
struct list_hook
{
   typedef typename containers_detail::bi::make_list_base_hook
      <containers_detail::bi::void_pointer<VoidPointer>, containers_detail::bi::link_mode<containers_detail::bi::normal_link> >::type type;
};

template <class T, class VoidPointer>
struct list_node
   :  public list_hook<VoidPointer>::type
{

   #if defined(BOOST_CONTAINERS_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   list_node()
      : m_data()
   {}

   template<class ...Args>
   list_node(Args &&...args)
      : m_data(BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...)
   {}

   #else //#ifndef BOOST_CONTAINERS_PERFECT_FORWARDING

   list_node()
      : m_data()
   {}

   #define BOOST_PP_LOCAL_MACRO(n)                                                           \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                \
   list_node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                          \
      : m_data(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _))                     \
   {}                                                                                        \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif//#ifndef BOOST_CONTAINERS_PERFECT_FORWARDING

   T m_data;
};

template<class A>
struct intrusive_list_type
{
   typedef typename A::value_type               value_type;
   typedef typename boost::pointer_to_other
      <typename A::pointer, void>::type         void_pointer;
   typedef typename containers_detail::list_node
         <value_type, void_pointer>             node_type;
   typedef typename containers_detail::bi::make_list
      < node_type
      , containers_detail::bi::base_hook<typename list_hook<void_pointer>::type>
      , containers_detail::bi::constant_time_size<true>
      , containers_detail::bi::size_type<typename A::size_type>
      >::type                                   container_type;
   typedef container_type                       type ;
};

}  //namespace containers_detail {
/// @endcond

//! A list is a doubly linked list. That is, it is a Sequence that supports both
//! forward and backward traversal, and (amortized) constant time insertion and 
//! removal of elements at the beginning or the end, or in the middle. Lists have 
//! the important property that insertion and splicing do not invalidate iterators 
//! to list elements, and that even removal invalidates only the iterators that point 
//! to the elements that are removed. The ordering of iterators may be changed 
//! (that is, list<T>::iterator might have a different predecessor or successor 
//! after a list operation than it did before), but the iterators themselves will 
//! not be invalidated or made to point to different elements unless that invalidation 
//! or mutation is explicit.
template <class T, class A>
class list 
   : protected containers_detail::node_alloc_holder
      <A, typename containers_detail::intrusive_list_type<A>::type>
{
   /// @cond
   typedef typename containers_detail::
      move_const_ref_type<T>::type                    insert_const_ref_type;
   typedef typename 
      containers_detail::intrusive_list_type<A>::type Icont;
   typedef list <T, A>                                ThisType;
   typedef containers_detail::node_alloc_holder<A, Icont>        AllocHolder;
   typedef typename AllocHolder::NodePtr              NodePtr;
   typedef typename AllocHolder::NodeAlloc            NodeAlloc;
   typedef typename AllocHolder::ValAlloc             ValAlloc;
   typedef typename AllocHolder::Node                 Node;
   typedef containers_detail::allocator_destroyer<NodeAlloc>     Destroyer;
   typedef typename AllocHolder::allocator_v1         allocator_v1;
   typedef typename AllocHolder::allocator_v2         allocator_v2;
   typedef typename AllocHolder::alloc_version        alloc_version;

   class equal_to_value
   {
      typedef typename AllocHolder::value_type value_type;
      const value_type &t_;

      public:
      equal_to_value(const value_type &t)
         :  t_(t)
      {}

      bool operator()(const value_type &t)const
      {  return t_ == t;   }
   };

   template<class Pred>
   struct ValueCompareToNodeCompare
      :  Pred
   {
      ValueCompareToNodeCompare(Pred pred)
         :  Pred(pred)
      {}

      bool operator()(const Node &a, const Node &b) const
      {  return static_cast<const Pred&>(*this)(a.m_data, b.m_data);  }

      bool operator()(const Node &a) const
      {  return static_cast<const Pred&>(*this)(a.m_data);  }
   };
   /// @endcond

   public:
   //! The type of object, T, stored in the list
   typedef T                                       value_type;
   //! Pointer to T
   typedef typename A::pointer                     pointer;
   //! Const pointer to T
   typedef typename A::const_pointer               const_pointer;
   //! Reference to T
   typedef typename A::reference                   reference;
   //! Const reference to T
   typedef typename A::const_reference             const_reference;
   //! An unsigned integral type
   typedef typename A::size_type                   size_type;
   //! A signed integral type
   typedef typename A::difference_type             difference_type;
   //! The allocator type
   typedef A                                       allocator_type;
   //! The stored allocator type
   typedef NodeAlloc                               stored_allocator_type;

   /// @cond
   private:
   BOOST_MOVE_MACRO_COPYABLE_AND_MOVABLE(list)
   typedef difference_type                         list_difference_type;
   typedef pointer                                 list_pointer;
   typedef const_pointer                           list_const_pointer;
   typedef reference                               list_reference;
   typedef const_reference                         list_const_reference;
   /// @endcond

   public:
   //! Const iterator used to iterate through a list. 
   class const_iterator
      /// @cond
      : public std::iterator<std::bidirectional_iterator_tag, 
                                 value_type,         list_difference_type, 
                                 list_const_pointer, list_const_reference>
   {

      protected:
      typename Icont::iterator m_it;
      explicit const_iterator(typename Icont::iterator it)  : m_it(it){}
      void prot_incr() { ++m_it; }
      void prot_decr() { --m_it; }

      private:
      typename Icont::iterator get()
      {  return this->m_it;   }

      public:
      friend class list<T, A>;
      typedef list_difference_type        difference_type;

      //Constructors
      const_iterator()
         :  m_it()
      {}

      //Pointer like operators
      const_reference operator*()  const 
      { return  m_it->m_data;  }

      const_pointer   operator->() const 
      { return  const_pointer(&m_it->m_data); }

      //Increment / Decrement
      const_iterator& operator++()       
      { prot_incr();  return *this; }

      const_iterator operator++(int)      
      { typename Icont::iterator tmp = m_it; ++*this; return const_iterator(tmp);  }

      const_iterator& operator--()
      {   prot_decr(); return *this;   }

      const_iterator operator--(int)
      {  typename Icont::iterator tmp = m_it; --*this; return const_iterator(tmp); }

      //Comparison operators
      bool operator==   (const const_iterator& r)  const
      {  return m_it == r.m_it;  }

      bool operator!=   (const const_iterator& r)  const
      {  return m_it != r.m_it;  }
   }
   /// @endcond
   ;

   //! Iterator used to iterate through a list
   class iterator
   /// @cond
      : public const_iterator
   {

      private:
      explicit iterator(typename Icont::iterator it)
         :  const_iterator(it)
      {}
   
      typename Icont::iterator get()
      {  return this->m_it;   }

      public:
      friend class list<T, A>;
      typedef list_pointer       pointer;
      typedef list_reference     reference;

      //Constructors
      iterator(){}

      //Pointer like operators
      reference operator*()  const {  return  this->m_it->m_data;  }
      pointer   operator->() const {  return  pointer(&this->m_it->m_data);  }

      //Increment / Decrement
      iterator& operator++()  
         { this->prot_incr(); return *this;  }

      iterator operator++(int)
         { typename Icont::iterator tmp = this->m_it; ++*this; return iterator(tmp); }
      
      iterator& operator--()
         {  this->prot_decr(); return *this;  }

      iterator operator--(int)
         {  iterator tmp = *this; --*this; return tmp; }
   };
   /// @endcond

   //! Iterator used to iterate backwards through a list. 
   typedef std::reverse_iterator<iterator>        reverse_iterator;
   //! Const iterator used to iterate backwards through a list. 
   typedef std::reverse_iterator<const_iterator>  const_reverse_iterator;

   //! <b>Effects</b>: Constructs a list taking the allocator as parameter.
   //! 
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   explicit list(const allocator_type &a = A()) 
      : AllocHolder(a)
   {}

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts n copies of value.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's default or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to n.
   list(size_type n)
      : AllocHolder(A())
   {  this->resize(n);  }

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts n copies of value.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's default or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to n.
   list(size_type n, const T& value, const A& a = A())
      : AllocHolder(a)
   {  this->insert(this->cbegin(), n, value);  }

   //! <b>Effects</b>: Copy constructs a list.
   //!
   //! <b>Postcondition</b>: x == *this.
   //! 
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor throws.
   //! 
   //! <b>Complexity</b>: Linear to the elements x contains.
   list(const list& x) 
      : AllocHolder(x)
   {  this->insert(this->cbegin(), x.begin(), x.end());   }

   //! <b>Effects</b>: Move constructor. Moves mx's resources to *this.
   //!
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   list(BOOST_MOVE_MACRO_RV_REF(list) x)
      : AllocHolder(BOOST_CONTAINER_MOVE_NAMESPACE::move(static_cast<AllocHolder&>(x)))
   {}

   //! <b>Effects</b>: Constructs a list that will use a copy of allocator a
   //!   and inserts a copy of the range [first, last) in the list.
   //!
   //! <b>Throws</b>: If allocator_type's default constructor or copy constructor
   //!   throws or T's constructor taking an dereferenced InIt throws.
   //!
   //! <b>Complexity</b>: Linear to the range [first, last).
   template <class InpIt>
   list(InpIt first, InpIt last, const A &a = A())
      : AllocHolder(a)
   {  this->insert(this->cbegin(), first, last);  }

   //! <b>Effects</b>: Destroys the list. All stored values are destroyed
   //!   and used memory is deallocated.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements.
   ~list()
   {} //AllocHolder clears the list

   //! <b>Effects</b>: Returns a copy of the internal allocator.
   //! 
   //! <b>Throws</b>: If allocator's copy constructor throws.
   //! 
   //! <b>Complexity</b>: Constant.
   allocator_type get_allocator() const
   {  return allocator_type(this->node_alloc()); }

   const stored_allocator_type &get_stored_allocator() const 
   {  return this->node_alloc(); }

   stored_allocator_type &get_stored_allocator()
   {  return this->node_alloc(); }

   //! <b>Effects</b>: Erases all the elements of the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in the list.
   void clear()
   {  AllocHolder::clear(alloc_version());  }

   //! <b>Effects</b>: Returns an iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator begin()
   { return iterator(this->icont().begin()); }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator begin() const
   {  return this->cbegin();   }

   //! <b>Effects</b>: Returns an iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   iterator end()
   {  return iterator(this->icont().end());  }

   //! <b>Effects</b>: Returns a const_iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator end() const
   {  return this->cend();  }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the beginning 
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reverse_iterator rbegin()
   {  return reverse_iterator(end());  }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning 
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rbegin() const
   {  return this->crbegin();  }

   //! <b>Effects</b>: Returns a reverse_iterator pointing to the end
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reverse_iterator rend()
   {  return reverse_iterator(begin());   }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator rend() const
   {  return this->crend();   }

   //! <b>Effects</b>: Returns a const_iterator to the first element contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator cbegin() const
   {  return const_iterator(this->non_const_icont().begin());   }

   //! <b>Effects</b>: Returns a const_iterator to the end of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_iterator cend() const
   {  return const_iterator(this->non_const_icont().end());  }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the beginning 
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crbegin() const
   {  return const_reverse_iterator(this->cend());  }

   //! <b>Effects</b>: Returns a const_reverse_iterator pointing to the end
   //! of the reversed list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reverse_iterator crend() const
   {  return const_reverse_iterator(this->cbegin());   }

   //! <b>Effects</b>: Returns true if the list contains no elements.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   bool empty() const 
   {  return !this->size();  }

   //! <b>Effects</b>: Returns the number of the elements contained in the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type size() const 
   {   return this->icont().size();   }

   //! <b>Effects</b>: Returns the largest possible size of the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   size_type max_size() const 
   {  return AllocHolder::max_size();  }

   //! <b>Effects</b>: Inserts a copy of t in the beginning of the list.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_front(insert_const_ref_type x)   
   {  this->insert(this->cbegin(), x);  }

   #if defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void push_front(T &x) { push_front(const_cast<const T &>(x)); }

   template<class U>
   void push_front(const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::BOOST_CONTAINER_MOVE_NAMESPACE::is_movable<U>::value >::type* =0)
   {  this->insert(this->cbegin(), u);  }
   #endif

   //! <b>Effects</b>: Constructs a new element in the beginning of the list
   //!   and moves the resources of t to this new element.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_front(BOOST_MOVE_MACRO_RV_REF(T) x)
   {  this->insert(this->cbegin(), BOOST_CONTAINER_MOVE_NAMESPACE::move(x));  }

   //! <b>Effects</b>: Removes the last element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back (insert_const_ref_type x)   
   {  this->insert(this->cend(), x);    }

   #if defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void push_back(T &x) { push_back(const_cast<const T &>(x)); }

   template<class U>
   void push_back(const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::BOOST_CONTAINER_MOVE_NAMESPACE::is_movable<U>::value >::type* =0)
   {  this->insert(this->cend(), u);    }

   #endif

   //! <b>Effects</b>: Removes the first element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void push_back (BOOST_MOVE_MACRO_RV_REF(T) x)
   {  this->insert(this->cend(), BOOST_CONTAINER_MOVE_NAMESPACE::move(x));    }

   //! <b>Effects</b>: Removes the first element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void pop_front()              
   {  this->erase(this->cbegin());      }

   //! <b>Effects</b>: Removes the last element from the list.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   void pop_back()               
   {  const_iterator tmp = this->cend(); this->erase(--tmp);  }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference front()             
   { return *this->begin(); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference front() const 
   { return *this->begin(); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   reference back()              
   { return *(--this->end()); }

   //! <b>Requires</b>: !empty()
   //!
   //! <b>Effects</b>: Returns a const reference to the first element 
   //!   from the beginning of the container.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Constant.
   const_reference back()  const 
   { return *(--this->end()); }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are copy constructed from x.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size, const T& x)
   {
      const_iterator iend = this->cend();
      size_type len = this->size();
      
      if(len > new_size){
         size_type to_erase = len - new_size;
         while(to_erase--){
            --iend;
         }
         this->erase(iend, this->cend());
      }
      else{
         this->priv_create_and_insert_nodes(iend, new_size - len, x);
      }
   }

   //! <b>Effects</b>: Inserts or erases elements at the end such that
   //!   the size becomes n. New elements are default constructed.
   //!
   //! <b>Throws</b>: If memory allocation throws, or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the difference between size() and new_size.
   void resize(size_type new_size)
   {
      const_iterator iend = this->end();
      size_type len = this->size();
      
      if(len > new_size){
         size_type to_erase = len - new_size;
         const_iterator ifirst;
         if(to_erase < len/2u){
            ifirst = iend;
            while(to_erase--){
               --ifirst;
            }
         }
         else{
            ifirst = this->begin();
            size_type to_skip = len - to_erase;
            while(to_skip--){
               ++ifirst;
            }
         }
         this->erase(ifirst, iend);
      }
      else{
         this->priv_create_and_insert_nodes(this->cend(), new_size - len);
      }
   }

   //! <b>Effects</b>: Swaps the contents of *this and x.
   //!   If this->allocator_type() != x.allocator_type()
   //!   allocators are also swapped.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Constant.
   void swap(ThisType& x)
   {  AllocHolder::swap(x);   }

   //! <b>Effects</b>: Makes *this contain the same elements as x.
   //!
   //! <b>Postcondition</b>: this->size() == x.size(). *this contains a copy 
   //! of each of x's elements. 
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to the number of elements in x.
   ThisType& operator=(BOOST_MOVE_MACRO_COPY_ASSIGN_REF(ThisType) x)
   {
      if (this != &x) {
         this->assign(x.begin(), x.end());
      }
      return *this;
   }

   //! <b>Effects</b>: Move assignment. All mx's values are transferred to *this.
   //!
   //! <b>Postcondition</b>: x.empty(). *this contains a the elements x had
   //!   before the function.
   //!
   //! <b>Throws</b>: If allocator_type's copy constructor throws.
   //!
   //! <b>Complexity</b>: Constant.
   ThisType& operator=(BOOST_MOVE_MACRO_RV_REF(ThisType) mx)
   {
      this->clear();
      this->swap(mx);
      return *this;
   }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts n copies of x before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void insert(const_iterator p, size_type n, const T& x)
   { this->priv_create_and_insert_nodes(p, n, x); }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of the [first, last) range before p.
   //!
   //! <b>Throws</b>: If memory allocation throws, T's constructor from a
   //!   dereferenced InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to std::distance [first, last).
   template <class InpIt>
   void insert(const_iterator p, InpIt first, InpIt last) 
   {
      const bool aux_boolean = containers_detail::is_convertible<InpIt, std::size_t>::value;
      typedef containers_detail::bool_<aux_boolean> Result;
      this->priv_insert_dispatch(p, first, last, Result());
   }

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a copy of x before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or x's copy constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   iterator insert(const_iterator position, insert_const_ref_type x) 
   {  return this->priv_insert(position, x); }

   #if defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   iterator insert(const_iterator position, T &x) { return this->insert(position, const_cast<const T &>(x)); }

   template<class U>
   iterator insert(const_iterator position, const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::BOOST_CONTAINER_MOVE_NAMESPACE::is_movable<U>::value >::type* =0)
   {  return this->priv_insert(position, u); }
   #endif

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Insert a new element before p with mx's resources.
   //!
   //! <b>Throws</b>: If memory allocation throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   iterator insert(const_iterator p, BOOST_MOVE_MACRO_RV_REF(T) x) 
   {
      NodePtr tmp = AllocHolder::create_node(BOOST_CONTAINER_MOVE_NAMESPACE::move(x));
      return iterator(this->icont().insert(p.get(), *tmp));
   }

   #if defined(BOOST_CONTAINERS_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... in the end of the list.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's in-place constructor throws.
   //!
   //! <b>Complexity</b>: Constant
   template <class... Args>
   void emplace_back(Args&&... args)
   {
      this->emplace(this->cend(), BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...);
   }

   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... in the beginning of the list.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's in-place constructor throws.
   //!
   //! <b>Complexity</b>: Constant
   template <class... Args>
   void emplace_front(Args&&... args)
   {
      this->emplace(this->cbegin(), BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...);
   }

   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... before p.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's in-place constructor throws.
   //!
   //! <b>Complexity</b>: Constant
   template <class... Args>
   iterator emplace(const_iterator p, Args&&... args)
   {
      typename AllocHolder::Deallocator d(AllocHolder::create_node_and_deallocator());
      new ((void*)containers_detail::get_pointer(d.get())) Node(BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...);
      NodePtr node = d.get();
      d.release();
      return iterator(this->icont().insert(p.get(), *node));
   }

   #else //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   //0 args
   void emplace_back()
   {  this->emplace(this->cend());  }

   void emplace_front()
   {  this->emplace(this->cbegin());   }

   iterator emplace(const_iterator p)
   {
      typename AllocHolder::Deallocator d(AllocHolder::create_node_and_deallocator());
      new ((void*)containers_detail::get_pointer(d.get())) Node();
      NodePtr node = d.get();
      d.release();
      return iterator(this->icont().insert(p.get(), *node));
   }

   #define BOOST_PP_LOCAL_MACRO(n)                                                              \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                   \
   void emplace_back(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                     \
   {                                                                                            \
      this->emplace(this->cend(), BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));    \
   }                                                                                            \
                                                                                                \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                   \
   void emplace_front(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                    \
   {  this->emplace(this->cbegin(), BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));} \
                                                                                                \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                   \
   iterator emplace(const_iterator p, BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))    \
   {                                                                                            \
      typename AllocHolder::Deallocator d(AllocHolder::create_node_and_deallocator());          \
      new ((void*)containers_detail::get_pointer(d.get()))                                                 \
         Node(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));                        \
      NodePtr node = d.get();                                                                   \
      d.release();                                                                              \
      return iterator(this->icont().insert(p.get(), *node));                                    \
   }                                                                                            \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif   //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   //! <b>Requires</b>: p must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Erases the element at p p.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   iterator erase(const_iterator p) 
   {  return iterator(this->icont().erase_and_dispose(p.get(), Destroyer(this->node_alloc()))); }

   //! <b>Requires</b>: first and last must be valid iterator to elements in *this.
   //!
   //! <b>Effects</b>: Erases the elements pointed by [first, last).
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Complexity</b>: Linear to the distance between first and last.
   iterator erase(const_iterator first, const_iterator last)
   {  return iterator(AllocHolder::erase_range(first.get(), last.get(), alloc_version())); }

   //! <b>Effects</b>: Assigns the n copies of val to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or T's copy constructor throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   void assign(size_type n, const T& val) 
   {  this->priv_fill_assign(n, val);  }

   //! <b>Effects</b>: Assigns the the range [first, last) to *this.
   //!
   //! <b>Throws</b>: If memory allocation throws or
   //!   T's constructor from dereferencing InpIt throws.
   //!
   //! <b>Complexity</b>: Linear to n.
   template <class InpIt>
   void assign(InpIt first, InpIt last) 
   {
      const bool aux_boolean = containers_detail::is_convertible<InpIt, std::size_t>::value;
      typedef containers_detail::bool_<aux_boolean> Result;
      this->priv_assign_dispatch(first, last, Result());
   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by the list. x != *this
   //!
   //! <b>Effects</b>: Transfers all the elements of list x to this list, before the
   //!   the element pointed by p. No destructors or copy constructors are called.
   //!
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //!
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of
   //!    this list. Iterators of this list and all the references are not invalidated.
   void splice(iterator p, ThisType& x) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->icont().splice(p.get(), x.icont());
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. i must point to an element contained in list x.
   //! 
   //! <b>Effects</b>: Transfers the value pointed by i, from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //!   If p == i or p == ++i, this function is a null operation. 
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(const_iterator p, ThisType &x, const_iterator i) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->icont().splice(p.get(), x.icont(), i.get());
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. first and last must point to elements contained in list x.
   //! 
   //! <b>Effects</b>: Transfers the range pointed by first and last from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Linear to the number of elements transferred.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(const_iterator p, ThisType &x, const_iterator first, const_iterator last) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->icont().splice(p.get(), x.icont(), first.get(), last.get());
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

   //! <b>Requires</b>: p must point to an element contained
   //!   by this list. first and last must point to elements contained in list x.
   //!   n == std::distance(first, last)
   //! 
   //! <b>Effects</b>: Transfers the range pointed by first and last from list x to this list, 
   //!   before the the element pointed by p. No destructors or copy constructors are called.
   //! 
   //! <b>Throws</b>: std::runtime_error if this' allocator and x's allocator
   //!   are not equal.
   //! 
   //! <b>Complexity</b>: Constant.
   //! 
   //! <b>Note</b>: Iterators of values obtained from list x now point to elements of this
   //!   list. Iterators of this list and all the references are not invalidated.
   void splice(const_iterator p, ThisType &x, const_iterator first, const_iterator last, size_type n) 
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->icont().splice(p.get(), x.icont(), first.get(), last.get(), n);
      }
      else{
         throw std::runtime_error("list::splice called with unequal allocators");
      }
   }

   //! <b>Effects</b>: Reverses the order of elements in the list. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time.
   //! 
   //! <b>Note</b>: Iterators and references are not invalidated
   void reverse()
   {  this->icont().reverse(); }    

   //! <b>Effects</b>: Removes all the elements that compare equal to value.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear time. It performs exactly size() comparisons for equality.
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   void remove(const T& value)
   {  remove_if(equal_to_value(value));  }

   //! <b>Effects</b>: Removes all the elements for which a specified
   //!   predicate is satisfied.
   //! 
   //! <b>Throws</b>: If pred throws.
   //! 
   //! <b>Complexity</b>: Linear time. It performs exactly size() calls to the predicate.
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template <class Pred>
   void remove_if(Pred pred)
   {
      typedef ValueCompareToNodeCompare<Pred> Predicate;
      this->icont().remove_and_dispose_if(Predicate(pred), Destroyer(this->node_alloc()));
   }

   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent 
   //!   elements that are equal from the list.
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: Linear time (size()-1 comparisons calls to pred()).
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   void unique()
   {  this->unique(value_equal());  }

   //! <b>Effects</b>: Removes adjacent duplicate elements or adjacent 
   //!   elements that satisfy some binary predicate from the list.
   //! 
   //! <b>Throws</b>: If pred throws.
   //! 
   //! <b>Complexity</b>: Linear time (size()-1 comparisons equality comparisons).
   //! 
   //! <b>Note</b>: The relative order of elements that are not removed is unchanged,
   //!   and iterators to elements that are not removed remain valid.
   template <class BinaryPredicate>
   void unique(BinaryPredicate binary_pred)
   {
      typedef ValueCompareToNodeCompare<BinaryPredicate> Predicate;
      this->icont().unique_and_dispose(Predicate(binary_pred), Destroyer(this->node_alloc()));
   }

   //! <b>Requires</b>: The lists x and *this must be distinct. 
   //!
   //! <b>Effects</b>: This function removes all of x's elements and inserts them
   //!   in order into *this according to std::less<value_type>. The merge is stable; 
   //!   that is, if an element from *this is equivalent to one from x, then the element 
   //!   from *this will precede the one from x. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   void merge(list<T, A>& x)
   {  this->merge(x, value_less());  }

   //! <b>Requires</b>: p must be a comparison function that induces a strict weak
   //!   ordering and both *this and x must be sorted according to that ordering
   //!   The lists x and *this must be distinct. 
   //! 
   //! <b>Effects</b>: This function removes all of x's elements and inserts them
   //!   in order into *this. The merge is stable; that is, if an element from *this is 
   //!   equivalent to one from x, then the element from *this will precede the one from x. 
   //! 
   //! <b>Throws</b>: Nothing.
   //! 
   //! <b>Complexity</b>: This function is linear time: it performs at most
   //!   size() + x.size() - 1 comparisons.
   //! 
   //! <b>Note</b>: Iterators and references to *this are not invalidated.
   template <class StrictWeakOrdering>
   void merge(list &x, StrictWeakOrdering comp)
   {
      if((NodeAlloc&)*this == (NodeAlloc&)x){
         this->icont().merge(x.icont(),
            ValueCompareToNodeCompare<StrictWeakOrdering>(comp));
      }
      else{
         throw std::runtime_error("list::merge called with unequal allocators");
      }
   }

   //! <b>Effects</b>: This function sorts the list *this according to std::less<value_type>. 
   //!   The sort is stable, that is, the relative order of equivalent elements is preserved.
   //!
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Notes</b>: Iterators and references are not invalidated.
   //!   
   //! <b>Complexity</b>: The number of comparisons is approximately N log N, where N
   //!   is the list's size.
   void sort()
   {  this->sort(value_less());  }

   //! <b>Effects</b>: This function sorts the list *this according to std::less<value_type>. 
   //!   The sort is stable, that is, the relative order of equivalent elements is preserved.
   //! 
   //! <b>Throws</b>: Nothing.
   //!
   //! <b>Notes</b>: Iterators and references are not invalidated.
   //! 
   //! <b>Complexity</b>: The number of comparisons is approximately N log N, where N
   //!   is the list's size.
   template <class StrictWeakOrdering>
   void sort(StrictWeakOrdering comp)
   {
      // nothing if the list has length 0 or 1.
      if (this->size() < 2)
         return;
      this->icont().sort(ValueCompareToNodeCompare<StrictWeakOrdering>(comp));
   }

   /// @cond
   private:

   iterator priv_insert(const_iterator p, const T &x) 
   {
      NodePtr tmp = AllocHolder::create_node(x);
      return iterator(this->icont().insert(p.get(), *tmp));
   }

   //Iterator range version
   template<class InpIterator>
   void priv_create_and_insert_nodes
      (const_iterator pos, InpIterator beg, InpIterator end)
   {
      typedef typename std::iterator_traits<InpIterator>::iterator_category ItCat;
      priv_create_and_insert_nodes(pos, beg, end, alloc_version(), ItCat());
   }

   template<class InpIterator>
   void priv_create_and_insert_nodes
      (const_iterator pos, InpIterator beg, InpIterator end, allocator_v1, std::input_iterator_tag)
   {
      for (; beg != end; ++beg){
         this->icont().insert(pos.get(), *this->create_node_from_it(beg));
      }
   }

   template<class InpIterator>
   void priv_create_and_insert_nodes
      (const_iterator pos, InpIterator beg, InpIterator end, allocator_v2, std::input_iterator_tag)
   {  //Just forward to the default one
      priv_create_and_insert_nodes(pos, beg, end, allocator_v1(), std::input_iterator_tag());
   }

   class insertion_functor;
   friend class insertion_functor;

   class insertion_functor
   {
      Icont &icont_;
      typename Icont::const_iterator pos_;

      public:
      insertion_functor(Icont &icont, typename Icont::const_iterator pos)
         :  icont_(icont), pos_(pos)
      {}

      void operator()(Node &n)
      {  this->icont_.insert(pos_, n); }
   };


   template<class FwdIterator>
   void priv_create_and_insert_nodes
      (const_iterator pos, FwdIterator beg, FwdIterator end, allocator_v2, std::forward_iterator_tag)
   {
      if(beg != end){
         //Optimized allocation and construction
         this->allocate_many_and_construct
            (beg, std::distance(beg, end), insertion_functor(this->icont(), pos.get()));
      }
   }

   //Default constructed version
   void priv_create_and_insert_nodes(const_iterator pos, size_type n)
   {
      typedef default_construct_iterator<value_type, difference_type> default_iterator;
      this->priv_create_and_insert_nodes(pos, default_iterator(n), default_iterator());
   }

   //Copy constructed version
   void priv_create_and_insert_nodes(const_iterator pos, size_type n, const T& x)
   {
      typedef constant_iterator<value_type, difference_type> cvalue_iterator;
      this->priv_create_and_insert_nodes(pos, cvalue_iterator(x, n), cvalue_iterator());
   }

   //Dispatch to detect iterator range or integer overloads
   template <class InputIter>
   void priv_insert_dispatch(const_iterator p,
                             InputIter first, InputIter last,
                             containers_detail::false_)
   {  this->priv_create_and_insert_nodes(p, first, last);   }

   template<class Integer>
   void priv_insert_dispatch(const_iterator p, Integer n, Integer x, containers_detail::true_) 
   {  this->insert(p, (size_type)n, x);  }

   void priv_fill_assign(size_type n, const T& val) 
   {
      iterator i = this->begin(), iend = this->end();

      for ( ; i != iend && n > 0; ++i, --n)
         *i = val;
      if (n > 0){
         this->priv_create_and_insert_nodes(this->cend(), n, val);
      }
      else{
         this->erase(i, cend());
      }
   }

   template <class Integer>
   void priv_assign_dispatch(Integer n, Integer val, containers_detail::true_)
   {  this->priv_fill_assign((size_type) n, (T) val); }

   template <class InputIter>
   void priv_assign_dispatch(InputIter first2, InputIter last2, containers_detail::false_)
   {
      iterator first1   = this->begin();
      iterator last1    = this->end();
      for ( ; first1 != last1 && first2 != last2; ++first1, ++first2)
         *first1 = *first2;
      if (first2 == last2)
         this->erase(first1, last1);
      else{
         this->priv_create_and_insert_nodes(last1, first2, last2);
      }
   }

   //Functors for member algorithm defaults
   struct value_less
   {
      bool operator()(const value_type &a, const value_type &b) const
         {  return a < b;  }
   };

   struct value_equal
   {
      bool operator()(const value_type &a, const value_type &b) const
         {  return a == b;  }
   };
   /// @endcond

};

template <class T, class A>
inline bool operator==(const list<T,A>& x, const list<T,A>& y)
{
   if(x.size() != y.size()){
      return false;
   }
   typedef typename list<T,A>::const_iterator const_iterator;
   const_iterator end1 = x.end();

   const_iterator i1 = x.begin();
   const_iterator i2 = y.begin();
   while (i1 != end1 && *i1 == *i2) {
      ++i1;
      ++i2;
   }
   return i1 == end1;
}

template <class T, class A>
inline bool operator<(const list<T,A>& x,
                      const list<T,A>& y)
{
  return std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end());
}

template <class T, class A>
inline bool operator!=(const list<T,A>& x, const list<T,A>& y) 
{
  return !(x == y);
}

template <class T, class A>
inline bool operator>(const list<T,A>& x, const list<T,A>& y) 
{
  return y < x;
}

template <class T, class A>
inline bool operator<=(const list<T,A>& x, const list<T,A>& y) 
{
  return !(y < x);
}

template <class T, class A>
inline bool operator>=(const list<T,A>& x, const list<T,A>& y) 
{
  return !(x < y);
}

template <class T, class A>
inline void swap(list<T, A>& x, list<T, A>& y)
{
  x.swap(y);
}

/// @cond

}  //namespace container {
/*
//!has_trivial_destructor_after_move<> == true_type
//!specialization for optimizations
template <class T, class A>
struct has_trivial_destructor_after_move<boost::container::list<T, A> >
{
   static const bool value = has_trivial_destructor<A>::value;
};
*/
namespace container {

/// @endcond

}}

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif // BOOST_CONTAINERS_LIST_HPP_
