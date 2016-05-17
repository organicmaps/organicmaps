package com.mapswithme.maps.news;

import android.app.Dialog;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.view.View;
import android.view.Window;

import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;

// TODO (trashkalmar): This is temporary class
public class SinglePageNewsFragment extends BaseMwmDialogFragment
{
  @Override
  protected int getCustomTheme()
  {
    return (UiUtils.isTablet() ? super.getCustomTheme()
                               : getFullscreenTheme());
  }

  @Override
  public @NonNull Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);

    View content = View.inflate(getActivity(), R.layout.fragment_news_single, null);
    res.setContentView(content);

    View done = content.findViewById(R.id.done);
    if (MapManager.nativeIsLegacyMode())
    {
      UiUtils.hide(done);
      done = content.findViewById(R.id.close);

      content.findViewById(R.id.migrate).setOnClickListener(new View.OnClickListener()
      {
        @Override
        public void onClick(View v)
        {
          ((MwmActivity) getActivity()).showDownloader(false);
          dismissAllowingStateLoss();
        }
      });
    }
    else
    {
      UiUtils.hide(content.findViewById(R.id.migrate_message));
      UiUtils.hide(content.findViewById(R.id.migrate_buttons));
    }

    done.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismissAllowingStateLoss();
      }
    });

    return res;
  }

  @SuppressWarnings("TryWithIdenticalCatches")
  private static void create(FragmentActivity activity)
  {
    try
    {
      SinglePageNewsFragment fragment = SinglePageNewsFragment.class.newInstance();
      fragment.show(activity.getSupportFragmentManager(), SinglePageNewsFragment.class.getName());
    } catch (java.lang.InstantiationException ignored)
    {}
    catch (IllegalAccessException ignored)
    {}
  }

  private static boolean recreate(FragmentActivity activity)
  {
    FragmentManager fm = activity.getSupportFragmentManager();
    Fragment f = fm.findFragmentByTag(SinglePageNewsFragment.class.getName());
    if (f == null)
      return false;

    // If we're here, it means that the user has rotated the screen.
    // We use different dialog themes for landscape and portrait modes on tablets,
    // so the fragment should be recreated to be displayed correctly.
    fm.beginTransaction().remove(f).commitAllowingStateLoss();
    fm.executePendingTransactions();
    return true;
  }

  public static boolean showOn(FragmentActivity activity)
  {
    if (Config.getFirstInstallVersion() >= BuildConfig.VERSION_CODE)
      return false;

    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Config.getLastWhatsNewVersion() / 10 >= BuildConfig.VERSION_CODE / 10 &&
        !recreate(activity))
      return false;

    create(activity);

    Config.setWhatsNewShown();
    return true;
  }

}
