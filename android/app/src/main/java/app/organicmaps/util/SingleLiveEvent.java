package app.organicmaps.util;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * A {@link MutableLiveData} that delivers each value to its observer exactly once. A value set before an
 * observer (re)subscribes is not re-delivered, so one-shot signals (e.g. "show an error dialog") are not
 * replayed to a freshly created observer after a configuration change. Intended for a single observer.
 */
public class SingleLiveEvent<T> extends MutableLiveData<T>
{
  private final AtomicBoolean mPending = new AtomicBoolean(false);

  @MainThread
  @Override
  public void observe(@NonNull LifecycleOwner owner, @NonNull Observer<? super T> observer)
  {
    super.observe(owner, t -> {
      if (mPending.compareAndSet(true, false))
        observer.onChanged(t);
    });
  }

  @MainThread
  @Override
  public void setValue(@Nullable T value)
  {
    mPending.set(true);
    super.setValue(value);
  }

  /** Fire the event for {@code Void} payloads. */
  @MainThread
  public void call()
  {
    setValue(null);
  }
}
