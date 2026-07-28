package app.organicmaps.sdk.location;

import androidx.annotation.NonNull;
import java.util.Objects;
import org.chromium.base.ObserverList;

public final class CurrentCountryManager
{
  public interface OnCountryChangedListener
  {
    void onCountryChanged(String countryId);
  }

  @NonNull
  private final ObserverList<OnCountryChangedListener> mListeners = new ObserverList<>();
  @NonNull
  private final OnCountryChangedListener mListener = this::notifyListeners;

  public void addListener(@NonNull OnCountryChangedListener listener)
  {
    mListeners.addObserver(Objects.requireNonNull(listener));

    if (mListeners.size() == 1)
      nativeSetListener(mListener);
  }

  public void removeListener(@NonNull OnCountryChangedListener listener)
  {
    mListeners.removeObserver(Objects.requireNonNull(listener));

    if (mListeners.isEmpty())
      nativeRemoveListener();
  }

  private void notifyListeners(@NonNull String countryId)
  {
    for (final OnCountryChangedListener listener : mListeners)
    {
      listener.onCountryChanged(countryId);
    }
  }

  private static native void nativeSetListener(@NonNull OnCountryChangedListener listener);
  private static native void nativeRemoveListener();
}
