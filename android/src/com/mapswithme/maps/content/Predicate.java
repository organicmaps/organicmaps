package com.mapswithme.maps.content;

import android.support.annotation.NonNull;

public abstract class Predicate<T, D>
{
  @NonNull
  private final T mBaseValue;

  protected Predicate(@NonNull T baseValue)
  {
    mBaseValue = baseValue;
  }

  @NonNull
  protected T getBaseValue()
  {
    return mBaseValue;
  }

  public abstract boolean apply(D field);

  public static class Equals<T, D> extends Predicate<T, D> {

    @NonNull
    private final TypeConverter<D, T> mConverter;

    public Equals(@NonNull TypeConverter<D, T> converter, @NonNull T data)
    {
      super(data);
      mConverter = converter;
    }

    @Override
    public boolean apply(D field)
    {
      T converted = mConverter.convert(field);
      T value = getBaseValue();
      return value == converted || value.equals(converted);

    }
  }
}
