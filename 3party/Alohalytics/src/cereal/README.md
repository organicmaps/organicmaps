cereal - A C++11 library for serialization
==========================================

<img src="http://uscilab.github.io/cereal/assets/img/cerealboxside.png" align="right"/><p>cereal is a header-only C++11 serialization library.  cereal takes arbitrary data types and reversibly turns them into different representations, such as compact binary encodings, XML, or JSON.  cereal was designed to be fast, light-weight, and easy to extend - it has no external dependencies and can be easily bundled with other code or used standalone.</p>

### cereal has great documentation

Looking for more information on how cereal works and its documentation?  Visit [cereal's web page](http://USCiLab.github.com/cereal) to get the latest information.

### cereal is easy to use

Installation and use of of cereal is fully documented on the [main web page](http://USCiLab.github.com/cereal), but this is a quick and dirty version:

* Download cereal and place the headers somewhere your code can see them
* Write serialization functions for your custom types or use the built in support for the standard library cereal provides
* Use the serialization archives to load and save data

```cpp
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/archives/binary.hpp>
#include <fstream>

struct MyRecord
{
  uint8_t x, y;
  float z;

  template <class Archive>
  void serialize( Archive & ar )
  {
    ar( x, y, z );
  }
};

struct SomeData
{
  int32_t id;
  std::shared_ptr<std::unordered_map<uint32_t, MyRecord>> data;

  template <class Archive>
  void save( Archive & ar ) const
  {
    ar( data );
  }

  template <class Archive>
  void load( Archive & ar )
  {
    static int32_t idGen = 0;
    id = idGen++;
    ar( data );
  }
};

int main()
{
  std::ofstream os("out.cereal", std::ios::binary);
  cereal::BinaryOutputArchive archive( os );

  SomeData myData;
  archive( myData );

  return 0;
}
```

### cereal has a mailing list

Either get in touch over <a href="mailto:cerealcpp@googlegroups.com">email</a> or [on the web](https://groups.google.com/forum/#!forum/cerealcpp).



## cereal has a permissive license

cereal is licensed under the [BSD license](http://opensource.org/licenses/BSD-3-Clause).

## cereal build status

* develop : [![Build Status](https://travis-ci.org/USCiLab/cereal.png?branch=develop)](https://travis-ci.org/USCiLab/cereal)

---

Were you looking for the Haskell cereal?  Go <a href="https://github.com/GaloisInc/cereal">here</a>.
