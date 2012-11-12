//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2012. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/container for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_CONTAINER_DETAIL_MULTIALLOCATION_CHAIN_HPP
#define BOOST_CONTAINER_DETAIL_MULTIALLOCATION_CHAIN_HPP

#include "config_begin.hpp"
#include <boost/container/container_fwd.hpp>
#include <boost/container/detail/utilities.hpp>
#include <boost/container/detail/type_traits.hpp>
#include <boost/container/detail/transform_iterator.hpp>
#include <boost/intrusive/slist.hpp>
#include <boost/intrusive/pointer_traits.hpp>
#include <boost/type_traits/make_unsigned.hpp>
#include <boost/move/move.hpp>

namespace boost {
namespace container {
namespace container_detail {

template<class VoidPointer>
class basic_multiallocation_chain
{
   private:
   typedef bi::slist_base_hook<bi::void_pointer<VoidPointer>
                        ,bi::link_mode<bi::normal_link>
                        > node;

   typedef typename boost::intrusive::pointer_traits
      <VoidPointer>::template rebind_pointer<char>::type    char_ptr;
   typedef typename boost::intrusive::
      pointer_traits<char_ptr>::difference_type             difference_type;

   typedef bi::slist< node
                    , bi::linear<true>
                    , bi::cache_last<true>
                    , bi::size_type<typename boost::make_unsigned<difference_type>::type>
                    > slist_impl_t;
   slist_impl_t slist_impl_;

   typedef typename boost::intrusive::pointer_traits
      <VoidPointer>::template rebind_pointer<node>::type    node_ptr;
   typedef typename boost::intrusive::
      pointer_traits<node_ptr>                              node_ptr_traits;

   static node & build_node(const VoidPointer &p)
   {
      return *::new (static_cast<node*>(static_cast<void*>(container_detail::to_raw_pointer(p)))) node;
   }

   static VoidPointer destroy_node(node &n)
   {
      VoidPointer retptr = node_ptr_traits::pointer_to(n);
      n.~node();
      return retptr;
   }

   static node_ptr to_node_ptr(VoidPointer p)
   {  return node_ptr_traits::static_cast_from(p);   }

   BOOST_MOVABLE_BUT_NOT_COPYABLE(basic_multiallocation_chain)

   public:

   typedef VoidPointer  void_pointer;
   typedef typename slist_impl_t::iterator iterator;
   typedef typename slist_impl_t::size_type size_type;

   basic_multiallocation_chain()
      :  slist_impl_()
   {}

   basic_multiallocation_chain(BOOST_RV_REF(basic_multiallocation_chain) other)
      :  slist_impl_()
   {  slist_impl_.swap(other.slist_impl_); }

   basic_multiallocation_chain& operator=(BOOST_RV_REF(basic_multiallocation_chain) other)
   {
      basic_multiallocation_chain tmp(boost::move(other));
      this->swap(tmp);
      return *this;
   }

   bool empty() const
   {  return slist_impl_.empty(); }

   size_type size() const
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
   {  return slist_impl_.insert_after(it, build_node(m));   }

   void push_front(void_pointer m)
   {  return slist_impl_.push_front(build_node(m));  }

   void push_back(void_pointer m)
   {  return slist_impl_.push_back(build_node(m));   }

   void_pointer pop_front()
   {
      node & n = slist_impl_.front();
      void_pointer ret = destroy_node(n);
      slist_impl_.pop_front();
      return ret;
   }

   void splice_after(iterator after_this, basic_multiallocation_chain &x, iterator before_begin_, iterator before_end)
   {  slist_impl_.splice_after(after_this, x.slist_impl_, before_begin_, before_end);   }

   void splice_after(iterator after_this, basic_multiallocation_chain &x, iterator before_begin_, iterator before_end, size_type n)
   {  slist_impl_.splice_after(after_this, x.slist_impl_, before_begin_, before_end, n);   }

