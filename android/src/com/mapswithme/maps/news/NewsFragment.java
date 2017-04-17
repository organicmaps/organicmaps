package com.mapswithme.maps.news;

import android.support.annotation.NonNull;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.UpdaterDialogFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.concurrency.UiThread;

public class NewsFragment extends BaseNewsFragment
{
  private static final long DONE_DELAY = MwmApplication.get().getResources()
                                                .getInteger(android.R.integer.config_longAnimTime);

  @NonNull
  private final Runnable mDoneTask = new Runnable()
  {
    @Override
    public void run()
    {
      NewsFragment.super.onDoneClick();
    }
  };

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

  @Override
  protected void onDoneClick()
  {
    if (!UpdaterDialogFragment.showOn(getActivity()))
      super.onDoneClick();
    else
      UiThread.runLater(mDoneTask, DONE_DELAY);
  }

  @Override
  public void onDestroy()
  {
    UiThread.cancelDelayedTasks(mDoneTask);
    super.onDestroy();
  }

  /**
   * Displays "What's new" dialog on given {@code activity}. Or not.
   * @return whether "What's new" dialog should be shown.
   */
  public static boolean showOn(@NonNull FragmentActivity activity)
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
