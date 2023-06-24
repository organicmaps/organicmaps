package app.organicmaps.base;

import androidx.annotation.NonNull;

public interface Savable<T>
{
  void onSave(@NonNull T outState);
  void onRestore(@NonNull T inState);
}
