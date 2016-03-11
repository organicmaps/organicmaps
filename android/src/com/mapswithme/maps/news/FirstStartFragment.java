package com.mapswithme.maps.news;

import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.util.Config;

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
    int getSubtitles()
    {
      return R.array.first_start_subtitles;
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

  public static boolean showOn(FragmentActivity activity)
  {
    if (Config.getFirstInstallVersion() < BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Config.isFirstStartDialogSeen() &&
        !recreate(activity, FirstStartFragment.class))
      return false;

    create(activity, FirstStartFragment.class);

    Config.setFirstStartDialogSeen();
    return true;
  }
}
