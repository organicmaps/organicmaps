//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINERS_DETAIL_MULTIALLOCATION_CHAIN_HPP
#define BOOST_CONTAINERS_DETAIL_MULTIALLOCATION_CHAIN_HPP

#include "config_begin.hpp"
#include INCLUDE_BOOST_CONTAINER_CONTAINER_FWD_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_UTILITIES_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_TYPE_TRAITS_HPP
#include INCLUDE_BOOST_CONTAINER_DETAIL_TRANSFORM_ITERATOR_HPP
#include <boost/intrusive/slist.hpp>
#include <boost/pointer_to_other.hpp>
#include INCLUDE_BOOST_CONTAINER_MOVE_HPP

namespace boost {
namespace container {
namespace containers_detail {

template<class VoidPointer>
class basic_multiallocation_chain
{
   private:
   typedef bi::slist_base_hook<bi::void_pointer<VoidPointer>
                        ,bi::link_mode<bi::normal_link>
                        > node;

   typedef bi::slist< node
                    , bi::linear<true>
                    , bi::cache_last<true>
                    > slist_impl_t;
   slist_impl_t slist_impl_;

   static node & to_node(VoidPointer p)
   {  return *static_cast<node*>(static_cast<void*>(containers_detail::get_pointer(p))); }

   BOOST_MOVE_MACRO_MOVABLE_BUT_NOT_COPYABLE(basic_multiallocation_chain)

   public:


   typedef VoidPointer  void_pointer;
   typedef typename slist_impl_t::iterator iterator;

   basic_multiallocation_chain()
      :  slist_impl_()
   {}

   basic_multiallocation_chain(BOOST_MOVE_MACRO_RV_REF(basic_multiallocation_chain) other)
      :  slist_impl_()
   {  slist_impl_.swap(other.slist_impl_); }

   basic_multiallocation_chain& operator=(BOOST_MOVE_MACRO_RV_REF(basic_multiallocation_chain) other)
   {
      basic_multiallocation_chain tmp(BOOST_CONTAINER_MOVE_NAMESPACE::move(other));
      this->swap(tmp);
      return *this;
   }

   bool empty() const
   {  return slist_impl_.empty(); }

   std::size_t size() const
   {  return slist_impl_.size();  }

   iterator before_begin()
   {  return slist_impl_.before_begin(); }

   iterator begin()
   {  return slist_impl_.begin(); }

   iterator end()
   {  return slist_impl_.end(); }

   iterator last()
   {  return slist_impl_.last(); }

   void clear()
   {  slist_impl_.clear(); }

   iterator insert_after(iterator it, void_pointer m)
   {  return slist_impl_.insert_after(it, to_node(m));   }

   void push_front(void_pointer m)
   {  return slist_impl_.push_front(to_node(m));   }

   void push_back(void_pointer m)
   {  return slist_impl_.push_back(to_node(m));   }

   void pop_front()
   {  return slist_impl_.pop_front();   }

   void *front()
   {  return &*slist_impl_.begin();   }

   void splice_after(iterator after_this, basic_multiallocation_chain &x, iterator before_begin, iterator before_end)
   {  slist_impl_.splice_after(after_this, x.slist_impl_, before_begin, before_end);   }

   void splice_after(iterator after_this, basic_multiallocation_chain &x, iterator before_begin, iterator before_end, std::size_t n)
   {  slist_impl_.splice_after(after_this, x.slist_impl_, before_begin, before_end, n);   }

   void splice_after(iterator after_this, basic_multiallocation_chain &x)
   {  slist_impl_.splice_after(after_this, x.slist_impl_);   }

   void incorporate_after(iterator after_this, void_pointer begin , iterator before_end)
   {  slist_impl_.incorporate_after(after_this, &to_node(begin), &to_node(before_end));   }

   void incorporate_after(iterator after_this, void_pointer begin, void_pointer before_end, std::size_t n)
   {  slist_impl_.incorporate_after(after_this, &to_node(begin), &to_node(before_end), n);   }

   void swap(basic_multiallocation_chain &x)
   {  slist_impl_.swap(x.slist_impl_);   }

