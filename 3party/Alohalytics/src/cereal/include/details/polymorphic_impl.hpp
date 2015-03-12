/*! \file polymorphic_impl.hpp
    \brief Internal polymorphism support
    \ingroup Internal */
/*
  Copyright (c) 2014, Randolph Voorhies, Shane Grant
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
      * Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
      * Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
      * Neither the name of cereal nor the
        names of its contributors may be used to endorse or promote products
        derived from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL RANDOLPH VOORHIES OR SHANE GRANT BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* This code is heavily inspired by the boost serialization implementation by the following authors

   (C) Copyright 2002 Robert Ramey - http://www.rrsd.com .
   Use, modification and distribution is subject to the Boost Software
   License, Version 1.0. (See http://www.boost.org/LICENSE_1_0.txt)

    See http://www.boost.org for updates, documentation, and revision history.

   (C) Copyright 2006 David Abrahams - http://www.boost.org.

   See /boost/serialization/export.hpp and /boost/archive/detail/register_archive.hpp for their
   implementation.
*/
#ifndef CEREAL_DETAILS_POLYMORPHIC_IMPL_HPP_
#define CEREAL_DETAILS_POLYMORPHIC_IMPL_HPP_

#include "../details/static_object.hpp"
#include "../types/memory.hpp"
#include "../types/string.hpp"
#include <functional>
#include <typeindex>
#include <map>

//! Binds a polymorhic type to all registered archives
/*! This binds a polymorphic type to all compatible registered archives that
    have been registered with CEREAL_REGISTER_ARCHIVE.  This must be called
    after all archives are registered (usually after the archives themselves
    have been included). */
#define CEREAL_BIND_TO_ARCHIVES(T)                   \
    namespace cereal {                               \
    namespace detail {                               \
    template<>                                       \
    struct init_binding<T> {                         \
        static bind_to_archives<T> const & b;        \
        static void unused() { (void)b; }            \
    };                                               \
    bind_to_archives<T> const & init_binding<T>::b = \
        ::cereal::detail::StaticObject<              \
            bind_to_archives<T>                      \
        >::getInstance().bind();                     \
    }} /* end namespaces */

namespace cereal
{
  namespace detail
  {
    //! Binds a compile time type with a user defined string
    template <class T>
    struct binding_name {};

    //! A structure holding a map from type_indices to output serializer functions
    /*! A static object of this map should be created for each registered archive
        type, containing entries for every registered type that describe how to
        properly cast the type to its real type in polymorphic scenarios for
        shared_ptr, weak_ptr, and unique_ptr. */
    template <class Archive>
    struct OutputBindingMap
    {
      //! A serializer function
      /*! Serializer functions return nothing and take an archive as
          their first parameter (will be cast properly inside the function,
          and a pointer to actual data (contents of smart_ptr's get() function)
          as their second parameter */
      typedef std::function<void(void*, void const *)> Serializer;

      //! Struct containing the serializer functions for all pointer types
      struct Serializers
      {
        Serializer shared_ptr, //!< Serializer function for shared/weak pointers
                   unique_ptr; //!< Serializer function for unique pointers
      };

      //! A map of serializers for pointers of all registered types
      std::map<std::type_index, Serializers> map;
    };

    //! An empty noop deleter
    template<class T> struct EmptyDeleter { void operator()(T *) const {} };

    //! A structure holding a map from type name strings to input serializer functions
    /*! A static object of this map should be created for each registered archive
        type, containing entries for every registered type that describe how to
        properly cast the type to its real type in polymorphic scenarios for
        shared_ptr, weak_ptr, and unique_ptr. */
    template <class Archive>
    struct InputBindingMap
    {
      //! Shared ptr serializer function
      /*! Serializer functions return nothing and take an archive as
          their first parameter (will be cast properly inside the function,
          and a shared_ptr (or unique_ptr for the unique case) of any base
          type.  Internally it will properly be loaded and cast to the
          correct type. */
      typedef std::function<void(void*, std::shared_ptr<void> & )> SharedSerializer;
      //! Unique ptr serializer function
      typedef std::function<void(void*, std::unique_ptr<void, EmptyDeleter<void>> & )> UniqueSerializer;

