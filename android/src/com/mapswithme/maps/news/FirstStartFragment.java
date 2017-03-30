package com.mapswithme.maps.news;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.util.Counters;

public class FirstStartFragment extends BaseNewsFragment
{
  private class Adapter extends BaseNewsFragment.Adapter
  {
    @Override
    int getTitles()
    {
      return R.array.first_start_titles;
    }

    @Override
    int getSubtitles1()
    {
      return R.array.first_start_subtitles;
    }

    @Override
    int getSubtitles2()
    {
      return 0;
    }

    @Override
    int getSwitchTitles()
    {
      return R.array.first_start_switch_titles;
    }

    @Override
    int getSwitchSubtitles()
    {
      return R.array.first_start_switch_subtitles;
    }

    @Override
    int getImages()
    {
      return R.array.first_start_images;
    }
  }

  @Override
  BaseNewsFragment.Adapter createAdapter()
  {
    return new Adapter();
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState)
  {
    LocationHelper.INSTANCE.onEnteredIntoFirstRun();
    return super.onCreateDialog(savedInstanceState);
  }

  @Override
  protected void onDoneClick()
  {
    super.onDoneClick();
    LocationHelper.INSTANCE.onExitFromFirstRun();
  }

  public static boolean showOn(@NonNull FragmentActivity activity,
                               @Nullable NewsDialogListener listener)
  {
    if (Counters.getFirstInstallVersion() < BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Counters.isFirstStartDialogSeen() &&
        !recreate(activity, FirstStartFragment.class))
      return false;

    create(activity, FirstStartFragment.class, listener);

    Counters.setFirstStartDialogSeen();
    return true;
  }
}
