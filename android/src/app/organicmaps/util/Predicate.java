package app.organicmaps.util;

import androidx.annotation.NonNull;

public abstract class Predicate<T, D>
{
  @NonNull
  private final T mBaseValue;

  Predicate(@NonNull T baseValue)
  {
    mBaseValue = baseValue;
  }

  @NonNull
  T getBaseValue()
  {
    return mBaseValue;
  }

  public abstract boolean apply(@NonNull D field);

  public static class Equals<T, D> extends Predicate<T, D>
  {
    @NonNull
    private final TypeConverter<D, T> mConverter;

    public Equals(@NonNull TypeConverter<D, T> converter, @NonNull T data)
    {
      super(data);
      mConverter = converter;
    }

    @Override
    public boolean apply(@NonNull D field)
    {
      T converted = mConverter.convert(field);
      T value = getBaseValue();
      return value == converted || value.equals(converted);
    }
  }
}
