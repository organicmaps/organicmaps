//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_OFFSET_PTR_HPP
#define BOOST_OFFSET_PTR_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/interprocess/interprocess_fwd.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/cast_tags.hpp>
#include <boost/interprocess/detail/mpl.hpp>
#include <boost/assert.hpp>
#include <ostream>
#include <istream>
#include <iterator>

//!\file
//!Describes a smart pointer that stores the offset between this pointer and
//!target pointee, called offset_ptr.

namespace boost {

//Predeclarations
template <class T>
struct has_trivial_constructor;

template <class T>
struct has_trivial_destructor;

namespace interprocess {

//!A smart pointer that stores the offset between between the pointer and the
//!the object it points. This allows offset allows special properties, since
//!the pointer is independent from the address address of the pointee, if the
//!pointer and the pointee are still separated by the same offset. This feature
//!converts offset_ptr in a smart pointer that can be placed in shared memory and
//!memory mapped files mapped in different addresses in every process.
template <class PointedType>
class offset_ptr
{
   /// @cond
   typedef offset_ptr<PointedType>           self_t;

   void unspecified_bool_type_func() const {}
   typedef void (self_t::*unspecified_bool_type)() const;

   #if defined(_MSC_VER) && (_MSC_VER >= 1400)
   __declspec(noinline) //this workaround is needed for msvc-8.0 and msvc-9.0
   #endif
   void set_offset(const volatile void *ptr)
   {  set_offset(const_cast<const void*>(ptr)); }

   void set_offset(const void *ptr)
   {
      const char *p = static_cast<const char*>(ptr);
      //offset == 1 && ptr != 0 is not legal for this pointer
      if(!ptr){
         m_offset = 1;
      }
      else{
         m_offset = p - reinterpret_cast<const char*>(this);
         BOOST_ASSERT(m_offset != 1);
      }
   }

   #if defined(_MSC_VER) && (_MSC_VER >= 1400)
   __declspec(noinline) //this workaround is needed for msvc-8.0 and msvc-9.0
   #endif
   void* get_pointer() const
   {  return (m_offset == 1) ? 0 : (const_cast<char*>(reinterpret_cast<const char*>(this)) + m_offset); }

   void inc_offset(std::ptrdiff_t bytes)
   {  m_offset += bytes;   }

   void dec_offset(std::ptrdiff_t bytes)
   {  m_offset -= bytes;   }

   std::ptrdiff_t m_offset; //Distance between this object and pointed address
   /// @endcond

   public:
   typedef PointedType *                     pointer;
   typedef typename detail::
      add_reference<PointedType>::type       reference;
   typedef PointedType                       value_type;
   typedef std::ptrdiff_t                    difference_type;
   typedef std::random_access_iterator_tag   iterator_category;

   public:   //Public Functions

   //!Constructor from raw pointer (allows "0" pointer conversion).
   //!Never throws.
   offset_ptr(pointer ptr = 0) {  this->set_offset(ptr); }

   //!Constructor from other pointer.
   //!Never throws.
   template <class T>
   offset_ptr(T *ptr) 
   {  pointer p (ptr);  (void)p; this->set_offset(p); }

   //!Constructor from other offset_ptr
   //!Never throws.
   offset_ptr(const offset_ptr& ptr) 
   {  this->set_offset(ptr.get());   }

   //!Constructor from other offset_ptr. If pointers of pointee types are 
   //!convertible, offset_ptrs will be convertibles. Never throws.
   template<class T2>
   offset_ptr(const offset_ptr<T2> &ptr) 
   {  pointer p(ptr.get());  (void)p; this->set_offset(p);   }

   //!Emulates static_cast operator.
   //!Never throws.
   template<class Y>
   offset_ptr(const offset_ptr<Y> & r, detail::static_cast_tag)
   {  this->set_offset(static_cast<PointedType*>(r.get()));   }

   //!Emulates const_cast operator.
   //!Never throws.
   template<class Y>
   offset_ptr(const offset_ptr<Y> & r, detail::const_cast_tag)
   {  this->set_offset(const_cast<PointedType*>(r.get()));   }

   //!Emulates dynamic_cast operator.
   //!Never throws.
   template<class Y>
   offset_ptr(const offset_ptr<Y> & r, detail::dynamic_cast_tag)
   {  this->set_offset(dynamic_cast<PointedType*>(r.get()));   }

   //!Emulates reinterpret_cast operator.
   //!Never throws.
   template<class Y>
   offset_ptr(const offset_ptr<Y> & r, detail::reinterpret_cast_tag)
   {  this->set_offset(reinterpret_cast<PointedType*>(r.get()));   }

   //!Obtains raw pointer from offset.
   //!Never throws.
   pointer get()const
   {  return static_cast<pointer>(this->get_pointer());   }

   std::ptrdiff_t get_offset()
   {  return m_offset;  }

   //!Pointer-like -> operator. It can return 0 pointer.
   //!Never throws.
   pointer operator->() const           
   {  return this->get(); }

   //!Dereferencing operator, if it is a null offset_ptr behavior 
   //!   is undefined. Never throws.
   reference operator* () const           
   {
      pointer p = this->get();
      reference r = *p;
      return r;
   }

