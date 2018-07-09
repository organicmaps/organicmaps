package com.mapswithme.maps.news;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.R;
import com.mapswithme.maps.downloader.UpdaterDialogFragment;
import com.mapswithme.util.Counters;
import com.mapswithme.util.SharedPropertiesUtils;

public class NewsFragment extends BaseNewsFragment
{

  private class Adapter extends BaseNewsFragment.Adapter
  {
    @Override
    int getTitleKeys()
    {
      return TITLE_KEYS;
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
    int getButtonLabels()
    {
      return R.array.news_button_labels;
    }

    @Override
    int getButtonLinks()
    {
      return R.array.news_button_links;
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
    if (!UpdaterDialogFragment.showOn(getActivity(), getListener()))
      super.onDoneClick();
    else
      dismissAllowingStateLoss();
  }

  /**
   * Displays "What's new" dialog on given {@code activity}. Or not.
   * @return whether "What's new" dialog should be shown.
   */
  public static boolean showOn(@NonNull FragmentActivity activity,
                               final @Nullable NewsDialogListener listener)
  {
    if (Counters.getFirstInstallVersion() >= BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    Fragment f = fm.findFragmentByTag(UpdaterDialogFragment.class.getName());
    if (f != null)
      return UpdaterDialogFragment.showOn(activity, listener);

    String currentTitle = getCurrentTitleConcatenation(activity.getApplicationContext());
    String oldTitle = SharedPropertiesUtils.getWhatsNewTitleConcatenation();
    if (currentTitle.equals(oldTitle) && !recreate(activity, NewsFragment.class))
      return false;

    create(activity, NewsFragment.class, listener);

    Counters.setWhatsNewShown();
    SharedPropertiesUtils.setWhatsNewTitleConcatenation(currentTitle);
    Counters.setShowReviewForOldUser(true);

    return true;
  }

  @NonNull
  private static String getCurrentTitleConcatenation(@NonNull Context context)
  {
    String[] keys = context.getResources().getStringArray(TITLE_KEYS);
    final int length = keys.length;
    if (length == 0)
      return "";

    StringBuilder sb = new StringBuilder("");
    for (String key : keys)
      sb.append(key);

    return sb.toString().trim();
  }
}
