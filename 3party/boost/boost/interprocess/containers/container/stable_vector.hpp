//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2008-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////
/* Stable vector.
 *
 * Copyright 2008 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 */

#ifndef BOOST_CONTAINER_STABLE_VECTOR_HPP
#define BOOST_CONTAINER_STABLE_VECTOR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include "detail/config_begin.hpp"
#include INCLUDE_BOOST_CONTAINER_DETAIL_WORKAROUND_HPP
#include INCLUDE_BOOST_CONTAINER_CONTAINER_FWD_HPP
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/noncopyable.hpp>
#include <boost/type_traits/is_integral.hpp>
#include INCLUDE_BOOST_CONTAINER_DETAIL_VERSION_TYPE_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_MULTIALLOCATION_CHAIN_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_UTILITIES_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_ITERATORS_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_ALGORITHMS_HPP
#include <boost/pointer_to_other.hpp>
#include <boost/get_pointer.hpp>

#include <algorithm>
#include <stdexcept>
#include <memory>

///@cond

#define STABLE_VECTOR_USE_CONTAINERS_VECTOR

#if defined (STABLE_VECTOR_USE_CONTAINERS_VECTOR)
#include INCLUDE_BOOST_CONTAINER_VECTOR_HPP
#else
#include <vector>
#endif   //STABLE_VECTOR_USE_CONTAINERS_VECTOR

//#define STABLE_VECTOR_ENABLE_INVARIANT_CHECKING

#if defined(STABLE_VECTOR_ENABLE_INVARIANT_CHECKING)
#include <boost/assert.hpp>
#endif

///@endcond

namespace boost {
namespace container {

///@cond

namespace stable_vector_detail{

template<class SmartPtr>
struct smart_ptr_type
{
   typedef typename SmartPtr::value_type value_type;
   typedef value_type *pointer;
   static pointer get (const SmartPtr &smartptr)
   {  return smartptr.get();}
};

template<class T>
struct smart_ptr_type<T*>
{
   typedef T value_type;
   typedef value_type *pointer;
   static pointer get (pointer ptr)
   {  return ptr;}
};

template<class Ptr>
inline typename smart_ptr_type<Ptr>::pointer get_pointer(const Ptr &ptr)
{  return smart_ptr_type<Ptr>::get(ptr);   }

template <class C>
class clear_on_destroy
{
   public:
   clear_on_destroy(C &c)
      :  c_(c), do_clear_(true)
   {}

   void release()
   {  do_clear_ = false; }

   ~clear_on_destroy()
   {
      if(do_clear_){
         c_.clear();
         c_.clear_pool();  
      }
   }

   private:
   clear_on_destroy(const clear_on_destroy &);
   clear_on_destroy &operator=(const clear_on_destroy &);
   C &c_;
   bool do_clear_;
};

template<class VoidPtr>
struct node_type_base
{/*
   node_type_base(VoidPtr p)
      : up(p)
   {}*/
   node_type_base()
   {}
   void set_pointer(VoidPtr p)
   {  up = p; }

   VoidPtr up;
};

template<typename VoidPointer, typename T>
struct node_type
   : public node_type_base<VoidPointer>
{
   #if defined(BOOST_CONTAINERS_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   node_type()
      : value()
   {}

   template<class ...Args>
   node_type(Args &&...args)
      : value(BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...)
   {}

   #else //BOOST_CONTAINERS_PERFECT_FORWARDING

   node_type()
      : value()
   {}

   #define BOOST_PP_LOCAL_MACRO(n)                                      \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                           \
   node_type(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))       \
      : value(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _))   \
   {}                                                                   \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif//BOOST_CONTAINERS_PERFECT_FORWARDING
   
   void set_pointer(VoidPointer p)
   {  node_type_base<VoidPointer>::set_pointer(p); }

