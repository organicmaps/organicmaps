/*! \file tuple.hpp
    \brief Support for types found in \<tuple\>
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
#ifndef CEREAL_TYPES_TUPLE_HPP_
#define CEREAL_TYPES_TUPLE_HPP_

#include "../cereal.hpp"
#include <tuple>

namespace cereal
{
  namespace tuple_detail
  {
    // unwinds a tuple to save it
    //! @internal
    template <size_t Height>
    struct serialize
    {
      template <class Archive, class ... Types> inline
      static void apply( Archive & ar, std::tuple<Types...> & tuple )
      {
        ar( _CEREAL_NVP("tuple_element", std::get<Height - 1>( tuple )) );
        serialize<Height - 1>::template apply( ar, tuple );
      }
    };

    // Zero height specialization - nothing to do here
    //! @internal
    template <>
    struct serialize<0>
    {
      template <class Archive, class ... Types> inline
      static void apply( Archive &, std::tuple<Types...> & )
      { }
    };
  }

  //! Serializing for std::tuple
  template <class Archive, class ... Types> inline
  void serialize( Archive & ar, std::tuple<Types...> & tuple )
  {
    tuple_detail::serialize<std::tuple_size<std::tuple<Types...>>::value>::template apply( ar, tuple );
  }
} // namespace cereal

#endif // CEREAL_TYPES_TUPLE_HPP_
