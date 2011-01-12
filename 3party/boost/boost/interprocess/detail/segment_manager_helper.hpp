//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////

#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_BASE_HPP
#define BOOST_INTERPROCESS_SEGMENT_MANAGER_BASE_HPP

#if (defined _MSC_VER) && (_MSC_VER >= 1200)
#  pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/detail/workaround.hpp>

#include <boost/pointer_to_other.hpp>

#include <boost/detail/no_exceptions_support.hpp>
#include <boost/interprocess/detail/type_traits.hpp>
#include <boost/interprocess/detail/utilities.hpp>
#include <boost/interprocess/detail/in_place_interface.hpp>
#include <boost/interprocess/exceptions.hpp>
#include <cstddef>   //std::size_t
#include <string>    //char_traits
#include <new>       //std::nothrow
#include <utility>   //std::pair
#include <boost/assert.hpp>   //BOOST_ASSERT
#include <functional>   //unary_function
#ifndef BOOST_NO_EXCEPTIONS
#include <exception>
#endif

//!\file
//!Describes the object placed in a memory segment that provides
//!named object allocation capabilities.

namespace boost{
namespace interprocess{

template<class MemoryManager>
class segment_manager_base;

//!An integer that describes the type of the
//!instance constructed in memory
enum instance_type {   anonymous_type, named_type, unique_type, max_allocation_type };

namespace detail{

template<class MemoryAlgorithm>
class mem_algo_deallocator
{
   void *            m_ptr;
   MemoryAlgorithm & m_algo;

   public:
   mem_algo_deallocator(void *ptr, MemoryAlgorithm &algo)
      :  m_ptr(ptr), m_algo(algo)
   {}

   void release()
   {  m_ptr = 0;  }

   ~mem_algo_deallocator()
   {  if(m_ptr) m_algo.deallocate(m_ptr);  }
};

/// @cond
struct block_header
{
   std::size_t    m_value_bytes;
   unsigned short m_num_char;
   unsigned char  m_value_alignment;
   unsigned char  m_alloc_type_sizeof_char;

   block_header(std::size_t value_bytes
               ,std::size_t value_alignment
               ,std::size_t alloc_type
               ,std::size_t sizeof_char
               ,std::size_t num_char
               )
      :  m_value_bytes(value_bytes)
      ,  m_num_char(num_char)
      ,  m_value_alignment(value_alignment)
      ,  m_alloc_type_sizeof_char
         ( ((unsigned char)alloc_type << 5u) | 
           ((unsigned char)sizeof_char & 0x1F)   )
   {};


   template<class T>
   block_header &operator= (const T& )
   {  return *this;  }

   std::size_t total_size() const
   {
      if(alloc_type() != anonymous_type){
         return name_offset() + (m_num_char+1)*sizeof_char();
      }
      else{
         return value_offset() + m_value_bytes;
      }
   }

   std::size_t value_bytes() const
   {  return m_value_bytes;   }

   template<class Header>
   std::size_t total_size_with_header() const
   {
      return get_rounded_size
               (  sizeof(Header)
               ,  detail::alignment_of<block_header>::value)
           + total_size();
   }

   std::size_t alloc_type() const
   {  return (m_alloc_type_sizeof_char >> 5u)&(unsigned char)0x7;  }

   std::size_t sizeof_char() const
   {  return m_alloc_type_sizeof_char & (unsigned char)0x1F;  }

   template<class CharType>
   CharType *name() const
   {  
      return const_cast<CharType*>(reinterpret_cast<const CharType*>
         (reinterpret_cast<const char*>(this) + name_offset()));
   }

   std::size_t name_length() const
   {  return m_num_char;   }

   std::size_t name_offset() const
   { 
      return value_offset() + get_rounded_size(m_value_bytes, sizeof_char());
   }

   void *value() const
   {
      return const_cast<char*>((reinterpret_cast<const char*>(this) + value_offset()));
   }

   std::size_t value_offset() const
   {
      return get_rounded_size(sizeof(block_header), m_value_alignment);
   }

   template<class CharType>
   bool less_comp(const block_header &b) const
   {
      return m_num_char < b.m_num_char ||
             (m_num_char < b.m_num_char && 
               std::char_traits<CharType>::compare
                  (name<CharType>(), b.name<CharType>(), m_num_char) < 0);
   }

   template<class CharType>
   bool equal_comp(const block_header &b) const
   {
      return m_num_char == b.m_num_char &&
             std::char_traits<CharType>::compare
               (name<CharType>(), b.name<CharType>(), m_num_char) == 0;
   }

   template<class T>
   static block_header *block_header_from_value(T *value)
   {  return block_header_from_value(value, sizeof(T), detail::alignment_of<T>::value);  }

