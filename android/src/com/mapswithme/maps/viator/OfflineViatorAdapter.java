package com.mapswithme.maps.viator;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;

public class OfflineViatorAdapter extends ViatorAdapter
{
  public OfflineViatorAdapter(@Nullable ItemSelectedListener listener)
  {
    super(null, false, listener, TYPE_OFFLINE_MESSAGE);
  }

  @NonNull
  @Override
  protected String getLoadingTitle()
  {
    return MwmApplication
        .get().getString(R.string.preloader_viator_title);
  }

  @Nullable
  @Override
  protected String getLoadingSubtitle()
  {
    return MwmApplication
        .get().getString(R.string.common_check_internet_connection_dialog);
  }
}
