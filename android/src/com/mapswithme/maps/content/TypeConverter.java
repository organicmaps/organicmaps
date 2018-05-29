package com.mapswithme.maps.content;

public interface TypeConverter<D, T>
{
  T convert(D data);
}
