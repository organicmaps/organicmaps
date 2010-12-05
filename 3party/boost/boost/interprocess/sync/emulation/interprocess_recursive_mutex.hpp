//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2005-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////
//
// Parts of the pthread code come from Boost Threads code:
//
//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.
//////////////////////////////////////////////////////////////////////////////

#include <boost/interprocess/detail/posix_time_types_wrk.hpp>
#include <boost/interprocess/detail/os_thread_functions.hpp>
#include <boost/interprocess/exceptions.hpp>

namespace boost {

namespace interprocess {

inline interprocess_recursive_mutex::interprocess_recursive_mutex() 
   : m_nLockCount(0), m_nOwner(detail::get_invalid_systemwide_thread_id()){}

inline interprocess_recursive_mutex::~interprocess_recursive_mutex(){}

inline void interprocess_recursive_mutex::lock()
{
   typedef detail::OS_systemwide_thread_id_t handle_t;
   const handle_t thr_id(detail::get_current_systemwide_thread_id());
   handle_t old_id;
   detail::systemwide_thread_id_copy(m_nOwner, old_id);
   if(detail::equal_systemwide_thread_id(thr_id , old_id)){
      if((unsigned int)(m_nLockCount+1) == 0){
         //Overflow, throw an exception
         throw interprocess_exception();
      } 
      ++m_nLockCount;
   }
   else{
      m_mutex.lock();
      detail::systemwide_thread_id_copy(thr_id, m_nOwner);
      m_nLockCount = 1;
   }
}

inline bool interprocess_recursive_mutex::try_lock()
{
   typedef detail::OS_systemwide_thread_id_t handle_t;
   handle_t thr_id(detail::get_current_systemwide_thread_id());
   handle_t old_id;
   detail::systemwide_thread_id_copy(m_nOwner, old_id);
   if(detail::equal_systemwide_thread_id(thr_id , old_id)) {  // we own it
      if((unsigned int)(m_nLockCount+1) == 0){
         //Overflow, throw an exception
         throw interprocess_exception();
      } 
      ++m_nLockCount;
      return true;
   }
   if(m_mutex.try_lock()){
      detail::systemwide_thread_id_copy(thr_id, m_nOwner);
      m_nLockCount = 1;
      return true;
   }
   return false;
}

inline bool interprocess_recursive_mutex::timed_lock(const boost::posix_time::ptime &abs_time)
{
   typedef detail::OS_systemwide_thread_id_t handle_t;
   if(abs_time == boost::posix_time::pos_infin){
      this->lock();
      return true;
   }
   const handle_t thr_id(detail::get_current_systemwide_thread_id());
   handle_t old_id;
   detail::systemwide_thread_id_copy(m_nOwner, old_id);
   if(detail::equal_systemwide_thread_id(thr_id , old_id)) {  // we own it
      if((unsigned int)(m_nLockCount+1) == 0){
         //Overflow, throw an exception
         throw interprocess_exception();
      } 
      ++m_nLockCount;
      return true;
   }
   if(m_mutex.timed_lock(abs_time)){
      detail::systemwide_thread_id_copy(thr_id, m_nOwner);
      m_nLockCount = 1;
      return true;
   }
   return false;
}

inline void interprocess_recursive_mutex::unlock()
{
   typedef detail::OS_systemwide_thread_id_t handle_t;
   handle_t old_id;
   detail::systemwide_thread_id_copy(m_nOwner, old_id);
   const handle_t thr_id(detail::get_current_systemwide_thread_id());
   (void)old_id;
   (void)thr_id;
   assert(detail::equal_systemwide_thread_id(thr_id, old_id));
   --m_nLockCount;
   if(!m_nLockCount){
      const handle_t new_id(detail::get_invalid_systemwide_thread_id());
      detail::systemwide_thread_id_copy(new_id, m_nOwner);
      m_mutex.unlock();
   }
}

}  //namespace interprocess {

}  //namespace boost {

