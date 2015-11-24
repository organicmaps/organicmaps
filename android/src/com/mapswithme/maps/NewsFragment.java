package com.mapswithme.maps;

import android.app.Dialog;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v4.app.FragmentActivity;
import android.support.v4.view.PagerAdapter;
import android.support.v4.view.ViewPager;
import android.view.*;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.Config;
import com.mapswithme.util.UiUtils;

public class NewsFragment extends BaseMwmDialogFragment
{
  private ViewPager mPager;
  private TextView mNext;
  private ImageView[] mDots;

  private class Adapter extends PagerAdapter
  {
    private final int[] mImages;
    private final String[] mTitles = MwmApplication.get().getResources().getStringArray(R.array.news_titles);
    private final String[] mSubtitles = MwmApplication.get().getResources().getStringArray(R.array.news_subtitles);

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
    public Object instantiateItem(ViewGroup container, int position)
    {
      View res = LayoutInflater.from(getActivity()).inflate(R.layout.news_page, container, false);

      ((ImageView)res.findViewById(R.id.image))
        .setImageResource(mImages[position]);

      ((TextView)res.findViewById(R.id.title))
        .setText(mTitles[position]);

      ((TextView)res.findViewById(R.id.subtitle))
        .setText(mSubtitles[position]);

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
      mDots[i].setImageResource(i == cur ? R.drawable.news_marker_active
                                         : R.drawable.news_marker_inactive);
    }

    mNext.setText(cur + 1 == mDots.length ? R.string.done
                                          : R.string.whats_new_next);
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
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setStyle(STYLE_NORMAL, UiUtils.isTablet() ? R.style.MwmMain_DialogFragment
                                              : R.style.MwmMain_DialogFragment_Fullscreen);
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

    mNext = (TextView)content.findViewById(R.id.next);
    mNext.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        if (mPager.getCurrentItem() == mDots.length - 1)
          dismiss();
        else
          mPager.setCurrentItem(mPager.getCurrentItem() + 1, true);
      }
    });

    update();
    return res;
  }

  /**
   * Displays "What's new" dialog on given {@code activity}. Or not.
   * @return whether "What's new" dialog to be shown.
   */
  @SuppressWarnings("TryWithIdenticalCatches")
  public static boolean showOn(FragmentActivity activity)
  {
    String tag = NewsFragment.class.getName();
    if (Config.getLastWhatsNewVersion() >= BuildConfig.VERSION_CODE)
      return (activity.getSupportFragmentManager().findFragmentByTag(tag) != null);

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