      //! Struct containing the serializer functions for all pointer types
      struct Serializers
      {
        SharedSerializer shared_ptr; //!< Serializer function for shared/weak pointers
        UniqueSerializer unique_ptr; //!< Serializer function for unique pointers
      };

      //! A map of serializers for pointers of all registered types
      std::map<std::string, Serializers> map;
    };

    // forward decls for archives from cereal.hpp
    class InputArchiveBase;
    class OutputArchiveBase;

    //! Creates a binding (map entry) between an input archive type and a polymorphic type
    /*! Bindings are made when types are registered, assuming that at least one
        archive has already been registered.  When this struct is created,
        it will insert (at run time) an entry into a map that properly handles
        casting for serializing polymorphic objects */
    template <class Archive, class T> struct InputBindingCreator
    {
      //! Initialize the binding
      InputBindingCreator()
      {
        auto & map = StaticObject<InputBindingMap<Archive>>::getInstance().map;
        auto key = std::string(binding_name<T>::name());
        auto lb = map.lower_bound(key);

        if (lb != map.end() && lb->first == key)
          return;

        typename InputBindingMap<Archive>::Serializers serializers;

        serializers.shared_ptr =
          [](void * arptr, std::shared_ptr<void> & dptr)
          {
            Archive & ar = *static_cast<Archive*>(arptr);
            std::shared_ptr<T> ptr;

            ar( CEREAL_NVP_("ptr_wrapper", ::cereal::memory_detail::make_ptr_wrapper(ptr)) );

            dptr = ptr;
          };

        serializers.unique_ptr =
          [](void * arptr, std::unique_ptr<void, EmptyDeleter<void>> & dptr)
          {
            Archive & ar = *static_cast<Archive*>(arptr);
            std::unique_ptr<T> ptr;

            ar( CEREAL_NVP_("ptr_wrapper", ::cereal::memory_detail::make_ptr_wrapper(ptr)) );

            dptr.reset(ptr.release());
          };

        map.insert( lb, { std::move(key), std::move(serializers) } );
      }
    };

