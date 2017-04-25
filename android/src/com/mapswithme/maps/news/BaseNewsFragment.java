package com.mapswithme.maps.news;

import android.app.Dialog;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.support.annotation.ArrayRes;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
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

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;

public abstract class BaseNewsFragment extends BaseMwmDialogFragment
{
  private ViewPager mPager;
  private View mPrevButton;
  private View mNextButton;
  private View mDoneButton;
  private ImageView[] mDots;

  private int mPageCount;

  @Nullable
  private NewsDialogListener mListener;

  abstract class Adapter extends PagerAdapter
  {
    private final int[] mImages;
    private final String[] mTitles;
    private final String[] mSubtitles;
    private final String[] mSwitchTitles;
    private final String[] mSwitchSubtitles;

    Adapter()
    {
      Resources res = MwmApplication.get().getResources();

      mTitles = res.getStringArray(getTitles());
      mSubtitles = res.getStringArray(getSubtitles1());

      int subtitles2 = getSubtitles2();
      if (subtitles2 != 0)
      {
        String[] strings = res.getStringArray(subtitles2);
        for (int i = 0; i < mSubtitles.length; i++)
        {
          String s = strings[i];
          if (!TextUtils.isEmpty(s))
            mSubtitles[i] += "\n\n" + s;
        }
      }

      mSwitchTitles = res.getStringArray(getSwitchTitles());
      mSwitchSubtitles = res.getStringArray(getSwitchSubtitles());

      TypedArray images = res.obtainTypedArray(getImages());
      mImages = new int[images.length()];
      for (int i = 0; i < mImages.length; i++)
        mImages[i] = images.getResourceId(i, 0);

      images.recycle();
    }

    abstract @ArrayRes int getTitles();
    abstract @ArrayRes int getSubtitles1();
    abstract @ArrayRes int getSubtitles2();
    abstract @ArrayRes int getSwitchTitles();
    abstract @ArrayRes int getSwitchSubtitles();
    abstract @ArrayRes int getImages();

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
      View res = LayoutInflater.from(container.getContext()).inflate(R.layout.news_page, container, false);

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
            onSwitchChanged(position, isChecked);
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

  void onSwitchChanged(int index, boolean isChecked) {}

  private void update()
  {
    int cur = mPager.getCurrentItem();
    UiUtils.showIf(cur > 0, mPrevButton);
    UiUtils.showIf(cur + 1 < mPageCount, mNextButton);
    UiUtils.visibleIf(cur + 1 == mPageCount, mDoneButton);

    if (mPageCount == 1)
      return;

    for (int i = 0; i < mPageCount; i++)
    {
      mDots[i].setImageResource(ThemeUtils.isNightTheme() ? i == cur ? R.drawable.news_marker_active_night
                                                                     : R.drawable.news_marker_inactive_night
                                                          : i == cur ? R.drawable.news_marker_active
                                                                     : R.drawable.news_marker_inactive);
    }
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

  abstract Adapter createAdapter();

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

    View content = View.inflate(getActivity(), R.layout.fragment_news, null);
    res.setContentView(content);

    mPager = (ViewPager)content.findViewById(R.id.pager);
    fixPagerSize();

    Adapter adapter = createAdapter();
    mPageCount = adapter.getCount();
    mPager.setAdapter(adapter);
    mPager.addOnPageChangeListener(new ViewPager.SimpleOnPageChangeListener()
    {
      @Override
      public void onPageSelected(int position)
      {
        update();
      }
    });

    ViewGroup dots = (ViewGroup)content.findViewById(R.id.dots);
    if (mPageCount == 1)
      UiUtils.hide(dots);
    else
    {
      int dotCount = dots.getChildCount();
      mDots = new ImageView[mPageCount];
      for (int i = 0; i < dotCount; i++)
      {
        ImageView dot = (ImageView)dots.getChildAt(i);

        if (i < (dotCount - mPageCount))
          UiUtils.hide(dot);
        else
          mDots[i - (dotCount - mPageCount)] = dot;
      }
    }

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
        onDoneClick();
      }
    });

    update();
    return res;
  }

  @CallSuper
  protected void onDoneClick()
  {
    dismissAllowingStateLoss();
    if (mListener != null)
      mListener.onDialogDone();
  }

  @SuppressWarnings("TryWithIdenticalCatches")
  static void create(@NonNull FragmentActivity activity,
                     @NonNull Class<? extends BaseNewsFragment> clazz,
                     @Nullable NewsDialogListener listener)
  {
    try
    {
      final BaseNewsFragment fragment = clazz.newInstance();
      fragment.mListener = listener;
      activity.getSupportFragmentManager()
          .beginTransaction()
          .add(fragment, clazz.getName())
          .commitAllowingStateLoss();
    } catch (java.lang.InstantiationException ignored)
    {}
    catch (IllegalAccessException ignored)
    {}
  }

  static boolean recreate(FragmentActivity activity, Class<? extends BaseNewsFragment> clazz)
  {
    FragmentManager fm = activity.getSupportFragmentManager();
    Fragment f = fm.findFragmentByTag(clazz.getName());
    if (f == null)
      return false;

    // If we're here, it means that the user has rotated the screen.
    // We use different dialog themes for landscape and portrait modes on tablets,
    // so the fragment should be recreated to be displayed correctly.
    fm.beginTransaction().remove(f).commitAllowingStateLoss();
    fm.executePendingTransactions();
    return true;
  }

  public interface NewsDialogListener
  {
    void onDialogDone();
  }
}