   static iterator iterator_to(void_pointer p)
   {  return slist_impl_t::s_iterator_to(to_node(p));   }

   std::pair<void_pointer, void_pointer> extract_data()
   {
      std::pair<void_pointer, void_pointer> ret
         (slist_impl_.begin().operator->()
         ,slist_impl_.last().operator->());
      slist_impl_.clear();
      return ret;
   }
};

template<class T>
struct cast_functor
{
   typedef typename containers_detail::add_reference<T>::type result_type;
   template<class U>
   result_type operator()(U &ptr) const
   {  return *static_cast<T*>(static_cast<void*>(&ptr));  }
};

template<class MultiallocationChain, class T>
class transform_multiallocation_chain
{
   private:
   BOOST_MOVE_MACRO_MOVABLE_BUT_NOT_COPYABLE(transform_multiallocation_chain)

   MultiallocationChain   holder_;
   typedef typename MultiallocationChain::void_pointer   void_pointer;
   typedef typename boost::pointer_to_other
      <void_pointer, T>::type                            pointer;

   static pointer cast(void_pointer p)
   {
      return pointer(static_cast<T*>(containers_detail::get_pointer(p)));
   }

   public:
   typedef transform_iterator
      < typename MultiallocationChain::iterator
      , containers_detail::cast_functor <T> >                 iterator;

   transform_multiallocation_chain()
      : holder_()
   {}

   transform_multiallocation_chain(BOOST_MOVE_MACRO_RV_REF(transform_multiallocation_chain) other)
      : holder_()
   {  this->swap(other); }

   transform_multiallocation_chain(BOOST_MOVE_MACRO_RV_REF(MultiallocationChain) other)
      : holder_(BOOST_CONTAINER_MOVE_NAMESPACE::move(other))
   {}

   transform_multiallocation_chain& operator=(BOOST_MOVE_MACRO_RV_REF(transform_multiallocation_chain) other)
   {
      transform_multiallocation_chain tmp(BOOST_CONTAINER_MOVE_NAMESPACE::move(other));
      this->swap(tmp);
      return *this;
   }

   void push_front(pointer mem)
   {  holder_.push_front(mem);  }

   void swap(transform_multiallocation_chain &other_chain)
   {  holder_.swap(other_chain.holder_); }

   void splice_after(iterator after_this, transform_multiallocation_chain &x, iterator before_begin, iterator before_end, std::size_t n)
   {  holder_.splice_after(after_this.base(), x.holder_, before_begin.base(), before_end.base(), n);  }

   void incorporate_after(iterator after_this, void_pointer begin, void_pointer before_end, std::size_t n)
   {  holder_.incorporate_after(after_this.base(), begin, before_end, n);  }

   void pop_front()
   {  holder_.pop_front();  }

   pointer front()
   {  return cast(holder_.front());   }

   bool empty() const
   {  return holder_.empty(); }

   iterator before_begin()
   {  return iterator(holder_.before_begin());   }

   iterator begin()
   {  return iterator(holder_.begin());   }

   iterator end()
   {  return iterator(holder_.end());   }

   iterator last()
   {  return iterator(holder_.last());   }

   std::size_t size() const
   {  return holder_.size();  }

   void clear()
   {  holder_.clear(); }

   iterator insert_after(iterator it, pointer m)
   {  return iterator(holder_.insert_after(it.base(), m)); }

   static iterator iterator_to(pointer p)
   {  return iterator(MultiallocationChain::iterator_to(p));  }

   std::pair<void_pointer, void_pointer> extract_data()
   {  return holder_.extract_data();  }

   MultiallocationChain extract_multiallocation_chain()
   {
      return MultiallocationChain(BOOST_CONTAINER_MOVE_NAMESPACE::move(holder_));
   }
};

}}}

// namespace containers_detail {
// namespace container {
// namespace boost {

#include INCLUDE_BOOST_CONTAINER_DETAIL_CONFIG_END_HPP

#endif   //BOOST_CONTAINERS_DETAIL_MULTIALLOCATION_CHAIN_HPP
