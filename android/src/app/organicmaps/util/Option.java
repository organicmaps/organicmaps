package app.organicmaps.util;

public class Option<T>
{
  private final T mValue;

  public static<U> Option<U> empty()
  {
    return new Option<>(null);
  }

  public Option(T value)
  {
    mValue = value;
  }

  public boolean hasValue()
  {
    return mValue != null;
  }

  public T get()
  {
    assert(hasValue());
    return mValue;
  }

  public T getOrElse(T defaultValue)
  {
    return hasValue() ? mValue : defaultValue;
  }
}
