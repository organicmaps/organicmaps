#pragma once

#include "std/string.hpp"
#include "std/map.hpp"
#include "std/mutex.hpp"

namespace Settings
{
  template <class T> bool FromString(string const & str, T & outValue);
  template <class T> string ToString(T const & value);

  class StringStorage
  {
    typedef map<string, string> ContainerT;
    ContainerT m_values;

    mutable mutex m_mutex;

    StringStorage();
    void Save() const;

  public:
    static StringStorage & Instance();

    bool GetValue(string const & key, string & outValue) const;
    void SetValue(string const & key, string && value);
    void DeleteKeyAndValue(string const & key);
  };

  /// Retrieve setting
  /// @return false if setting is absent
  template <class ValueT> bool Get(string const & key, ValueT & outValue)
  {
    string strVal;
    return StringStorage::Instance().GetValue(key, strVal)
        && FromString(strVal, outValue);
  }
  /// Automatically saves setting to external file
  template <class ValueT> void Set(string const & key, ValueT const & value)
  {
    StringStorage::Instance().SetValue(key, ToString(value));
  }

  inline void Delete(string const & key)
  {
    StringStorage::Instance().DeleteKeyAndValue(key);
  }

  enum Units { Metric = 0, Foot };

  /// Use this function for running some stuff once according to date.
  /// @param[in]  date  Current date in format yymmdd.
  bool IsFirstLaunchForDate(int date);
}
