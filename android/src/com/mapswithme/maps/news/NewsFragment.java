package com.mapswithme.maps.news;

import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.util.Config;

public class NewsFragment extends BaseNewsFragment
{
  private class Adapter extends BaseNewsFragment.Adapter
  {
    @Override
    int getTitles()
    {
      return R.array.news_titles;
    }

    @Override
    int getSubtitles1()
    {
      return R.array.news_messages_1;
    }

    @Override
    int getSubtitles2()
    {
      return R.array.news_messages_2;
    }

    @Override
    int getSwitchTitles()
    {
      return R.array.news_switch_titles;
    }

    @Override
    int getSwitchSubtitles()
    {
      return R.array.news_switch_subtitles;
    }

    @Override
    int getImages()
    {
      return R.array.news_images;
    }
  }

  @Override
  BaseNewsFragment.Adapter createAdapter()
  {
    return new Adapter();
  }

  /**
   * Displays "What's new" dialog on given {@code activity}. Or not.
   * @return whether "What's new" dialog should be shown.
   */
  public static boolean showOn(FragmentActivity activity)
  {
    if (Config.getFirstInstallVersion() >= BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Config.getLastWhatsNewVersion() / 10 >= BuildConfig.VERSION_CODE / 10 &&
        !recreate(activity, NewsFragment.class))
      return false;

    create(activity, NewsFragment.class);

    Config.setWhatsNewShown();
    return true;
  }
}
