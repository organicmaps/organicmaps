package com.mapswithme.maps;

import android.app.Dialog;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v7.widget.SwitchCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public class NewsFragment extends BaseMwmDialogFragment
{
  private ViewPager mPager;
  private View mPrevButton;
  private View mNextButton;
  private View mDoneButton;
  private ImageView[] mDots;

  private class Adapter extends PagerAdapter
  {
    private final int[] mImages;
    private final String[] mTitles = MwmApplication.get().getResources().getStringArray(R.array.news_titles);
    private final String[] mSubtitles = MwmApplication.get().getResources().getStringArray(R.array.news_subtitles);
    private final String[] mSwitchTitles = MwmApplication.get().getResources().getStringArray(R.array.news_switch_titles);
    private final String[] mSwitchSubtitles = MwmApplication.get().getResources().getStringArray(R.array.news_switch_subtitles);

    Adapter()
    {
      TypedArray images = MwmApplication.get().getResources().obtainTypedArray(R.array.news_images);
      mImages = new int[images.length()];
      for (int i = 0; i < mImages.length; i++)
        mImages[i] = images.getResourceId(i, 0);

      images.recycle();
    }

    @Override
    public int getCount()
    {
      return mImages.length;
    }

    @Override
    public boolean isViewFromObject(View view, Object object)
    {
      return (view == object);
    }

    @Override
    public Object instantiateItem(ViewGroup container, final int position)
    {
      View res = LayoutInflater.from(getActivity()).inflate(R.layout.news_page, container, false);

      ((ImageView)res.findViewById(R.id.image))
        .setImageResource(mImages[position]);

      ((TextView)res.findViewById(R.id.title))
        .setText(mTitles[position]);

      ((TextView)res.findViewById(R.id.subtitle))
        .setText(mSubtitles[position]);

      View switchBlock = res.findViewById(R.id.switch_block);
      String text = mSwitchTitles[position];
      if (TextUtils.isEmpty(text))
        UiUtils.hide(switchBlock);
      else
      {
        ((TextView)switchBlock.findViewById(R.id.switch_title))
          .setText(text);

        TextView subtitle = (TextView)switchBlock.findViewById(R.id.switch_subtitle);
        if (TextUtils.isEmpty(mSwitchSubtitles[position]))
          UiUtils.hide(subtitle);
        else
          subtitle.setText(mSwitchSubtitles[position]);

        final SwitchCompat checkBox = (SwitchCompat)switchBlock.findViewById(R.id.switch_box);
        checkBox.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener()
        {
          @Override
          public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
          {
            // Unused now
          }
        });

        switchBlock.setOnClickListener(new View.OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
            checkBox.performClick();
          }
        });
      }

      container.addView(res);
      return res;
    }

    @Override
    public void destroyItem(ViewGroup container, int position, Object object)
    {
      container.removeView((View)object);
    }
  }

  private void update()
  {
    int cur = mPager.getCurrentItem();
    for (int i = 0; i < mDots.length; i++)
    {
      mDots[i].setImageResource(ThemeUtils.isNightTheme() ? i == cur ? R.drawable.news_marker_active_night
                                                                     : R.drawable.news_marker_inactive_night
                                                          : i == cur ? R.drawable.news_marker_active
                                                                     : R.drawable.news_marker_inactive);
    }

    UiUtils.showIf(cur > 0, mPrevButton);
    UiUtils.showIf(cur + 1 < mDots.length, mNextButton);
    UiUtils.visibleIf(cur + 1 == mDots.length, mDoneButton);
  }

  private void fixPagerSize()
  {
    if (!UiUtils.isTablet())
      return;

    UiUtils.waitLayout(mPager, new ViewTreeObserver.OnGlobalLayoutListener()
    {
      @Override
      public void onGlobalLayout()
      {
        int maxWidth = UiUtils.dimen(R.dimen.news_max_width);
        int maxHeight = UiUtils.dimen(R.dimen.news_max_height);

        if (mPager.getWidth() > maxWidth || mPager.getHeight() > maxHeight)
        {
          mPager.setLayoutParams(new LinearLayout.LayoutParams(Math.min(maxWidth, mPager.getWidth()),
                                                               Math.min(maxHeight, mPager.getHeight())));
        }
      }
    });
  }

  @Override
  protected int getCustomTheme()
  {
    return (UiUtils.isTablet() ? super.getCustomTheme()
                               : getFullscreenTheme());
  }

  @Override
  public @NonNull
  Dialog onCreateDialog(Bundle savedInstanceState)
  {
    Dialog res = super.onCreateDialog(savedInstanceState);
    res.requestWindowFeature(Window.FEATURE_NO_TITLE);

    View content = View.inflate(getActivity(), R.layout.fragment_news, null);
    res.setContentView(content);

    mPager = (ViewPager)content.findViewById(R.id.pager);
    fixPagerSize();
    mPager.setAdapter(new Adapter());
    mPager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener()
    {
      @Override
      public void onPageSelected(int position)
      {
        update();
      }
    });

    ViewGroup dots = (ViewGroup)content.findViewById(R.id.dots);
    mDots = new ImageView[dots.getChildCount()];
    for (int i = 0; i < mDots.length; i++)
      mDots[i] = (ImageView)dots.getChildAt(i);

    mPrevButton = content.findViewById(R.id.back);
    mNextButton = content.findViewById(R.id.next);
    mDoneButton = content.findViewById(R.id.done);

    mPrevButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mPager.setCurrentItem(mPager.getCurrentItem() - 1, true);
      }
    });

    mNextButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        mPager.setCurrentItem(mPager.getCurrentItem() + 1, true);
      }
    });

    mDoneButton.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
      }
    });

    update();
    return res;
  }

  /**
   * Displays "What's new" dialog on given {@code activity}. Or not.
   * @return whether "What's new" dialog should be shown.
   */
  @SuppressWarnings("TryWithIdenticalCatches")
  static boolean showOn(FragmentActivity activity)
  {
    if (Config.getFirstInstallVersion() >= BuildConfig.VERSION_CODE)
      return false;

    String tag = NewsFragment.class.getName();
    FragmentManager fm = activity.getSupportFragmentManager();
    if (fm.isDestroyed())
      return false;

    if (Config.getLastWhatsNewVersion() >= BuildConfig.VERSION_CODE)
    {
      Fragment f = fm.findFragmentByTag(tag);
      if (f == null)
        return false;

      // If we're here, it means that the user has rotated the screen.
      // We use different dialog themes for landscape and portrait modes on tablets,
      // so the fragment should be recreated to be displayed correctly.
      fm.beginTransaction().remove(f).commitAllowingStateLoss();
      fm.executePendingTransactions();
    }

    Config.setWhatsNewShown();

    try
    {
      final NewsFragment fragment = NewsFragment.class.newInstance();
      fragment.show(activity.getSupportFragmentManager(), tag);
    } catch (java.lang.InstantiationException ignored)
    {}
    catch (IllegalAccessException ignored)
    {}

    return true;
  }
}