   static block_header *block_header_from_value(const void *value, std::size_t sz, std::size_t algn)
   {  
      block_header * hdr = 
         const_cast<block_header*>
            (reinterpret_cast<const block_header*>(reinterpret_cast<const char*>(value) - 
               get_rounded_size(sizeof(block_header), algn)));
      (void)sz;
      //Some sanity checks
      BOOST_ASSERT(hdr->m_value_alignment == algn);
      BOOST_ASSERT(hdr->m_value_bytes % sz == 0);
      return hdr;
   }

   template<class Header>
   static block_header *from_first_header(Header *header)
   {  
      block_header * hdr = 
         reinterpret_cast<block_header*>(reinterpret_cast<char*>(header) + 
            get_rounded_size(sizeof(Header), detail::alignment_of<block_header>::value));
      //Some sanity checks
      return hdr;
   }

   template<class Header>
   static Header *to_first_header(block_header *bheader)
   {  
      Header * hdr = 
         reinterpret_cast<Header*>(reinterpret_cast<char*>(bheader) - 
         get_rounded_size(sizeof(Header), detail::alignment_of<block_header>::value));
      //Some sanity checks
      return hdr;
   }
};

inline void array_construct(void *mem, std::size_t num, detail::in_place_interface &table)
{
   //Try constructors
   std::size_t constructed = 0;
   BOOST_TRY{
      table.construct_n(mem, num, constructed);
   }
   //If there is an exception call destructors and erase index node
   BOOST_CATCH(...){
      std::size_t destroyed = 0;
      table.destroy_n(mem, constructed, destroyed);
      BOOST_RETHROW
   }
   BOOST_CATCH_END
}

template<class CharT>
struct intrusive_compare_key
{
   typedef CharT char_type;

   intrusive_compare_key(const CharT *str, std::size_t len)
      :  mp_str(str), m_len(len)
   {}

   const CharT *  mp_str;
   std::size_t    m_len;
};

//!This struct indicates an anonymous object creation
//!allocation
template<instance_type type>
class instance_t
{
   instance_t(){}
};

template<class T>
struct char_if_void
{
   typedef T type;
};

template<>
struct char_if_void<void>
{
   typedef char type;
};

typedef instance_t<anonymous_type>  anonymous_instance_t;
typedef instance_t<unique_type>     unique_instance_t;


template<class Hook, class CharType>
struct intrusive_value_type_impl
   :  public Hook
{
   private:
   //Non-copyable
   intrusive_value_type_impl(const intrusive_value_type_impl &);
   intrusive_value_type_impl& operator=(const intrusive_value_type_impl &);

   public:
   typedef CharType char_type;

   intrusive_value_type_impl(){}

   enum  {  BlockHdrAlignment = detail::alignment_of<block_header>::value  };

   block_header *get_block_header() const
   {
      return const_cast<block_header*>
         (reinterpret_cast<const block_header *>(reinterpret_cast<const char*>(this) +
            get_rounded_size(sizeof(*this), BlockHdrAlignment)));
   }

   bool operator <(const intrusive_value_type_impl<Hook, CharType> & other) const
   {  return (this->get_block_header())->template less_comp<CharType>(*other.get_block_header());  }

   bool operator ==(const intrusive_value_type_impl<Hook, CharType> & other) const
   {  return (this->get_block_header())->template equal_comp<CharType>(*other.get_block_header());  }

   static intrusive_value_type_impl *get_intrusive_value_type(block_header *hdr)
   {
      return reinterpret_cast<intrusive_value_type_impl *>(reinterpret_cast<char*>(hdr) -
         get_rounded_size(sizeof(intrusive_value_type_impl), BlockHdrAlignment));
   }

   CharType *name() const
   {  return get_block_header()->template name<CharType>(); }

   std::size_t name_length() const
   {  return get_block_header()->name_length(); }

   void *value() const
   {  return get_block_header()->value(); }
};

template<class CharType>
class char_ptr_holder
{
   public:
   char_ptr_holder(const CharType *name) 
      : m_name(name)
   {}

   char_ptr_holder(const detail::anonymous_instance_t *) 
      : m_name(static_cast<CharType*>(0))
   {}

   char_ptr_holder(const detail::unique_instance_t *) 
      : m_name(reinterpret_cast<CharType*>(-1))
   {}

   operator const CharType *()
   {  return m_name;  }

   private:
   const CharType *m_name;
};

//!The key of the the named allocation information index. Stores an offset pointer 
//!to a null terminated string and the length of the string to speed up sorting
template<class CharT, class VoidPointer>
struct index_key
{
   typedef typename boost::
      pointer_to_other<VoidPointer, const CharT>::type   const_char_ptr_t;
   typedef CharT                                         char_type;

