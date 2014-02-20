package com.mapswithme.util;

public class Option<T>
{
  private final T mValue;
  private final T mOption;

  public Option(T useIfValueIsNull, T value)
  {
    mValue = value;
    mOption = useIfValueIsNull;
  }

  public boolean hasValue()
  {
    return mValue != null;
  }

  public T get()
  {
    return hasValue() ? mValue : mOption;
  }
}