   //!Indexing operator.
   //!Never throws.
   reference operator[](std::ptrdiff_t idx) const   
   {  return this->get()[idx];  }

   //!Assignment from pointer (saves extra conversion).
   //!Never throws.
   offset_ptr& operator= (pointer from)
   {  this->set_offset(from); return *this;  }

   //!Assignment from other offset_ptr.
   //!Never throws.
   offset_ptr& operator= (const offset_ptr & pt)
   {  pointer p(pt.get());  (void)p; this->set_offset(p);  return *this;  }

   //!Assignment from related offset_ptr. If pointers of pointee types 
   //!   are assignable, offset_ptrs will be assignable. Never throws.
   template <class T2>
   offset_ptr& operator= (const offset_ptr<T2> & pt)
   {  pointer p(pt.get()); this->set_offset(p);  return *this;  }
 
   //!offset_ptr + std::ptrdiff_t.
   //!Never throws.
   offset_ptr operator+ (std::ptrdiff_t offset) const   
   {  return offset_ptr(this->get()+offset);   }

   //!offset_ptr - std::ptrdiff_t.
   //!Never throws.
   offset_ptr operator- (std::ptrdiff_t offset) const   
   {  return offset_ptr(this->get()-offset);   }

   //!offset_ptr += std::ptrdiff_t.
   //!Never throws.
   offset_ptr &operator+= (std::ptrdiff_t offset)
   {  this->inc_offset(offset * sizeof (PointedType));   return *this;  }

   //!offset_ptr -= std::ptrdiff_t.
   //!Never throws.
   offset_ptr &operator-= (std::ptrdiff_t offset)
   {  this->dec_offset(offset * sizeof (PointedType));   return *this;  }

   //!++offset_ptr.
   //!Never throws.
   offset_ptr& operator++ (void) 
   {  this->inc_offset(sizeof (PointedType));   return *this;  }

   //!offset_ptr++.
   //!Never throws.
   offset_ptr operator++ (int)
   {  offset_ptr temp(*this); ++*this; return temp; }

   //!--offset_ptr.
   //!Never throws.
   offset_ptr& operator-- (void) 
   {  this->dec_offset(sizeof (PointedType));   return *this;  }

   //!offset_ptr--.
   //!Never throws.
   offset_ptr operator-- (int)
   {  offset_ptr temp(*this); --*this; return temp; }

   //!safe bool conversion operator.
   //!Never throws.
   operator unspecified_bool_type() const  
   {  return this->get()? &self_t::unspecified_bool_type_func : 0;   }