   void splice_after(iterator after_this, basic_multiallocation_chain &x)
   {  slist_impl_.splice_after(after_this, x.slist_impl_);   }

   void incorporate_after(iterator after_this, void_pointer begin_ , iterator before_end)
   {
      slist_impl_.incorporate_after(after_this, to_node_ptr(begin_), to_node_ptr(before_end));
   }

   void incorporate_after(iterator after_this, void_pointer begin_, void_pointer before_end, size_type n)
   {  slist_impl_.incorporate_after(after_this, to_node_ptr(begin_), to_node_ptr(before_end), n);   }

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
   typedef typename container_detail::add_reference<T>::type result_type;
   template<class U>
   result_type operator()(U &ptr) const
   {  return *static_cast<T*>(static_cast<void*>(&ptr));  }
};

template<class MultiallocationChain, class T>
class transform_multiallocation_chain
{
   private:
   BOOST_MOVABLE_BUT_NOT_COPYABLE(transform_multiallocation_chain)

   MultiallocationChain   holder_;
   typedef typename MultiallocationChain::void_pointer   void_pointer;
   typedef typename boost::intrusive::pointer_traits
      <void_pointer>                                     void_pointer_traits;
   typedef typename void_pointer_traits::template
      rebind_pointer<T>::type                            pointer;
   typedef typename boost::intrusive::pointer_traits
      <pointer>                                          pointer_traits;

   static pointer cast(const void_pointer &p)
   {  return pointer_traits::static_cast_from(p);  }

   public:
   typedef transform_iterator
      < typename MultiallocationChain::iterator
      , container_detail::cast_functor <T> >             iterator;
   typedef typename MultiallocationChain::size_type      size_type;

   transform_multiallocation_chain()
      : holder_()
   {}

   transform_multiallocation_chain(BOOST_RV_REF(transform_multiallocation_chain) other)
      : holder_()
   {  this->swap(other); }

   transform_multiallocation_chain(BOOST_RV_REF(MultiallocationChain) other)
      : holder_(boost::move(other))
   {}

   transform_multiallocation_chain& operator=(BOOST_RV_REF(transform_multiallocation_chain) other)
   {
      transform_multiallocation_chain tmp(boost::move(other));
      this->swap(tmp);
      return *this;
   }

   void push_front(pointer mem)
   {  holder_.push_front(mem);  }

   void swap(transform_multiallocation_chain &other_chain)
   {  holder_.swap(other_chain.holder_); }

   void splice_after(iterator after_this, transform_multiallocation_chain &x, iterator before_begin_, iterator before_end, size_type n)
   {  holder_.splice_after(after_this.base(), x.holder_, before_begin_.base(), before_end.base(), n);  }

   void incorporate_after(iterator after_this, pointer begin_, pointer before_end, size_type n)
   {  holder_.incorporate_after(after_this.base(), begin_, before_end, n);  }

   pointer pop_front()
   {  return cast(holder_.pop_front());  }

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

   size_type size() const
   {  return holder_.size();  }

   void clear()
   {  holder_.clear(); }

   iterator insert_after(iterator it, pointer m)
   {  return iterator(holder_.insert_after(it.base(), m)); }

   static iterator iterator_to(pointer p)
   {  return iterator(MultiallocationChain::iterator_to(p));  }

   std::pair<pointer, pointer> extract_data()
   {
      std::pair<void_pointer, void_pointer> data(holder_.extract_data());
      return std::pair<pointer, pointer>(cast(data.first), cast(data.second));
   }

   MultiallocationChain extract_multiallocation_chain()
   {
      return MultiallocationChain(boost::move(holder_));
   }
};

}}}

// namespace container_detail {
// namespace container {
// namespace boost {

#include <boost/container/detail/config_end.hpp>

#endif   //BOOST_CONTAINER_DETAIL_MULTIALLOCATION_CHAIN_HPP
