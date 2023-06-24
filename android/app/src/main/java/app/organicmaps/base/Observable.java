package app.organicmaps.base;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;

public abstract class Observable<T extends DataChangedListener>
{
  @NonNull
  private final List<T> mListeners = new ArrayList<>();

  public void registerListener(@NonNull T listener)
  {
    if (mListeners.contains(listener))
      throw new IllegalStateException("Observer " + listener + " is already registered.");

    mListeners.add(listener);
  }

  public void unregisterListener(@NonNull T listener)
  {
    int index = mListeners.indexOf(listener);
    if (index == -1)
      throw new IllegalStateException("Observer " + listener + " was not registered.");

    mListeners.remove(index);
  }

  protected void notifyChanged()
  {
    for (T item : mListeners)
      item.onChanged();
  }
}