   //!Not operator. Not needed in theory, but improves portability. 
   //!Never throws
   bool operator! () const
   {  return this->get() == 0;   }
/*
   friend void swap (offset_ptr &pt, offset_ptr &pt2)
   {  
      value_type *ptr = pt.get();
      pt = pt2;
      pt2 = ptr;
   }
*/
};

//!offset_ptr<T1> == offset_ptr<T2>.
//!Never throws.
template<class T1, class T2>
inline bool operator== (const offset_ptr<T1> &pt1, 
                        const offset_ptr<T2> &pt2)
{  return pt1.get() == pt2.get();  }

//!offset_ptr<T1> != offset_ptr<T2>.
//!Never throws.
template<class T1, class T2>
inline bool operator!= (const offset_ptr<T1> &pt1, 
                        const offset_ptr<T2> &pt2)
{  return pt1.get() != pt2.get();  }

//!offset_ptr<T1> < offset_ptr<T2>.
//!Never throws.
template<class T1, class T2>
inline bool operator< (const offset_ptr<T1> &pt1, 
                       const offset_ptr<T2> &pt2)
{  return pt1.get() < pt2.get();  }

//!offset_ptr<T1> <= offset_ptr<T2>.
//!Never throws.
template<class T1, class T2>
inline bool operator<= (const offset_ptr<T1> &pt1, 
                        const offset_ptr<T2> &pt2)
{  return pt1.get() <= pt2.get();  }

//!offset_ptr<T1> > offset_ptr<T2>.
//!Never throws.
template<class T1, class T2>
inline bool operator> (const offset_ptr<T1> &pt1, 
                       const offset_ptr<T2> &pt2)
{  return pt1.get() > pt2.get();  }

//!offset_ptr<T1> >= offset_ptr<T2>.
//!Never throws.
template<class T1, class T2>
inline bool operator>= (const offset_ptr<T1> &pt1, 
                        const offset_ptr<T2> &pt2)
{  return pt1.get() >= pt2.get();  }

//!operator<<
//!for offset ptr
template<class E, class T, class Y> 
inline std::basic_ostream<E, T> & operator<< 
   (std::basic_ostream<E, T> & os, offset_ptr<Y> const & p)
{  return os << p.get_offset();   }

//!operator>> 
//!for offset ptr
template<class E, class T, class Y> 
inline std::basic_istream<E, T> & operator>> 
   (std::basic_istream<E, T> & is, offset_ptr<Y> & p)
{  return is >> p.get_offset();  }

//!std::ptrdiff_t + offset_ptr
//!operation
template<class T>
inline offset_ptr<T> operator+(std::ptrdiff_t diff, const offset_ptr<T>& right)
{  return right + diff;  }

//!offset_ptr - offset_ptr
//!operation
template<class T, class T2>
inline std::ptrdiff_t operator- (const offset_ptr<T> &pt, const offset_ptr<T2> &pt2)
{  return pt.get()- pt2.get();   }

//!swap specialization 
//!for offset_ptr
template<class T>
inline void swap (boost::interprocess::offset_ptr<T> &pt, 
                  boost::interprocess::offset_ptr<T> &pt2)
{  
   typename offset_ptr<T>::value_type *ptr = pt.get();
   pt = pt2;
   pt2 = ptr;
}

//!Simulation of static_cast between pointers. Never throws.
template<class T, class U> 
inline boost::interprocess::offset_ptr<T> 
   static_pointer_cast(boost::interprocess::offset_ptr<U> const & r)
{  
   return boost::interprocess::offset_ptr<T>
            (r, boost::interprocess::detail::static_cast_tag());  
}

//!Simulation of const_cast between pointers. Never throws.
template<class T, class U> 
inline boost::interprocess::offset_ptr<T>
   const_pointer_cast(boost::interprocess::offset_ptr<U> const & r)
{  
   return boost::interprocess::offset_ptr<T>
            (r, boost::interprocess::detail::const_cast_tag());  
}

//!Simulation of dynamic_cast between pointers. Never throws.
template<class T, class U> 
inline boost::interprocess::offset_ptr<T> 
   dynamic_pointer_cast(boost::interprocess::offset_ptr<U> const & r)
{  
   return boost::interprocess::offset_ptr<T>
            (r, boost::interprocess::detail::dynamic_cast_tag());  
}

//!Simulation of reinterpret_cast between pointers. Never throws.
template<class T, class U> 
inline boost::interprocess::offset_ptr<T>
   reinterpret_pointer_cast(boost::interprocess::offset_ptr<U> const & r)
{  
   return boost::interprocess::offset_ptr<T>
            (r, boost::interprocess::detail::reinterpret_cast_tag());  
}

}  //namespace interprocess {

/// @cond

//!has_trivial_constructor<> == true_type specialization for optimizations
template <class T>
struct has_trivial_constructor< boost::interprocess::offset_ptr<T> > 
{
   enum { value = true };
};

///has_trivial_destructor<> == true_type specialization for optimizations
template <class T>
struct has_trivial_destructor< boost::interprocess::offset_ptr<T> > 
{
   enum { value = true };
};

//#if !defined(_MSC_VER) || (_MSC_VER >= 1400)
namespace interprocess {
//#endif
//!get_pointer() enables boost::mem_fn to recognize offset_ptr. 
//!Never throws.
template<class T>
inline T * get_pointer(boost::interprocess::offset_ptr<T> const & p)
{  return p.get();   }
//#if !defined(_MSC_VER) || (_MSC_VER >= 1400)
}  //namespace interprocess
//#endif

/// @endcond
}  //namespace boost {

/// @cond

namespace boost{

//This is to support embedding a bit in the pointer
//for intrusive containers, saving space
namespace intrusive {

//Predeclaration to avoid including header
template<class VoidPointer, std::size_t N>
struct max_pointer_plus_bits;

template<std::size_t Alignment>
struct max_pointer_plus_bits<boost::interprocess::offset_ptr<void>, Alignment>
{
   //The offset ptr can embed one bit less than the alignment since it
   //uses offset == 1 to store the null pointer.
   static const std::size_t value = ::boost::interprocess::detail::ls_zeros<Alignment>::value - 1;
};

//Predeclaration
template<class Pointer, std::size_t NumBits>
struct pointer_plus_bits;

template<class T, std::size_t NumBits>
struct pointer_plus_bits<boost::interprocess::offset_ptr<T>, NumBits>
{
   typedef boost::interprocess::offset_ptr<T>         pointer;
   //Bits are stored in the lower bits of the pointer except the LSB,
   //because this bit is used to represent the null pointer.
   static const std::size_t Mask = ((std::size_t(1) << NumBits)-1)<<1u; 

   static pointer get_pointer(const pointer &n)
   {  return reinterpret_cast<T*>(std::size_t(n.get()) & ~std::size_t(Mask));  }

   static void set_pointer(pointer &n, pointer p)
   {
      std::size_t pint = std::size_t(p.get());
      assert(0 == (std::size_t(pint) & Mask));
      n = reinterpret_cast<T*>(pint | (std::size_t(n.get()) & std::size_t(Mask)));
   }

   static std::size_t get_bits(const pointer &n)
   {  return(std::size_t(n.get()) & std::size_t(Mask)) >> 1u;  }

   static void set_bits(pointer &n, std::size_t b)
   {
      assert(b < (std::size_t(1) << NumBits));
      n = reinterpret_cast<T*>(std::size_t(get_pointer(n).get()) | (b << 1u));
   }
};

}  //namespace intrusive
}  //namespace boost{
/// @endcond

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_OFFSET_PTR_HPP