   private:
   //Offset pointer to the object's name
   const_char_ptr_t  mp_str;
   //Length of the name buffer (null NOT included)
   std::size_t       m_len;
   public:

   //!Constructor of the key
   index_key (const char_type *name, std::size_t length)
      : mp_str(name), m_len(length) {}

   //!Less than function for index ordering
   bool operator < (const index_key & right) const
   {
      return (m_len < right.m_len) || 
               (m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str)
                  ,detail::get_pointer(right.mp_str), m_len) < 0);
   }

   //!Equal to function for index ordering
   bool operator == (const index_key & right) const
   {
      return   m_len == right.m_len && 
               std::char_traits<char_type>::compare 
                  (detail::get_pointer(mp_str),
                   detail::get_pointer(right.mp_str), m_len) == 0;
   }

   void name(const CharT *name)
   {  mp_str = name; }

   void name_length(std::size_t len)
   {  m_len = len; }

   const CharT *name() const
   {  return detail::get_pointer(mp_str); }

   std::size_t name_length() const
   {  return m_len; }
};

//!The index_data stores a pointer to a buffer and the element count needed
//!to know how many destructors must be called when calling destroy
template<class VoidPointer>
struct index_data
{
   typedef VoidPointer void_pointer;
   void_pointer    m_ptr;
   index_data(void *ptr) : m_ptr(ptr){}

   void *value() const
   {  return static_cast<void*>(detail::get_pointer(m_ptr));  }
};

template<class MemoryAlgorithm>
struct segment_manager_base_type
{  typedef segment_manager_base<MemoryAlgorithm> type;   };

template<class CharT, class MemoryAlgorithm>
struct index_config
{
   typedef typename MemoryAlgorithm::void_pointer        void_pointer;
   typedef CharT                                         char_type;
   typedef detail::index_key<CharT, void_pointer>        key_type;
   typedef detail::index_data<void_pointer>              mapped_type;
   typedef typename segment_manager_base_type
      <MemoryAlgorithm>::type                            segment_manager_base;

   template<class HeaderBase>
   struct intrusive_value_type
   {  typedef detail::intrusive_value_type_impl<HeaderBase, CharT>  type; };

   typedef intrusive_compare_key<CharT>            intrusive_compare_key_type;
};

template<class Iterator, bool intrusive>
class segment_manager_iterator_value_adaptor
{
   typedef typename Iterator::value_type        iterator_val_t;
   typedef typename iterator_val_t::char_type   char_type;

   public:
   segment_manager_iterator_value_adaptor(const typename Iterator::value_type &val)
      :  m_val(&val)
   {}

   const char_type *name() const
   {  return m_val->name(); }

   std::size_t name_length() const
   {  return m_val->name_length(); }

   const void *value() const
   {  return m_val->value(); }

   const typename Iterator::value_type *m_val;
};


template<class Iterator>
class segment_manager_iterator_value_adaptor<Iterator, false>
{
   typedef typename Iterator::value_type        iterator_val_t;
   typedef typename iterator_val_t::first_type  first_type;
   typedef typename iterator_val_t::second_type second_type;
   typedef typename first_type::char_type       char_type;

   public:
   segment_manager_iterator_value_adaptor(const typename Iterator::value_type &val)
      :  m_val(&val)
   {}

   const char_type *name() const
   {  return m_val->first.name(); }

   std::size_t name_length() const
   {  return m_val->first.name_length(); }

   const void *value() const
   {
      return reinterpret_cast<block_header*>
         (detail::get_pointer(m_val->second.m_ptr))->value();
   }

   const typename Iterator::value_type *m_val;
};

template<class Iterator, bool intrusive>
struct segment_manager_iterator_transform
   :  std::unary_function< typename Iterator::value_type
                         , segment_manager_iterator_value_adaptor<Iterator, intrusive> >
{
   typedef segment_manager_iterator_value_adaptor<Iterator, intrusive> result_type;
   
   result_type operator()(const typename Iterator::value_type &arg) const
   {  return result_type(arg); }
};

}  //namespace detail {

//These pointers are the ones the user will use to 
//indicate previous allocation types
static const detail::anonymous_instance_t   * anonymous_instance = 0;
static const detail::unique_instance_t      * unique_instance = 0;

namespace detail_really_deep_namespace {

//Otherwise, gcc issues a warning of previously defined
//anonymous_instance and unique_instance
struct dummy
{
   dummy()
   {
      (void)anonymous_instance;
      (void)unique_instance;
   }
};

}  //detail_really_deep_namespace

}} //namespace boost { namespace interprocess

#include <boost/interprocess/detail/config_end.hpp>

#endif //#ifndef BOOST_INTERPROCESS_SEGMENT_MANAGER_BASE_HPP

