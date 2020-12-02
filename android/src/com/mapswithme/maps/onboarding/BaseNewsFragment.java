package com.mapswithme.maps.onboarding;

import android.app.Dialog;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.ArrayRes;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.appcompat.widget.SwitchCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.viewpager.widget.PagerAdapter;
import androidx.viewpager.widget.ViewPager;
import com.mapswithme.maps.BuildConfig;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.List;

public abstract class BaseNewsFragment extends BaseMwmDialogFragment
{
  @ArrayRes
  static final int TITLE_KEYS = R.array.news_title_keys;
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
    @NonNull
    private final int[] mImages;
    @NonNull
    private final String[] mTitles;
    @NonNull
    private final String[] mSubtitles;
    @NonNull
    private final List<PromoButton> mPromoButtons;
    @NonNull
    private final String[] mSwitchTitles;
    @NonNull
    private final String[] mSwitchSubtitles;

    Adapter()
    {
      Resources res = MwmApplication.from(requireContext()).getResources();

      mTitles = getTitles(res);
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

      mPromoButtons = getPromoButtons(res);
      mSwitchTitles = res.getStringArray(getSwitchTitles());
      mSwitchSubtitles = res.getStringArray(getSwitchSubtitles());

      TypedArray images = res.obtainTypedArray(getImages());
      mImages = new int[images.length()];
      for (int i = 0; i < mImages.length; i++)
        mImages[i] = images.getResourceId(i, 0);

      images.recycle();
    }

    @NonNull
    private String[] getTitles(@NonNull Resources res)
    {
      String[] keys = res.getStringArray(getTitleKeys());
      final int length = keys.length;
      if (length == 0)
        throw new AssertionError("Title keys must me non-empty!");

      String[] titles = new String[length];
      for (int i = 0; i < length; i++)
        titles[i] = Utils.getStringValueByKey(getContext(), keys[i]);

      return titles;
    }

    @ArrayRes
    abstract int getTitleKeys();
    @ArrayRes
    abstract int getSubtitles1();
    @ArrayRes
    abstract int getSubtitles2();
    @ArrayRes
    abstract int getButtonLabels();
    @ArrayRes
    abstract int getButtonLinks();
    @ArrayRes
    abstract int getSwitchTitles();
    @ArrayRes
    abstract int getSwitchSubtitles();
    @ArrayRes
    abstract int getImages();

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

      processSwitchBlock(position, res);
      processButton(position, res);

      container.addView(res);
      return res;
    }

    @NonNull
    private List<PromoButton> getPromoButtons(@NonNull Resources res)
    {
      String[] labels = res.getStringArray(getButtonLabels());
      String[] links = res.getStringArray(getButtonLinks());

      if (labels.length != links.length)
        throw new AssertionError("Button labels count must be equal to links count!");

      List<PromoButton> result = new ArrayList<>();
      for (int i = 0; i < labels.length; i++)
        result.add(new PromoButton(labels[i], links[i]));

      return result;
    }

    private void processSwitchBlock(int position, @NonNull View res)
    {
      View switchBlock = res.findViewById(R.id.switch_block);
      String text = mSwitchTitles[position];
      if (TextUtils.isEmpty(text))
        UiUtils.hide(switchBlock);
      else
      {
        ((TextView)switchBlock.findViewById(R.id.switch_title))
          .setText(text);

        TextView subtitle = switchBlock.findViewById(R.id.switch_subtitle);
        if (TextUtils.isEmpty(mSwitchSubtitles[position]))
          UiUtils.hide(subtitle);
        else
          subtitle.setText(mSwitchSubtitles[position]);

        final SwitchCompat checkBox = switchBlock.findViewById(R.id.switch_box);
        checkBox.setOnCheckedChangeListener((buttonView, isChecked) -> onSwitchChanged(position, isChecked));

        switchBlock.setOnClickListener(v -> checkBox.performClick());
      }
    }

    private void processButton(int position, @NonNull View res)
    {
      final TextView button = res.findViewById(R.id.button);
      PromoButton promo = mPromoButtons.get(position);
      if (promo == null || TextUtils.isEmpty(promo.getLabel()))
      {
        UiUtils.hide(button);
        return;
      }

      button.setText(promo.getLabel());
      button.setOnClickListener(v -> onPromoButtonClicked(button));
    }

    abstract void onPromoButtonClicked(@NonNull View view);

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
      mDots[i].setImageResource(ThemeUtils.isNightTheme(requireContext()) ? i == cur ? R.drawable.news_marker_active_night
                                                                                     : R.drawable.news_marker_inactive_night
                                                                          : i == cur ? R.drawable.news_marker_active
                                                                                     : R.drawable.news_marker_inactive);
    }
  }

  private void fixPagerSize()
  {
    Context context = requireContext();

    if (!UiUtils.isTablet(context))
      return;

    UiUtils.waitLayout(mPager, () -> {
      int maxWidth = UiUtils.dimen(context, R.dimen.news_max_width);
      int maxHeight = UiUtils.dimen(context, R.dimen.news_max_height);

      if (mPager.getWidth() > maxWidth || mPager.getHeight() > maxHeight)
      {
        mPager.setLayoutParams(new LinearLayout.LayoutParams(Math.min(maxWidth, mPager.getWidth()),
                                                             Math.min(maxHeight, mPager.getHeight())));
      }
    });
  }

  abstract Adapter createAdapter();

  @Override
  protected int getCustomTheme()
  {
    return (UiUtils.isTablet(requireContext()) ? super.getCustomTheme()
                                               : getFullscreenTheme());
  }

  @StyleRes
  @Override
  protected int getFullscreenLightTheme()
  {
    return R.style.MwmTheme_DialogFragment_NoFullscreen;
  }

  @StyleRes
  @Override
  protected int getFullscreenDarkTheme()
  {
    return R.style.MwmTheme_DialogFragment_NoFullscreen_Night;
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
        trackStatisticEvent(Statistics.ParamValue.NEXT);
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

    trackStatisticEvent(Statistics.ParamValue.OPEN);
    return res;
  }

  private void trackStatisticEvent(@NonNull String value)
  {
    Statistics.ParameterBuilder builder = Statistics
        .params()
        .add(Statistics.EventParam.ACTION, value)
        .add(Statistics.EventParam.VERSION, BuildConfig.VERSION_NAME);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.WHATS_NEW_ACTION, builder);
  }

  @CallSuper
  protected void onDoneClick()
  {
    dismissAllowingStateLoss();
    if (mListener != null)
      mListener.onDialogDone();
    trackStatisticEvent(Statistics.ParamValue.CLOSE);
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

  @Nullable
  protected NewsDialogListener getListener()
  {
    return mListener;
  }

  void resetListener(@Nullable NewsDialogListener listener)
  {
    mListener = listener;
  }

  public interface NewsDialogListener
  {
    void onDialogDone();
  }
}