   T value;
};

template<typename T, typename Reference, typename Pointer>
class iterator
   : public std::iterator< std::random_access_iterator_tag
                         , typename std::iterator_traits<Pointer>::value_type
                         , typename std::iterator_traits<Pointer>::difference_type
                         , Pointer
                         , Reference>
{

   typedef typename boost::pointer_to_other
      <Pointer, void>::type                  void_ptr;
   typedef node_type<void_ptr, T>            node_type_t;
   typedef typename boost::pointer_to_other
      <void_ptr, node_type_t>::type          node_type_ptr_t;
   typedef typename boost::pointer_to_other
      <void_ptr, void_ptr>::type             void_ptr_ptr;

   friend class iterator<T, const T, typename boost::pointer_to_other<Pointer, T>::type>;

   public:
   typedef std::random_access_iterator_tag   iterator_category;
   typedef T                                 value_type;
   typedef typename std::iterator_traits
      <Pointer>::difference_type             difference_type;
   typedef Pointer                           pointer;
   typedef Reference                         reference;

   iterator()
   {}

   explicit iterator(node_type_ptr_t pn)
      : pn(pn)
   {}

   iterator(const iterator<T, T&, typename boost::pointer_to_other<Pointer, T>::type >& x)
      : pn(x.pn)
   {}
   
   private:
   static node_type_ptr_t node_ptr_cast(void_ptr p)
   {
      using boost::get_pointer;
      return node_type_ptr_t(static_cast<node_type_t*>(stable_vector_detail::get_pointer(p)));
   }

   static void_ptr_ptr void_ptr_ptr_cast(void_ptr p)
   {
      using boost::get_pointer;
      return void_ptr_ptr(static_cast<void_ptr*>(stable_vector_detail::get_pointer(p)));
   }

   reference dereference() const
   {  return pn->value; }
   bool equal(const iterator& x) const
   {  return pn==x.pn;  }
   void increment()
   {  pn = node_ptr_cast(*(void_ptr_ptr_cast(pn->up)+1)); }
   void decrement()
   {  pn = node_ptr_cast(*(void_ptr_ptr_cast(pn->up)-1)); }
   void advance(std::ptrdiff_t n)
   {  pn = node_ptr_cast(*(void_ptr_ptr_cast(pn->up)+n)); }
   std::ptrdiff_t distance_to(const iterator& x)const
   {  return void_ptr_ptr_cast(x.pn->up) - void_ptr_ptr_cast(pn->up);   }

   public:
   //Pointer like operators
   reference operator*()  const {  return  this->dereference();  }
   pointer   operator->() const {  return  pointer(&this->dereference());  }

   //Increment / Decrement
   iterator& operator++()  
   {  this->increment(); return *this;  }

   iterator operator++(int)
   {  iterator tmp(*this);  ++*this; return iterator(tmp); }

   iterator& operator--()
   {  this->decrement(); return *this;  }

   iterator operator--(int)
   {  iterator tmp(*this);  --*this; return iterator(tmp);  }

   reference operator[](difference_type off) const
   {
      iterator tmp(*this);
      tmp += off;
      return *tmp;
   }

   iterator& operator+=(difference_type off)
   {
      pn = node_ptr_cast(*(void_ptr_ptr_cast(pn->up)+off));
      return *this;
   }

   iterator operator+(difference_type off) const
   {
      iterator tmp(*this);
      tmp += off;
      return tmp;
   }

   friend iterator operator+(difference_type off, const iterator& right)
   {
      iterator tmp(right);
      tmp += off;
      return tmp;
   }

   iterator& operator-=(difference_type off)
   {  *this += -off; return *this;   }

   iterator operator-(difference_type off) const
   {
      iterator tmp(*this);
      tmp -= off;
      return tmp;
   }

   difference_type operator-(const iterator& right) const
   {
      void_ptr_ptr p1 = void_ptr_ptr_cast(this->pn->up);
      void_ptr_ptr p2 = void_ptr_ptr_cast(right.pn->up);
      return p1 - p2;
   }

   //Comparison operators
   bool operator==   (const iterator& r)  const
   {  return pn == r.pn;  }

   bool operator!=   (const iterator& r)  const
   {  return pn != r.pn;  }

   bool operator<    (const iterator& r)  const
   {  return void_ptr_ptr_cast(pn->up) < void_ptr_ptr_cast(r.pn->up);  }

   bool operator<=   (const iterator& r)  const
   {  return void_ptr_ptr_cast(pn->up) <= void_ptr_ptr_cast(r.pn->up);  }

   bool operator>    (const iterator& r)  const
   {  return void_ptr_ptr_cast(pn->up) > void_ptr_ptr_cast(r.pn->up);  }

   bool operator>=   (const iterator& r)  const
   {  return void_ptr_ptr_cast(pn->up) >= void_ptr_ptr_cast(r.pn->up);  }

   node_type_ptr_t pn;
};

template<class Allocator, unsigned int Version>
struct select_multiallocation_chain
{
   typedef typename Allocator::multiallocation_chain type;
};

template<class Allocator>
struct select_multiallocation_chain<Allocator, 1>
{
   typedef typename Allocator::template
      rebind<void>::other::pointer                          void_ptr;
   typedef containers_detail::basic_multiallocation_chain
      <void_ptr>                                            multialloc_cached_counted;
   typedef boost::container::containers_detail::transform_multiallocation_chain
      <multialloc_cached_counted, typename Allocator::value_type>   type;
};

} //namespace stable_vector_detail

#if defined(STABLE_VECTOR_ENABLE_INVARIANT_CHECKING)

#define STABLE_VECTOR_CHECK_INVARIANT \
invariant_checker BOOST_JOIN(check_invariant_,__LINE__)(*this); \
BOOST_JOIN(check_invariant_,__LINE__).touch();
#else

#define STABLE_VECTOR_CHECK_INVARIANT

#endif   //#if defined(STABLE_VECTOR_ENABLE_INVARIANT_CHECKING)

/// @endcond

