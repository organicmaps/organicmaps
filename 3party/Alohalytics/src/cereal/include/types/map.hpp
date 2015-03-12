/*! \file map.hpp
    \brief Support for types found in \<map\>
    \ingroup STLSupport */
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
#ifndef CEREAL_TYPES_MAP_HPP_
#define CEREAL_TYPES_MAP_HPP_

#include "../cereal.hpp"
#include <map>

namespace cereal
{
  namespace map_detail
  {
    //! @internal
    template <class Archive, class MapT> inline
    void save( Archive & ar, MapT const & map )
    {
      ar( make_size_tag( static_cast<size_type>(map.size()) ) );

      for( const auto & i : map )
      {
        ar( make_map_item(i.first, i.second) );
      }
    }

    //! @internal
    template <class Archive, class MapT> inline
    void load( Archive & ar, MapT & map )
    {
      size_type size;
      ar( make_size_tag( size ) );

      map.clear();

      auto hint = map.begin();
      for( size_t i = 0; i < size; ++i )
      {
        typename MapT::key_type key;
        typename MapT::mapped_type value;

        ar( make_map_item(key, value) );
        #ifdef CEREAL_OLDER_GCC
        hint = map.insert( hint, std::make_pair(std::move(key), std::move(value)) );
        #else // NOT CEREAL_OLDER_GCC
        hint = map.emplace_hint( hint, std::move( key ), std::move( value ) );
        #endif // NOT CEREAL_OLDER_GCC
      }
    }
  }

  //! Saving for std::map
  template <class Archive, class K, class T, class C, class A> inline
  void CEREAL_SAVE_FUNCTION_NAME( Archive & ar, std::map<K, T, C, A> const & map )
  {
    map_detail::save( ar, map );
  }

  //! Loading for std::map
  template <class Archive, class K, class T, class C, class A> inline
  void CEREAL_LOAD_FUNCTION_NAME( Archive & ar, std::map<K, T, C, A> & map )
  {
    map_detail::load( ar, map );
  }

  //! Saving for std::multimap
  /*! @note serialization for this type is not guaranteed to preserve ordering */
  template <class Archive, class K, class T, class C, class A> inline
  void CEREAL_SAVE_FUNCTION_NAME( Archive & ar, std::multimap<K, T, C, A> const & multimap )
  {
    map_detail::save( ar, multimap );
  }

  //! Loading for std::multimap
  /*! @note serialization for this type is not guaranteed to preserve ordering */
  template <class Archive, class K, class T, class C, class A> inline
  void CEREAL_LOAD_FUNCTION_NAME( Archive & ar, std::multimap<K, T, C, A> & multimap )
  {
    map_detail::load( ar, multimap );
  }
} // namespace cereal

#endif // CEREAL_TYPES_MAP_HPP_
