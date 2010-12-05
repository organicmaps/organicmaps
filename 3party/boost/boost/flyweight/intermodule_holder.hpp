/* Copyright 2006-2009 Joaquin M Lopez Munoz.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 * See http://www.boost.org/libs/flyweight for library home page.
 */

#ifndef BOOST_FLYWEIGHT_INTERMODULE_HOLDER_HPP
#define BOOST_FLYWEIGHT_INTERMODULE_HOLDER_HPP

#if defined(_MSC_VER)&&(_MSC_VER>=1200)
#pragma once
#endif

#include <boost/config.hpp> /* keep it first to prevent nasty warns in MSVC */
#include <boost/flyweight/holder_tag.hpp>
#include <boost/flyweight/intermodule_holder_fwd.hpp>
#include <boost/flyweight/detail/process_id.hpp>
#include <boost/functional/hash.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/mpl/aux_/lambda_support.hpp>
#include <cstdio>
#include <cstring>
#include <memory>

/* intermodule_holder_class guarantees a unique instance across all dynamic
 * modules of a program.
 */

namespace boost{

namespace flyweights{

template<typename C>
struct intermodule_holder_class:holder_marker
{
  static C& get()
  {
    static instantiator instance;    
    return instance.get();
  }

private:
  struct instantiator
  {
    instantiator():
      mutex(interprocess::open_or_create,compute_mutex_name()),
      seg(interprocess::open_or_create,compute_segment_name(),16384),
      ppref(0),
      pc(0)
    {
      /* Instance creation is done according to a two-phase protocol so
       * that we call "new" in an unlocked situation, thus minimizing the
       * chance of leaving dangling locks due to catastrophic failure.
       */

      {
        interprocess::scoped_lock<interprocess::named_mutex> lock(mutex);
        ppref=seg.find_or_construct<referenced_instance*>(
          typeid(C).name())((referenced_instance*)0);
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
    
    ~instantiator()
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
    
    C& get()const{return *pc;}
    
  private:
    /* Although mutex and seg are system-wide, their names intend to
     * make them specific for the current process and type, hence their
     * containing process id and type id info.
     */
    
    char mutex_name[128];
    char segment_name[128];

    const char* compute_mutex_name()
    {
      std::sprintf(
        mutex_name,
        "boost_flyweight_intermodule_holder_mutex_"
        "%ld_%u_%u",
        (long)detail::process_id(),
        (unsigned)compute_hash(typeid(C).name(),0),
        (unsigned)compute_hash(typeid(C).name(),1));

      return mutex_name;
    }
    
    const char* compute_segment_name()
    {      
      std::sprintf(
        segment_name,
        "boost_flyweight_intermodule_holder_segment_"
        "%ld_%u_%u",
        (long)detail::process_id(),
        (unsigned)compute_hash(typeid(C).name(),0),
        (unsigned)compute_hash(typeid(C).name(),1));

      return segment_name;
    }
    
    static std::size_t compute_hash(const char* str,std::size_t off)
    {
      std::size_t len=std::strlen(str);
      if(off>len)off=len;
      return hash_range(str+off,str+len);
    }
    
    interprocess::named_mutex           mutex;
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

public:
  typedef intermodule_holder_class type;
  BOOST_MPL_AUX_LAMBDA_SUPPORT(1,intermodule_holder_class,(C))
};

/* intermodule_holder_class specifier */

struct intermodule_holder:holder_marker
{
  template<typename C>
  struct apply
  {
    typedef intermodule_holder_class<C> type;
  };
};

} /* namespace flyweights */

} /* namespace boost */

#endif