//!Help taken from (<a href="http://bannalia.blogspot.com/2008/09/introducing-stablevector.html" > Introducing stable_vector</a>)
//!
//!We present stable_vector, a fully STL-compliant stable container that provides
//!most of the features of std::vector except element contiguity. 
//!
//!General properties: stable_vector satisfies all the requirements of a container,
//!a reversible container and a sequence and provides all the optional operations
//!present in std::vector. Like std::vector,  iterators are random access.
//!stable_vector does not provide element contiguity; in exchange for this absence,
//!the container is stable, i.e. references and iterators to an element of a stable_vector
//!remain valid as long as the element is not erased, and an iterator that has been
//!assigned the return value of end() always remain valid until the destruction of
//!the associated  stable_vector.
//!
//!Operation complexity: The big-O complexities of stable_vector operations match
//!exactly those of std::vector. In general, insertion/deletion is constant time at
//!the end of the sequence and linear elsewhere. Unlike std::vector, stable_vector
//!does not internally perform any value_type destruction, copy or assignment
//!operations other than those exactly corresponding to the insertion of new
//!elements or deletion of stored elements, which can sometimes compensate in terms
//!of performance for the extra burden of doing more pointer manipulation and an
//!additional allocation per element.
//!
//!Exception safety: As stable_vector does not internally copy elements around, some
//!operations provide stronger exception safety guarantees than in std::vector:
template<typename T, typename Allocator>
class stable_vector
{
   ///@cond
   typedef typename containers_detail::
      move_const_ref_type<T>::type insert_const_ref_type;
   typedef typename Allocator::template
      rebind<void>::other::pointer                    void_ptr;
   typedef typename Allocator::template
      rebind<void_ptr>::other::pointer                void_ptr_ptr;
   typedef stable_vector_detail::node_type
      <void_ptr, T>                                   node_type_t;
   typedef typename Allocator::template
      rebind<node_type_t>::other::pointer             node_type_ptr_t;
   typedef stable_vector_detail::node_type_base
      <void_ptr>                                      node_type_base_t;
   typedef typename Allocator::template
      rebind<node_type_base_t>::other::pointer        node_type_base_ptr_t;
   typedef 
   #if defined (STABLE_VECTOR_USE_CONTAINERS_VECTOR)
   ::boost::container::
   #else
   ::std::
   #endif   //STABLE_VECTOR_USE_CONTAINERS_VECTOR
   vector<void_ptr,
      typename Allocator::
      template rebind<void_ptr>::other
   >                                                  impl_type;
   typedef typename impl_type::iterator               impl_iterator;
   typedef typename impl_type::const_iterator         const_impl_iterator;

   typedef ::boost::container::containers_detail::
      integral_constant<unsigned, 1>                  allocator_v1;
   typedef ::boost::container::containers_detail::
      integral_constant<unsigned, 2>                  allocator_v2;
   typedef ::boost::container::containers_detail::integral_constant 
      <unsigned, boost::container::containers_detail::
      version<Allocator>::value>                      alloc_version;
   typedef typename Allocator::
      template rebind<node_type_t>::other             node_allocator_type;

   node_type_ptr_t allocate_one()
   {  return this->allocate_one(alloc_version());   }

   node_type_ptr_t allocate_one(allocator_v1)
   {  return get_al().allocate(1);   }

   node_type_ptr_t allocate_one(allocator_v2)
   {  return get_al().allocate_one();   }

   void deallocate_one(node_type_ptr_t p)
   {  return this->deallocate_one(p, alloc_version());   }

   void deallocate_one(node_type_ptr_t p, allocator_v1)
   {  get_al().deallocate(p, 1);   }

   void deallocate_one(node_type_ptr_t p, allocator_v2)
   {  get_al().deallocate_one(p);   }

   friend class stable_vector_detail::clear_on_destroy<stable_vector>;
   ///@endcond
   public:


   // types:

   typedef typename Allocator::reference              reference;
   typedef typename Allocator::const_reference        const_reference;
   typedef typename Allocator::pointer                pointer;
   typedef typename Allocator::const_pointer          const_pointer;
   typedef stable_vector_detail::iterator
      <T,T&, pointer>                                 iterator;
   typedef stable_vector_detail::iterator
      <T,const T&, const_pointer>                     const_iterator;
   typedef typename impl_type::size_type              size_type;
   typedef typename iterator::difference_type         difference_type;
   typedef T                                          value_type;
   typedef Allocator                                  allocator_type;
   typedef std::reverse_iterator<iterator>            reverse_iterator;
   typedef std::reverse_iterator<const_iterator>      const_reverse_iterator;

   ///@cond
   private:
   BOOST_MOVE_MACRO_COPYABLE_AND_MOVABLE(stable_vector)
   static const size_type ExtraPointers = 3;
   typedef typename stable_vector_detail::
      select_multiallocation_chain
      < node_allocator_type
      , alloc_version::value
      >::type                                         multiallocation_chain;
   ///@endcond
   public:

