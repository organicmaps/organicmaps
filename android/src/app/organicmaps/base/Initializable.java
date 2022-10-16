package app.organicmaps.base;

import androidx.annotation.Nullable;

public interface Initializable<T>
{
  void initialize(@Nullable T t);
  void destroy();
}
