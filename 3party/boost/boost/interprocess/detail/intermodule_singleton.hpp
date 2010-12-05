/* Copyright 2006-2009 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */
//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Ion Gaztanaga 2009-2009. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/interprocess for documentation.
//
//////////////////////////////////////////////////////////////////////////////


#ifndef BOOST_INTERPROCESS_INTERMODULE_SINGLETON_HPP
#define BOOST_INTERPROCESS_INTERMODULE_SINGLETON_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/interprocess/detail/config_begin.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>

namespace boost{
namespace interprocess{
namespace detail{

const char *get_singleton_unique_name()
{  return "unique_name";

template<typename C>
struct atomic_functor
{
   atomic_functor(managed_shared_memory &seg)
      : seg_(seg)
   {}

   inline void operator()()
   {
      referenced_instance**pptr = seg.find_or_construct<referenced_instance*>(unique_instance)();
      detail::atomic_cas32(
   }

   managed_shared_memory &seg_;
};

template<class C>
struct intermodule_singleton_instantiator
{
   intermodule_singleton_instantiator()
   :  ppref(0), pc(0)
   {
   
   bool done = false;
      try{
         managed_shared_memory seg(interprocess::create_only, get_singleton_unique_name(), 16384);
         //Register cleanup??? We can't because the address of a local function might be the
         //address of a dll, and that dll can be UNLOADED!!!
      }
      catch(interprocess_exception &ex){
         if(ex.get_error_code() != already_exists_error){
            managed_shared_memory seg(open_only, "unique_name", 16384);
            seg.find_or_construct<referenced_instance*>(unique_instance)();
         }
         else{
            throw;
         }
      }

   {
      ppref=seg.find_or_construct<referenced_instance*>(unique_instance)();
      if(*ppref){
         /* As in some OSes Boost.Interprocess memory segments can outlive
         * their associated processes, there is a possibility that we
         * retrieve a dangling pointer (coming from a previous aborted run,
         * for instance). Try to protect against this by checking that
         * the contents of the pointed object are consistent.
         */
         if(std::strcmp(segment_name,(*ppref)->segment_name)!=0){
         *ppref=0; /* dangling pointer! */
         }
         else ++((*ppref)->ref);
      }
   }
   if(!*ppref){
      std::auto_ptr<referenced_instance> apc(
         new referenced_instance(segment_name));
      interprocess::scoped_lock<interprocess::named_mutex> lock(mutex);
      ppref=seg.find_or_construct<referenced_instance*>(
         typeid(C).name())((referenced_instance*)0);
      if(!*ppref)*ppref=apc.release();
      ++((*ppref)->ref);
   }
   pc=&(*ppref)->c;
   }
   
   ~intermodule_singleton_instantiator()
   {
   /* As in construction time, actual deletion is performed outside the
      * lock to avoid leaving the lock dangling in case of crash.
      */
      
   referenced_instance* pref=0;
   {
      interprocess::scoped_lock<interprocess::named_mutex> lock(mutex);
      if(--((*ppref)->ref)==0){
         pref=*ppref;
         *ppref=0;
      }
   }
   if(pref)delete pref;
   }
};

template<typename C>
struct intermodule_singleton
{
  static C& get()
  {
    static intermodule_singleton_instantiator<C> instance;    
    return instance.get();
  }

private:

    C& get()const{return *pc;}
    
  private:
    interprocess::managed_shared_memory seg;
    struct referenced_instance
    {
      referenced_instance(const char* segment_name_):ref(0)
      {
        std::strcpy(segment_name,segment_name_);
      }

      ~referenced_instance(){segment_name[0]='\0';}

      char         segment_name[128]; /* used to detect dangling pointers */
      mutable long ref;
      C            c;
    }**                                 ppref;
    C*                                  pc;
  };
};

template<typename C>
inline void atomic_functor<C>::operator()()
{
   referenced_instance *pptr = seg.find_or_construct<referenced_instance>(unique_instance)();
};


}  //namespace detail{
}  //namespace interprocess{
}  //namespace boost{

#include <boost/interprocess/detail/config_end.hpp>

#endif
