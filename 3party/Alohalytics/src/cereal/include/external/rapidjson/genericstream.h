// Generic*Stream code from https://code.google.com/p/rapidjson/issues/detail?id=20
#ifndef RAPIDJSON_GENERICSTREAM_H_
#define RAPIDJSON_GENERICSTREAM_H_

#include "rapidjson.h"
#include <iostream>

namespace rapidjson {

  //! Wrapper of std::istream for input.
  class GenericReadStream {
    public:
      typedef char Ch;    //!< Character type (byte).

      //! Constructor.
      /*!
        \param is Input stream.
        */
      GenericReadStream(std::istream & is) : is_(&is) {
      }


      Ch Peek() const {
        if(is_->eof()) return '\0';
        return static_cast<char>(is_->peek());
      }

      Ch Take() {
        if(is_->eof()) return '\0';
        return static_cast<char>(is_->get());
      }

      size_t Tell() const {
        return (int)is_->tellg();
      }

      // Not implemented
      void Put(Ch)       { RAPIDJSON_ASSERT(false); }
      void Flush()       { RAPIDJSON_ASSERT(false); }
      Ch* PutBegin()     { RAPIDJSON_ASSERT(false); return 0; }
      size_t PutEnd(Ch*) { RAPIDJSON_ASSERT(false); return 0; }

      std::istream * is_;
  };


  //! Wrapper of std::ostream for output.
  class GenericWriteStream {
    public:
      typedef char Ch;    //!< Character type. Only support char.

      //! Constructor
      /*!
        \param os Output stream.
        */
      GenericWriteStream(std::ostream& os) : os_(os) {
      }

      void Put(char c) {
        os_.put(c);
      }

      void PutN(char c, size_t n) {
        for (size_t i = 0; i < n; ++i) {
          Put(c);
        }
      }

      void Flush() {
        os_.flush();
      }

      size_t Tell() const {
        return (int)os_.tellp();
      }

      // Not implemented
      char Peek() const    { RAPIDJSON_ASSERT(false); }
      char Take()          { RAPIDJSON_ASSERT(false); }
      char* PutBegin()     { RAPIDJSON_ASSERT(false); return 0; }
      size_t PutEnd(char*) { RAPIDJSON_ASSERT(false); return 0; }

    private:
      std::ostream& os_;
  };

  template<>
    inline void PutN(GenericWriteStream& stream, char c, size_t n) {
      stream.PutN(c, n);
    }

} // namespace rapidjson

#endif // RAPIDJSON_GENERICSTREAM_H_
