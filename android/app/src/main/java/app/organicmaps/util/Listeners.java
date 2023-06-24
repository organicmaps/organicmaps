package app.organicmaps.util;

import androidx.annotation.NonNull;

import java.util.Iterator;
import java.util.LinkedHashSet;
import java.util.Set;

/**
 * `Registrator` pattern implementation which allows to maintain the list of listeners,
 * offers safe adding/removing of listeners during iteration.
 * <br/>{@link #finishIterate()} must be called after iteration is complete.
 */
public final class Listeners<T> implements Iterable<T>
{
  private final Set<T> mListeners = new LinkedHashSet<>();
  private final Set<T> mListenersToAdd = new LinkedHashSet<>();
  private final Set<T> mListenersToRemove = new LinkedHashSet<>();

  private boolean mIterating;

  @Override
  public @NonNull Iterator<T> iterator()
  {
    if (mIterating)
      throw new RuntimeException("finishIterate() must be called before new iteration");

    mIterating = true;
    return mListeners.iterator();
  }

  /**
   * Completes listeners iteration. Must be called after iteration is done.
   */
  public void finishIterate()
  {
    if (!mListenersToRemove.isEmpty())
      mListeners.removeAll(mListenersToRemove);

    if (!mListenersToAdd.isEmpty())
      mListeners.addAll(mListenersToAdd);

    mListenersToAdd.clear();
    mListenersToRemove.clear();
    mIterating = false;
  }

  /**
   * Safely registers new listener. If registered during iteration, new listener will NOT be called before current iteration is complete.
   */
  public void register(T listener)
  {
    if (mIterating)
    {
      mListenersToRemove.remove(listener);
      if (!mListeners.contains(listener))
        mListenersToAdd.add(listener);
    }
    else
      mListeners.add(listener);
  }

  /**
   * Safely unregisters listener. If unregistered during iteration, old listener WILL be called in the current iteration.
   */
  public void unregister(T listener)
  {
    if (mIterating)
    {
      mListenersToAdd.remove(listener);
      if (mListeners.contains(listener))
        mListenersToRemove.add(listener);
    }
    else
      mListeners.remove(listener);
  }

  public int getSize()
  {
    int res = mListeners.size();
    if (mIterating)
    {
      res += mListenersToAdd.size();
      res -= mListenersToRemove.size();
    }

    return res;
  }

  public boolean isEmpty()
  {
    return (getSize() <= 0);
  }
}
