package app.organicmaps.sdk.countryinfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import org.chromium.base.ObserverList;

public final class CurrentCountryInfoManagerImpl implements CurrentCountryInfoManager
{
  @Nullable
  private CountryInfo mCurrentCountryInfo;

  @NonNull
  private final ObserverList<OnCurrentCountryChangedListener> mListeners = new ObserverList<>();

  @Override
  @Nullable
  public CountryInfo getCurrentCountryInfo()
  {
    return mCurrentCountryInfo;
  }

  @Override
  public void addListener(@NonNull OnCurrentCountryChangedListener listener)
  {
    mListeners.addObserver(listener);
  }

  @Override
  public void removeListener(@NonNull OnCurrentCountryChangedListener listener)
  {
    mListeners.removeObserver(listener);
  }

  public void initialize()
  {
    nativeSubscribe(countryInfo -> {
      mCurrentCountryInfo = countryInfo;
      for (OnCurrentCountryChangedListener listener : mListeners)
        listener.onCurrentCountryChanged(countryInfo);
    });
  }

  private static native void nativeSubscribe(@NonNull OnCurrentCountryChangedListener listener);
  // TODO: Is it needed?
  // private static native void nativeUnsubscribe();
}