   // construct/copy/destroy:
   explicit stable_vector(const Allocator& al=Allocator())
   : internal_data(al),impl(al)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
   }

   explicit stable_vector(size_type n)
   : internal_data(Allocator()),impl(Allocator())
   {
      stable_vector_detail::clear_on_destroy<stable_vector> cod(*this);
      this->resize(n);
      STABLE_VECTOR_CHECK_INVARIANT;
      cod.release();
   }

   stable_vector(size_type n, const T& t, const Allocator& al=Allocator())
   : internal_data(al),impl(al)
   {
      stable_vector_detail::clear_on_destroy<stable_vector> cod(*this);
      this->insert(this->cbegin(), n, t);
      STABLE_VECTOR_CHECK_INVARIANT;
      cod.release();
   }

   template <class InputIterator>
   stable_vector(InputIterator first,InputIterator last,const Allocator& al=Allocator())
      : internal_data(al),impl(al)
   {
      stable_vector_detail::clear_on_destroy<stable_vector> cod(*this);
      this->insert(this->cbegin(), first, last);
      STABLE_VECTOR_CHECK_INVARIANT;
      cod.release();
   }

   stable_vector(const stable_vector& x)
      : internal_data(x.get_al()),impl(x.get_al())
   {
      stable_vector_detail::clear_on_destroy<stable_vector> cod(*this);
      this->insert(this->cbegin(), x.begin(), x.end());
      STABLE_VECTOR_CHECK_INVARIANT;
      cod.release();
   }

   stable_vector(BOOST_MOVE_MACRO_RV_REF(stable_vector) x) 
      : internal_data(x.get_al()),impl(x.get_al())
   {  this->swap(x);   }

   ~stable_vector()
   {
      this->clear();
      clear_pool();  
   }

   stable_vector& operator=(BOOST_MOVE_MACRO_COPY_ASSIGN_REF(stable_vector) x)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      if (this != &x) {
         this->assign(x.begin(), x.end());
      }
      return *this;
   }

   stable_vector& operator=(BOOST_MOVE_MACRO_RV_REF(stable_vector) x)
   {
      if (&x != this){
         this->swap(x);
         x.clear();
      }
      return *this;
   }

   template<typename InputIterator>
   void assign(InputIterator first,InputIterator last)
   {
      assign_dispatch(first, last, boost::is_integral<InputIterator>());
   }

   void assign(size_type n,const T& t)
   {
      typedef constant_iterator<value_type, difference_type> cvalue_iterator;
      return assign_dispatch(cvalue_iterator(t, n), cvalue_iterator(), boost::mpl::false_());
   }

   allocator_type get_allocator()const  {return get_al();}

   // iterators:

   iterator  begin()
   {   return (impl.empty()) ? end(): iterator(node_ptr_cast(impl.front())) ;   }

   const_iterator  begin()const
   {   return (impl.empty()) ? cend() : const_iterator(node_ptr_cast(impl.front())) ;   }

   iterator        end()                {return iterator(get_end_node());}
   const_iterator  end()const           {return const_iterator(get_end_node());}

   reverse_iterator       rbegin()      {return reverse_iterator(this->end());}
   const_reverse_iterator rbegin()const {return const_reverse_iterator(this->end());}
   reverse_iterator       rend()        {return reverse_iterator(this->begin());}
   const_reverse_iterator rend()const   {return const_reverse_iterator(this->begin());}

   const_iterator         cbegin()const {return this->begin();}
   const_iterator         cend()const   {return this->end();}
   const_reverse_iterator crbegin()const{return this->rbegin();}
   const_reverse_iterator crend()const  {return this->rend();}

   // capacity:
   size_type size() const
   {  return impl.empty() ? 0 : (impl.size() - ExtraPointers);   }

   size_type max_size() const
   {  return impl.max_size() - ExtraPointers;  }

   size_type capacity() const
   {
      if(!impl.capacity()){
         return 0;
      }
      else{
         const size_type num_nodes = this->impl.size() + this->internal_data.pool_size;
         const size_type num_buck  = this->impl.capacity();
         return (num_nodes < num_buck) ? num_nodes : num_buck;
      }
   }

   bool empty() const
   {  return impl.empty() || impl.size() == ExtraPointers;  }

   void resize(size_type n, const T& t)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      if(n > size())
         this->insert(this->cend(), n - this->size(), t);
      else if(n < this->size())
         this->erase(this->cbegin() + n, this->cend());
   }

   void resize(size_type n)
   {
      typedef default_construct_iterator<value_type, difference_type> default_iterator;
      STABLE_VECTOR_CHECK_INVARIANT;
      if(n > size())
         this->insert(this->cend(), default_iterator(n - this->size()), default_iterator());
      else if(n < this->size())
         this->erase(this->cbegin() + n, this->cend());
   }

   void reserve(size_type n)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      if(n > this->max_size())
         throw std::bad_alloc();

      size_type size = this->size();   
      size_type old_capacity = this->capacity();
      if(n > old_capacity){
         this->initialize_end_node(n);
         const void * old_ptr = &impl[0];
         impl.reserve(n + ExtraPointers);
         bool realloced = &impl[0] != old_ptr;
         //Fix the pointers for the newly allocated buffer
         if(realloced){
            this->align_nodes(impl.begin(), impl.begin()+size+1);
         }
         //Now fill pool if data is not enough
         if((n - size) > this->internal_data.pool_size){
            this->add_to_pool((n - size) - this->internal_data.pool_size);
         }
      }
   }

   // element access:

   reference operator[](size_type n){return value(impl[n]);}
   const_reference operator[](size_type n)const{return value(impl[n]);}

   const_reference at(size_type n)const
   {
      if(n>=size())
         throw std::out_of_range("invalid subscript at stable_vector::at");
      return operator[](n);
   }

   reference at(size_type n)
   {
      if(n>=size())
         throw std::out_of_range("invalid subscript at stable_vector::at");
      return operator[](n);
   }

   reference front()
   {  return value(impl.front());   }

   const_reference front()const
   {  return value(impl.front());   }

   reference back()
   {  return value(*(&impl.back() - ExtraPointers)); }

   const_reference back()const
   {  return value(*(&impl.back() - ExtraPointers)); }

   // modifiers:

   void push_back(insert_const_ref_type x) 
   {  return priv_push_back(x);  }

   #if defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   void push_back(T &x) { push_back(const_cast<const T &>(x)); }

   template<class U>
   void push_back(const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::BOOST_CONTAINER_MOVE_NAMESPACE::is_movable<U>::value >::type* =0)
   { return priv_push_back(u); }
   #endif

   void push_back(BOOST_MOVE_MACRO_RV_REF(T) t) 
   {  this->insert(end(), BOOST_CONTAINER_MOVE_NAMESPACE::move(t));  }

   void pop_back()
   {  this->erase(this->end()-1);   }

   iterator insert(const_iterator position, insert_const_ref_type x) 
   {  return this->priv_insert(position, x); }

   #if defined(BOOST_NO_RVALUE_REFERENCES) && !defined(BOOST_MOVE_DOXYGEN_INVOKED)
   iterator insert(const_iterator position, T &x) { return this->insert(position, const_cast<const T &>(x)); }

   template<class U>
   iterator insert(const_iterator position, const U &u, typename containers_detail::enable_if_c<containers_detail::is_same<T, U>::value && !::BOOST_CONTAINER_MOVE_NAMESPACE::is_movable<U>::value >::type* =0)
   {  return this->priv_insert(position, u); }
   #endif

   iterator insert(const_iterator position, BOOST_MOVE_MACRO_RV_REF(T) x) 
   {
      typedef repeat_iterator<T, difference_type>           repeat_it;
      typedef BOOST_CONTAINER_MOVE_NAMESPACE::move_iterator<repeat_it> repeat_move_it;
      //Just call more general insert(pos, size, value) and return iterator
      size_type pos_n = position - cbegin();
      this->insert(position
         ,repeat_move_it(repeat_it(x, 1))
         ,repeat_move_it(repeat_it()));
      return iterator(this->begin() + pos_n);
   }

   void insert(const_iterator position, size_type n, const T& t)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      this->insert_not_iter(position, n, t);
   }

   template <class InputIterator>
   void insert(const_iterator position,InputIterator first, InputIterator last)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      this->insert_iter(position,first,last,
                        boost::mpl::not_<boost::is_integral<InputIterator> >());
   }

   #if defined(BOOST_CONTAINERS_PERFECT_FORWARDING) || defined(BOOST_CONTAINER_DOXYGEN_INVOKED)

   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... in the end of the vector.
   //!
   //! <b>Throws</b>: If memory allocation throws or the in-place constructor throws.
   //!
   //! <b>Complexity</b>: Amortized constant time.
   template<class ...Args>
   void emplace_back(Args &&...args)
   {
      typedef emplace_functor<node_type_t, Args...>         EmplaceFunctor;
      typedef emplace_iterator<node_type_t, EmplaceFunctor> EmplaceIterator;
      EmplaceFunctor &&ef = EmplaceFunctor(BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...);
      this->insert(this->cend(), EmplaceIterator(ef), EmplaceIterator());
   }

   //! <b>Requires</b>: position must be a valid iterator of *this.
   //!
   //! <b>Effects</b>: Inserts an object of type T constructed with
   //!   std::forward<Args>(args)... before position
   //!
   //! <b>Throws</b>: If memory allocation throws or the in-place constructor throws.
   //!
   //! <b>Complexity</b>: If position is end(), amortized constant time
   //!   Linear time otherwise.
   template<class ...Args>
   iterator emplace(const_iterator position, Args && ...args)
   {
      //Just call more general insert(pos, size, value) and return iterator
      size_type pos_n = position - cbegin();
      typedef emplace_functor<node_type_t, Args...>         EmplaceFunctor;
      typedef emplace_iterator<node_type_t, EmplaceFunctor> EmplaceIterator;
      EmplaceFunctor &&ef = EmplaceFunctor(BOOST_CONTAINER_MOVE_NAMESPACE::forward<Args>(args)...);
      this->insert(position, EmplaceIterator(ef), EmplaceIterator());
      return iterator(this->begin() + pos_n);
   }

   #else

   void emplace_back()
   {
      typedef emplace_functor<node_type_t>                   EmplaceFunctor;
      typedef emplace_iterator<node_type_t, EmplaceFunctor>  EmplaceIterator;
      EmplaceFunctor ef;
      this->insert(this->cend(), EmplaceIterator(ef), EmplaceIterator());
   }

   iterator emplace(const_iterator position)
   {
      typedef emplace_functor<node_type_t>                   EmplaceFunctor;
      typedef emplace_iterator<node_type_t, EmplaceFunctor>  EmplaceIterator;
      EmplaceFunctor ef;
      size_type pos_n = position - this->cbegin();
      this->insert(position, EmplaceIterator(ef), EmplaceIterator());
      return iterator(this->begin() + pos_n);
   }

   #define BOOST_PP_LOCAL_MACRO(n)                                                              \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                   \
   void emplace_back(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))                       \
   {                                                                                            \
      typedef BOOST_PP_CAT(BOOST_PP_CAT(emplace_functor, n), arg)                               \
         <node_type_t, BOOST_PP_ENUM_PARAMS(n, P)>           EmplaceFunctor;                    \
      typedef emplace_iterator<node_type_t, EmplaceFunctor>  EmplaceIterator;                   \
      EmplaceFunctor ef(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));                \
      this->insert(this->cend(), EmplaceIterator(ef), EmplaceIterator());                       \
   }                                                                                            \
                                                                                                \
   template<BOOST_PP_ENUM_PARAMS(n, class P)>                                                   \
   iterator emplace(const_iterator pos, BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_LIST, _))    \
   {                                                                                            \
      typedef BOOST_PP_CAT(BOOST_PP_CAT(emplace_functor, n), arg)                               \
         <node_type_t, BOOST_PP_ENUM_PARAMS(n, P)>           EmplaceFunctor;                    \
      typedef emplace_iterator<node_type_t, EmplaceFunctor>  EmplaceIterator;                   \
      EmplaceFunctor ef(BOOST_PP_ENUM(n, BOOST_CONTAINERS_PP_PARAM_FORWARD, _));                \
      size_type pos_n = pos - this->cbegin();                                                   \
      this->insert(pos, EmplaceIterator(ef), EmplaceIterator());                                \
      return iterator(this->begin() + pos_n);                                                   \
   }                                                                                            \
   //!
   #define BOOST_PP_LOCAL_LIMITS (1, BOOST_CONTAINERS_MAX_CONSTRUCTOR_PARAMETERS)
   #include BOOST_PP_LOCAL_ITERATE()

   #endif   //#ifdef BOOST_CONTAINERS_PERFECT_FORWARDING

   iterator erase(const_iterator position)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      difference_type d=position-this->cbegin();
      impl_iterator   it=impl.begin()+d;
      this->delete_node(*it);
      impl.erase(it);
      this->align_nodes(impl.begin()+d,get_last_align());
      return this->begin()+d;
   }

   iterator erase(const_iterator first, const_iterator last)
   {   return priv_erase(first, last, alloc_version());  }

   void swap(stable_vector & x)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      this->swap_impl(*this,x);
   }

   void clear()
   {   this->erase(this->cbegin(),this->cend()); }

   /// @cond
   private:

   iterator priv_insert(const_iterator position, const value_type &t)
   {
      typedef constant_iterator<value_type, difference_type> cvalue_iterator;
      return this->insert_iter(position, cvalue_iterator(t, 1), cvalue_iterator(), std::forward_iterator_tag());
   }

   void priv_push_back(const value_type &t)
   {  this->insert(end(), t);  }

   void clear_pool(allocator_v1)
   {
      if(!impl.empty() && impl.back()){
         void_ptr &p1 = *(impl.end()-2);
         void_ptr &p2 = impl.back();

         multiallocation_chain holder;
         holder.incorporate_after(holder.before_begin(), p1, p2, this->internal_data.pool_size);
         while(!holder.empty()){
            node_type_ptr_t n = holder.front();
            holder.pop_front();
            this->deallocate_one(n);
         }
         p1 = p2 = 0;
         this->internal_data.pool_size = 0;
      }
   }

   void clear_pool(allocator_v2)
   {

      if(!impl.empty() && impl.back()){
         void_ptr &p1 = *(impl.end()-2);
         void_ptr &p2 = impl.back();
         multiallocation_chain holder;
         holder.incorporate_after(holder.before_begin(), p1, p2, internal_data.pool_size);
         get_al().deallocate_individual(BOOST_CONTAINER_MOVE_NAMESPACE::move(holder));
         p1 = p2 = 0;
         this->internal_data.pool_size = 0;
      }
   }

   void clear_pool()
   {
      this->clear_pool(alloc_version());
   }

   void add_to_pool(size_type n)
   {
      this->add_to_pool(n, alloc_version());
   }

   void add_to_pool(size_type n, allocator_v1)
   {
      size_type remaining = n;
      while(remaining--){
         this->put_in_pool(this->allocate_one());
      }
   }

   void add_to_pool(size_type n, allocator_v2)
   {
      void_ptr &p1 = *(impl.end()-2);
      void_ptr &p2 = impl.back();
      multiallocation_chain holder;
      holder.incorporate_after(holder.before_begin(), p1, p2, internal_data.pool_size);
      BOOST_STATIC_ASSERT((::BOOST_CONTAINER_MOVE_NAMESPACE::is_movable<multiallocation_chain>::value == true));
      multiallocation_chain m (get_al().allocate_individual(n));
      holder.splice_after(holder.before_begin(), m, m.before_begin(), m.last(), n);
      this->internal_data.pool_size += n;
      std::pair<void_ptr, void_ptr> data(holder.extract_data());
      p1 = data.first;
      p2 = data.second;
   }

   void put_in_pool(node_type_ptr_t p)
   {
      void_ptr &p1 = *(impl.end()-2);
      void_ptr &p2 = impl.back();
      multiallocation_chain holder;
      holder.incorporate_after(holder.before_begin(), p1, p2, internal_data.pool_size);
      holder.push_front(p);
      ++this->internal_data.pool_size;
      std::pair<void_ptr, void_ptr> ret(holder.extract_data());
      p1 = ret.first;
      p2 = ret.second;
   }

   node_type_ptr_t get_from_pool()
   {
      if(!impl.back()){
         return node_type_ptr_t(0);
      }
      else{
         void_ptr &p1 = *(impl.end()-2);
         void_ptr &p2 = impl.back();
         multiallocation_chain holder;
         holder.incorporate_after(holder.before_begin(), p1, p2, internal_data.pool_size);
         node_type_ptr_t ret = holder.front();
         holder.pop_front();
         --this->internal_data.pool_size;
         if(!internal_data.pool_size){
            p1 = p2 = 0;
         }
         else{
            std::pair<void_ptr, void_ptr> data(holder.extract_data());
            p1 = data.first;
            p2 = data.second;
         }
         return ret;
      }
   }

   void insert_iter_prolog(size_type n, difference_type d)
   {
      initialize_end_node(n);
      const void* old_ptr = &impl[0];
      //size_type old_capacity = capacity();
      //size_type old_size = size();
      impl.insert(impl.begin()+d, n, 0);
      bool realloced = &impl[0] != old_ptr;
      //Fix the pointers for the newly allocated buffer
      if(realloced){
         align_nodes(impl.begin(), impl.begin()+d);
      }
   }

   template<typename InputIterator>
   void assign_dispatch(InputIterator first, InputIterator last, boost::mpl::false_)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      iterator first1   = this->begin();
      iterator last1    = this->end();
      for ( ; first1 != last1 && first != last; ++first1, ++first)
         *first1 = *first;
      if (first == last){
         this->erase(first1, last1);
      }
      else{
         this->insert(last1, first, last);
      }
   }

   template<typename Integer>
   void assign_dispatch(Integer n, Integer t, boost::mpl::true_)
   {
      typedef constant_iterator<value_type, difference_type> cvalue_iterator;
      this->assign_dispatch(cvalue_iterator(t, n), cvalue_iterator(), boost::mpl::false_());
   }

   iterator priv_erase(const_iterator first, const_iterator last, allocator_v1)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      difference_type d1 = first - this->cbegin(), d2 = last - this->cbegin();
      if(d1 != d2){
         impl_iterator it1(impl.begin() + d1), it2(impl.begin() + d2);
         for(impl_iterator it = it1; it != it2; ++it)
            this->delete_node(*it);
         impl.erase(it1, it2);
         this->align_nodes(impl.begin() + d1, get_last_align());
      }
      return iterator(this->begin() + d1);
   }

   impl_iterator get_last_align()
   {
      return impl.end() - (ExtraPointers - 1);
   }

   const_impl_iterator get_last_align() const
   {
      return impl.cend() - (ExtraPointers - 1);
   }

   template<class AllocatorVersion>
   iterator priv_erase(const_iterator first, const_iterator last, AllocatorVersion,
      typename boost::container::containers_detail::enable_if_c
         <boost::container::containers_detail::is_same<AllocatorVersion, allocator_v2>
            ::value>::type * = 0)
   {
      STABLE_VECTOR_CHECK_INVARIANT;
      return priv_erase(first, last, allocator_v1());
   }

   static node_type_ptr_t node_ptr_cast(void_ptr p)
   {
      using boost::get_pointer;
      return node_type_ptr_t(static_cast<node_type_t*>(stable_vector_detail::get_pointer(p)));
   }

   static node_type_base_ptr_t node_base_ptr_cast(void_ptr p)
   {
      using boost::get_pointer;
      return node_type_base_ptr_t(static_cast<node_type_base_t*>(stable_vector_detail::get_pointer(p)));
   }

   static value_type& value(void_ptr p)
   {
      return node_ptr_cast(p)->value;
   }

   void initialize_end_node(size_type impl_capacity = 0)
   {
      if(impl.empty()){
         impl.reserve(impl_capacity + ExtraPointers);
         impl.resize (ExtraPointers, void_ptr(0));
         impl[0] = &this->internal_data.end_node;
         this->internal_data.end_node.up = &impl[0];
      }
   }

   void readjust_end_node()
   {
      if(!this->impl.empty()){
         void_ptr &end_node_ref = *(this->get_last_align()-1);
         end_node_ref = this->get_end_node();
         this->internal_data.end_node.up = &end_node_ref;
      }
      else{
         this->internal_data.end_node.up = void_ptr(&this->internal_data.end_node.up);
      }
   }

   node_type_ptr_t get_end_node() const
   {
      const node_type_base_t* cp = &this->internal_data.end_node;
      node_type_base_t* p = const_cast<node_type_base_t*>(cp);
      return node_ptr_cast(p);
   }

   template<class Iter>
   void_ptr new_node(void_ptr up, Iter it)
   {
      node_type_ptr_t p = this->allocate_one();
      try{
         boost::container::construct_in_place(&*p, it);
         p->set_pointer(up);
      }
      catch(...){
         this->deallocate_one(p);
         throw;
      }
      return p;
   }

   void delete_node(void_ptr p)
   {
      node_type_ptr_t n(node_ptr_cast(p));
      n->~node_type_t();
      this->put_in_pool(n);
   }

   static void align_nodes(impl_iterator first,impl_iterator last)
   {
      while(first!=last){
         node_ptr_cast(*first)->up = void_ptr(&*first);
         ++first;
      }
   }

   void insert_not_iter(const_iterator position, size_type n, const T& t)
   {
      typedef constant_iterator<value_type, difference_type> cvalue_iterator;
      this->insert_iter(position, cvalue_iterator(t, n), cvalue_iterator(), std::forward_iterator_tag());
   }

   template <class InputIterator>
   void insert_iter(const_iterator position,InputIterator first,InputIterator last, boost::mpl::true_)
   {
      typedef typename std::iterator_traits<InputIterator>::iterator_category category;
      this->insert_iter(position, first, last, category());
   }

   template <class InputIterator>
   void insert_iter(const_iterator position,InputIterator first,InputIterator last,std::input_iterator_tag)
   {
      for(; first!=last; ++first){
         this->insert(position, *first);
      }    
   }

   template <class InputIterator>
   iterator insert_iter(const_iterator position, InputIterator first, InputIterator last, std::forward_iterator_tag)
   {
      size_type       n = (size_type)std::distance(first,last);
      difference_type d = position-this->cbegin();
      if(n){
         this->insert_iter_prolog(n, d);
         const impl_iterator it(impl.begin() + d);
         this->insert_iter_fwd(it, first, last, n);
         //Fix the pointers for the newly allocated buffer
         this->align_nodes(it + n, get_last_align());
      }
      return this->begin() + d;
   }

   template <class FwdIterator>
   void insert_iter_fwd_alloc(const impl_iterator it, FwdIterator first, FwdIterator last, difference_type n, allocator_v1)
   {
      size_type i=0;
      try{
         while(first!=last){
            *(it + i) = this->new_node(void_ptr((void*)(&*(it + i))), first);
            ++first;
            ++i;
         }
      }
      catch(...){
         impl.erase(it + i, it + n);
         this->align_nodes(it + i, get_last_align());
         throw;
      }
   }

   template <class FwdIterator>
   void insert_iter_fwd_alloc(const impl_iterator it, FwdIterator first, FwdIterator last, difference_type n, allocator_v2)
   {
      multiallocation_chain mem(get_al().allocate_individual(n));

      size_type i = 0;
      node_type_ptr_t p = 0;
      try{
         while(first != last){
            p = mem.front();
            mem.pop_front();
            //This can throw
            boost::container::construct_in_place(&*p, first);
            p->set_pointer(void_ptr((void*)(&*(it + i))));
            ++first;
            *(it + i) = p;
            ++i;
         }
      }
      catch(...){
         get_al().deallocate_one(p);
         get_al().deallocate_many(BOOST_CONTAINER_MOVE_NAMESPACE::move(mem));
         impl.erase(it+i, it+n);
         this->align_nodes(it+i,get_last_align());
         throw;
      }
   }

   template <class FwdIterator>
   void insert_iter_fwd(const impl_iterator it, FwdIterator first, FwdIterator last, difference_type n)
   {
      size_type i = 0;
      node_type_ptr_t p = 0;
      try{
         while(first != last){
            p = get_from_pool();
            if(!p){
               insert_iter_fwd_alloc(it+i, first, last, n-i, alloc_version());
               break;
            }
            //This can throw
            boost::container::construct_in_place(&*p, first);
            p->set_pointer(void_ptr(&*(it+i)));
            ++first;
            *(it+i)=p;
            ++i;
         }
      }
      catch(...){
         put_in_pool(p);
         impl.erase(it+i,it+n);
         this->align_nodes(it+i,get_last_align());
         throw;
      }
   }

   template <class InputIterator>
   void insert_iter(const_iterator position, InputIterator first, InputIterator last, boost::mpl::false_)
   {
      this->insert_not_iter(position, first, last);
   }

   static void swap_impl(stable_vector& x,stable_vector& y)
   {
      using std::swap;
      swap(x.get_al(),y.get_al());
      swap(x.impl,y.impl);
      swap(x.internal_data.pool_size, y.internal_data.pool_size);
      x.readjust_end_node();
      y.readjust_end_node();
   }

   #if defined(STABLE_VECTOR_ENABLE_INVARIANT_CHECKING)
   bool invariant()const
   {
      if(impl.empty())
         return !capacity() && !size();
      if(get_end_node() != *(impl.end() - ExtraPointers)){
         return false;
      }
      for(const_impl_iterator it=impl.begin(),it_end=get_last_align();it!=it_end;++it){
         if(node_ptr_cast(*it)->up != &*it)
         return false;
      }
      size_type n = capacity()-size();
      const void_ptr &pool_head = impl.back();
      size_type num_pool = 0;
      node_type_ptr_t p = node_ptr_cast(pool_head);
      while(p){
         ++num_pool;
         p = p->up;
      }
      return n >= num_pool;
   }

   class invariant_checker:private boost::noncopyable
   {
      const stable_vector* p;
      public:
      invariant_checker(const stable_vector& v):p(&v){}
      ~invariant_checker(){BOOST_ASSERT(p->invariant());}
      void touch(){}
   };
   #endif

   struct ebo_holder
      : node_allocator_type
   {
      ebo_holder(const allocator_type &a)
         : node_allocator_type(a), pool_size(0), end_node()
      {
         end_node.set_pointer(void_ptr(&end_node.up));
      }
      size_type pool_size;
      node_type_base_t end_node;
   } internal_data;

   node_allocator_type &get_al()              { return internal_data;  }
   const node_allocator_type &get_al() const  { return internal_data;  }

   impl_type                           impl;
   /// @endcond
};