    //! Creates a binding (map entry) between an output archive type and a polymorphic type
    /*! Bindings are made when types are registered, assuming that at least one
        archive has already been registered.  When this struct is created,
        it will insert (at run time) an entry into a map that properly handles
        casting for serializing polymorphic objects */
    template <class Archive, class T> struct OutputBindingCreator
    {
      //! Writes appropriate metadata to the archive for this polymorphic type
      static void writeMetadata(Archive & ar)
      {
        // Register the polymorphic type name with the archive, and get the id
        char const * name = binding_name<T>::name();
        std::uint32_t id = ar.registerPolymorphicType(name);

        // Serialize the id
        ar( CEREAL_NVP_("polymorphic_id", id) );

        // If the msb of the id is 1, then the type name is new, and we should serialize it
        if( id & detail::msb_32bit )
        {
          std::string namestring(name);
          ar( CEREAL_NVP_("polymorphic_name", namestring) );
        }
      }

      //! Holds a properly typed shared_ptr to the polymorphic type
      class PolymorphicSharedPointerWrapper
      {
        public:
          /*! Wrap a raw polymorphic pointer in a shared_ptr to its true type

              The wrapped pointer will not be responsible for ownership of the held pointer
              so it will not attempt to destroy it; instead the refcount of the wrapped
              pointer will be tied to a fake 'ownership pointer' that will do nothing
              when it ultimately goes out of scope.

              The main reason for doing this, other than not to destroy the true object
              with our wrapper pointer, is to avoid meddling with the internal reference
              count in a polymorphic type that inherits from std::enable_shared_from_this.

              @param dptr A void pointer to the contents of the shared_ptr to serialize */
          PolymorphicSharedPointerWrapper( void const * dptr ) : refCount()
          {
            #ifdef _LIBCPP_VERSION
            // libc++ needs this hacky workaround, see http://llvm.org/bugs/show_bug.cgi?id=18843
            wrappedPtr = std::shared_ptr<T const>(
                std::const_pointer_cast<T const>(
                  std::shared_ptr<T>( refCount, static_cast<T *>(const_cast<void *>(dptr) ))));
            #else // NOT _LIBCPP_VERSION
            wrappedPtr = std::shared_ptr<T const>( refCount, static_cast<T const *>(dptr) );
            #endif // _LIBCPP_VERSION
          }

          //! Get the wrapped shared_ptr */
          inline std::shared_ptr<T const> const & operator()() const { return wrappedPtr; }

        private:
          std::shared_ptr<void> refCount;      //!< The ownership pointer
          std::shared_ptr<T const> wrappedPtr; //!< The wrapped pointer
      };

      //! Does the actual work of saving a polymorphic shared_ptr
      /*! This function will properly create a shared_ptr from the void * that is passed in
          before passing it to the archive for serialization.

          In addition, this will also preserve the state of any internal enable_shared_from_this mechanisms

          @param ar The archive to serialize to
          @param dptr Pointer to the actual data held by the shared_ptr */
      static inline void savePolymorphicSharedPtr( Archive & ar, void const * dptr, std::true_type /* has_shared_from_this */ )
      {
        ::cereal::memory_detail::EnableSharedStateHelper<T> state( static_cast<T *>(const_cast<void *>(dptr)) );
        PolymorphicSharedPointerWrapper psptr( dptr );
        ar( CEREAL_NVP_("ptr_wrapper", memory_detail::make_ptr_wrapper( psptr() ) ) );
      }

      //! Does the actual work of saving a polymorphic shared_ptr
      /*! This function will properly create a shared_ptr from the void * that is passed in
          before passing it to the archive for serialization.

          This version is for types that do not inherit from std::enable_shared_from_this.

          @param ar The archive to serialize to
          @param dptr Pointer to the actual data held by the shared_ptr */
      static inline void savePolymorphicSharedPtr( Archive & ar, void const * dptr, std::false_type /* has_shared_from_this */ )
      {
        PolymorphicSharedPointerWrapper psptr( dptr );
        ar( CEREAL_NVP_("ptr_wrapper", memory_detail::make_ptr_wrapper( psptr() ) ) );
      }

      //! Initialize the binding
      OutputBindingCreator()
      {
        auto & map = StaticObject<OutputBindingMap<Archive>>::getInstance().map;
        auto key = std::type_index(typeid(T));
        auto lb = map.lower_bound(key);

        if (lb != map.end() && lb->first == key)
          return;

        typename OutputBindingMap<Archive>::Serializers serializers;

        serializers.shared_ptr =
          [&](void * arptr, void const * dptr)
          {
            Archive & ar = *static_cast<Archive*>(arptr);

            writeMetadata(ar);

            #ifdef _MSC_VER
            savePolymorphicSharedPtr( ar, dptr, ::cereal::traits::has_shared_from_this<T>::type() ); // MSVC doesn't like typename here
            #else // not _MSC_VER
            savePolymorphicSharedPtr( ar, dptr, typename ::cereal::traits::has_shared_from_this<T>::type() );
            #endif // _MSC_VER
          };

        serializers.unique_ptr =
          [&](void * arptr, void const * dptr)
          {
            Archive & ar = *static_cast<Archive*>(arptr);

            writeMetadata(ar);

            std::unique_ptr<T const, EmptyDeleter<T const>> const ptr(static_cast<T const *>(dptr));

            ar( CEREAL_NVP_("ptr_wrapper", memory_detail::make_ptr_wrapper(ptr)) );
          };

        map.insert( { std::move(key), std::move(serializers) } );
      }
    };

    //! Used to help out argument dependent lookup for finding potential overloads
    //! of instantiate_polymorphic_binding
    struct adl_tag {};

    //! Tag for init_binding, bind_to_archives and instantiate_polymorphic_binding. Due to the use of anonymous
    //! namespace it becomes a different type in each translation unit.
    namespace { struct polymorphic_binding_tag {}; }

