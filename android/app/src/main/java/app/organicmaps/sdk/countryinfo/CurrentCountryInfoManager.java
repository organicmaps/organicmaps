package app.organicmaps.sdk.countryinfo;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

public interface CurrentCountryInfoManager
{
  @Nullable
  CountryInfo getCurrentCountryInfo();

  void addListener(@NonNull OnCurrentCountryChangedListener listener);

  void removeListener(@NonNull OnCurrentCountryChangedListener listener);
}
