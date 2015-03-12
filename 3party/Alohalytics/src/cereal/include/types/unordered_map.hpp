/*! \file unordered_map.hpp
    \brief Support for types found in \<unordered_map\>
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
#ifndef CEREAL_TYPES_UNORDERED_MAP_HPP_
#define CEREAL_TYPES_UNORDERED_MAP_HPP_

#include "../cereal.hpp"
#include <unordered_map>

namespace cereal
{
  namespace unordered_map_detail
  {
    //! @internal
    template <class Archive, class MapT> inline
    void save( Archive & ar, MapT const & map )
    {
      ar( make_size_tag( static_cast<size_type>(map.size()) ) );

      for( const auto & i : map )
        ar( make_map_item(i.first, i.second) );
    }

    //! @internal
    template <class Archive, class MapT> inline
    void load( Archive & ar, MapT & map )
    {
      size_type size;
      ar( make_size_tag( size ) );

      map.clear();
      map.reserve( static_cast<std::size_t>( size ) );

      for( size_type i = 0; i < size; ++i )
      {
        typename MapT::key_type key;
        typename MapT::mapped_type value;

        ar( make_map_item(key, value) );
        map.emplace( std::move( key ), std::move( value ) );
      }
    }
  }

  //! Saving for std::unordered_map
  template <class Archive, class K, class T, class H, class KE, class A> inline
  void CEREAL_SAVE_FUNCTION_NAME( Archive & ar, std::unordered_map<K, T, H, KE, A> const & unordered_map )
  {
    unordered_map_detail::save( ar, unordered_map );
  }

  //! Loading for std::unordered_map
  template <class Archive, class K, class T, class H, class KE, class A> inline
  void CEREAL_LOAD_FUNCTION_NAME( Archive & ar, std::unordered_map<K, T, H, KE, A> & unordered_map )
  {
    unordered_map_detail::load( ar, unordered_map );
  }

  //! Saving for std::unordered_multimap
  template <class Archive, class K, class T, class H, class KE, class A> inline
  void CEREAL_SAVE_FUNCTION_NAME( Archive & ar, std::unordered_multimap<K, T, H, KE, A> const & unordered_multimap )
  {
    unordered_map_detail::save( ar, unordered_multimap );
  }

  //! Loading for std::unordered_multimap
  template <class Archive, class K, class T, class H, class KE, class A> inline
  void CEREAL_LOAD_FUNCTION_NAME( Archive & ar, std::unordered_multimap<K, T, H, KE, A> & unordered_multimap )
  {
    unordered_map_detail::load( ar, unordered_multimap );
  }
} // namespace cereal

#endif // CEREAL_TYPES_UNORDERED_MAP_HPP_