    //! Causes the static object bindings between an archive type and a serializable type T
    template <class Archive, class T>
    struct create_bindings
    {
      static const InputBindingCreator<Archive, T> &
      load(std::true_type)
      {
        return cereal::detail::StaticObject<InputBindingCreator<Archive, T>>::getInstance();
      }

      static const OutputBindingCreator<Archive, T> &
      save(std::true_type)
      {
        return cereal::detail::StaticObject<OutputBindingCreator<Archive, T>>::getInstance();
      }

      inline static void load(std::false_type) {}
      inline static void save(std::false_type) {}
    };

    //! When specialized, causes the compiler to instantiate its parameter
    template <void(*)()>
    struct instantiate_function {};

    /*! This struct is used as the return type of instantiate_polymorphic_binding
        for specific Archive types.  When the compiler looks for overloads of
        instantiate_polymorphic_binding, it will be forced to instantiate this
        struct during overload resolution, even though it will not be part of a valid
        overload */
    template <class Archive, class T>
    struct polymorphic_serialization_support
    {
      #if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
      //! Creates the appropriate bindings depending on whether the archive supports
      //! saving or loading
      virtual CEREAL_DLL_EXPORT void instantiate() CEREAL_USED;
      #else // NOT _MSC_VER
      //! Creates the appropriate bindings depending on whether the archive supports
      //! saving or loading
      static CEREAL_DLL_EXPORT void instantiate() CEREAL_USED;
      //! This typedef causes the compiler to instantiate this static function
      typedef instantiate_function<instantiate> unused;
      #endif // _MSC_VER
    };

    // instantiate implementation
    template <class Archive, class T>
    CEREAL_DLL_EXPORT void polymorphic_serialization_support<Archive,T>::instantiate()
    {
      create_bindings<Archive,T>::save( std::integral_constant<bool,
                                          std::is_base_of<detail::OutputArchiveBase, Archive>::value &&
                                          traits::is_output_serializable<T, Archive>::value>{} );

      create_bindings<Archive,T>::load( std::integral_constant<bool,
                                          std::is_base_of<detail::InputArchiveBase, Archive>::value &&
                                          traits::is_input_serializable<T, Archive>::value>{} );
    }

    //! Begins the binding process of a type to all registered archives
    /*! Archives need to be registered prior to this struct being instantiated via
        the CEREAL_REGISTER_ARCHIVE macro.  Overload resolution will then force
        several static objects to be made that allow us to bind together all
        registered archive types with the parameter type T. */
    template <class T, class Tag = polymorphic_binding_tag>
    struct bind_to_archives
    {
      //! Binding for non abstract types
      void bind(std::false_type) const
	    {
		    instantiate_polymorphic_binding((T*) 0, 0, Tag{}, adl_tag{});
      }

      //! Binding for abstract types
      void bind(std::true_type) const
      { }

      //! Binds the type T to all registered archives
      /*! If T is abstract, we will not serialize it and thus
          do not need to make a binding */
      bind_to_archives const & bind() const
      {
        static_assert( std::is_polymorphic<T>::value,
                       "Attempting to register non polymorphic type" );
        bind( std::is_abstract<T>() );
        return *this;
      }
    };

    //! Used to hide the static object used to bind T to registered archives
    template <class T, class Tag = polymorphic_binding_tag>
    struct init_binding;

    //! Base case overload for instantiation
    /*! This will end up always being the best overload due to the second
        parameter always being passed as an int.  All other overloads will
        accept pointers to archive types and have lower precedence than int.

        Since the compiler needs to check all possible overloads, the
        other overloads created via CEREAL_REGISTER_ARCHIVE, which will have
        lower precedence due to requring a conversion from int to (Archive*),
        will cause their return types to be instantiated through the static object
        mechanisms even though they are never called.

        See the documentation for the other functions to try and understand this */
    template <class T, typename BindingTag>
    void instantiate_polymorphic_binding( T*, int, BindingTag, adl_tag ) {}
  } // namespace detail
} // namespace cereal

#endif // CEREAL_DETAILS_POLYMORPHIC_IMPL_HPP_
