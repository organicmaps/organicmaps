package app.organicmaps.sdk.countryinfo;

import androidx.annotation.NonNull;

public interface OnCurrentCountryChangedListener
{
  void onCurrentCountryChanged(@NonNull CountryInfo countryInfo);
}