template <typename T,typename Allocator>
bool operator==(const stable_vector<T,Allocator>& x,const stable_vector<T,Allocator>& y)
{
   return x.size()==y.size()&&std::equal(x.begin(),x.end(),y.begin());
}

template <typename T,typename Allocator>
bool operator< (const stable_vector<T,Allocator>& x,const stable_vector<T,Allocator>& y)
{
   return std::lexicographical_compare(x.begin(),x.end(),y.begin(),y.end());
}

template <typename T,typename Allocator>
bool operator!=(const stable_vector<T,Allocator>& x,const stable_vector<T,Allocator>& y)
{
   return !(x==y);
}

template <typename T,typename Allocator>
bool operator> (const stable_vector<T,Allocator>& x,const stable_vector<T,Allocator>& y)
{
   return y<x;
}

template <typename T,typename Allocator>
bool operator>=(const stable_vector<T,Allocator>& x,const stable_vector<T,Allocator>& y)
{
   return !(x<y);
}

template <typename T,typename Allocator>
bool operator<=(const stable_vector<T,Allocator>& x,const stable_vector<T,Allocator>& y)
{
   return !(x>y);
}

// specialized algorithms:

template <typename T, typename Allocator>
void swap(stable_vector<T,Allocator>& x,stable_vector<T,Allocator>& y)
{
   x.swap(y);
}

/// @cond

#undef STABLE_VECTOR_CHECK_INVARIANT

/// @endcond

}}

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif   //BOOST_CONTAINER_STABLE_VECTOR_HPP
