package com.mapswithme.maps.widget.placepage;

import android.animation.ValueAnimator;
import android.content.res.Resources;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import com.bumptech.glide.Glide;
import com.bumptech.glide.load.resource.drawable.GlideDrawable;
import com.bumptech.glide.request.RequestListener;
import com.bumptech.glide.request.target.Target;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.data.Banner;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.UiUtils;

import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;
import static com.mapswithme.util.SharedPropertiesUtils.isShowcaseSwitchedOnLocal;

final class BannerController implements View.OnClickListener
{
  private static final int DURATION_DEFAULT =
      MwmApplication.get().getResources().getInteger(R.integer.anim_default);

  @Nullable
  private Banner mBanner;
  @Nullable
  private OnBannerClickListener mListener;

  @NonNull
  private final View mFrame;
  @Nullable
  private final ImageView mIcon;
  @Nullable
  private final TextView mTitle;
  @Nullable
  private final TextView mMessage;
  @Nullable
  private final View mAdMarker;

  private final float mCloseFrameHeight;
  private final float mCloseIconSize;
  private final float mOpenIconSize;
  private final float mMarginBase;
  private final float mMarginHalfPlus;

  @NonNull
  private final Resources mResources;

  private boolean mIsOpened = false;

  @Nullable
  private ValueAnimator mIconAnimator;

  BannerController(@NonNull View bannerView, @Nullable OnBannerClickListener listener)
  {
    mFrame = bannerView;
    mListener = listener;
    mResources = mFrame.getResources();
    mCloseFrameHeight = mResources.getDimension(R.dimen.placepage_banner_height);
    mCloseIconSize = mResources.getDimension(R.dimen.placepage_banner_icon_size);
    mOpenIconSize = mResources.getDimension(R.dimen.placepage_banner_icon_size_full);
    mMarginBase = mResources.getDimension(R.dimen.margin_base);
    mMarginHalfPlus = mResources.getDimension(R.dimen.margin_half_plus);
    mIcon = (ImageView) bannerView.findViewById(R.id.iv__banner_icon);
    mTitle = (TextView) bannerView.findViewById(R.id.tv__banner_title);
    mMessage = (TextView) bannerView.findViewById(R.id.tv__banner_message);
    mAdMarker = bannerView.findViewById(R.id.tv__banner);
  }

  void updateData(@Nullable Banner banner)
  {
    mBanner = banner;
    boolean showBanner = banner != null && ConnectionState.isConnected()
                         && isShowcaseSwitchedOnLocal();
    UiUtils.showIf(showBanner, mFrame);
    if (!showBanner)
      return;

    loadIcon(banner);
    if (mTitle != null)
    {
      String title = mResources.getString(mResources.getIdentifier(banner.getTitle(), "string", mFrame.getContext().getPackageName()));
      if (!TextUtils.isEmpty(title))
        mTitle.setText(title);
    }
    if (mMessage != null)
    {
      String message = mResources.getString(mResources.getIdentifier(banner.getMessage(), "string", mFrame.getContext().getPackageName()));
      if (!TextUtils.isEmpty(message))
        mMessage.setText(message);
    }

    if (UiUtils.isLandscape(mFrame.getContext()))
      open();
  }

  boolean isShowing()
  {
    return !UiUtils.isHidden(mFrame);
  }

  void open()
  {
    if (!isShowing() || mBanner == null || mIsOpened)
      return;

    mIsOpened = true;
    setFrameHeight(WRAP_CONTENT);
    setIconParams(mOpenIconSize, 0, mMarginBase);
    UiUtils.show(mMessage, mAdMarker);
    loadIcon(mBanner);
    if (mTitle != null)
      mTitle.setMaxLines(2);
    mFrame.setOnClickListener(this);
  }

  void close()
  {
    if (!isShowing() || mBanner == null || !mIsOpened)
      return;

    mIsOpened = false;
    setFrameHeight((int) mCloseFrameHeight);
    setIconParams(mCloseIconSize, mMarginBase, mMarginHalfPlus);
    UiUtils.hide(mMessage, mAdMarker);
    loadIcon(mBanner);
    if (mTitle != null)
      mTitle.setMaxLines(1);
    mFrame.setOnClickListener(null);
  }

  private void setFrameHeight(int height)
  {
    ViewGroup.LayoutParams lp = mFrame.getLayoutParams();
    lp.height = height;
    mFrame.setLayoutParams(lp);
  }

  private void setIconParams(final float size, final float marginRight, final float marginTop)
  {
    if (mIcon == null || UiUtils.isHidden(mIcon))
      return;

    if (mIconAnimator != null)
      mIconAnimator.cancel();

    final ViewGroup.MarginLayoutParams lp = (ViewGroup.MarginLayoutParams) mIcon.getLayoutParams();
    final float startSize = lp.height;
    final float startRight = lp.rightMargin;
    final float startTop = lp.topMargin;
    mIconAnimator = ValueAnimator.ofFloat(0.0f, 1.0f);
    if (mIconAnimator == null)
      return;

    mIconAnimator.addUpdateListener(new ValueAnimator.AnimatorUpdateListener()
    {
      @Override
      public void onAnimationUpdate(ValueAnimator animation)
      {
        float t = (float) animation.getAnimatedValue();
        int newSize = (int) (startSize + t * (size - startSize));
        lp.height = newSize;
        lp.width = newSize;
        lp.rightMargin = (int) (startRight + t * (marginRight - startRight));
        lp.topMargin = (int) (startTop + t * (marginTop - startTop));
        mIcon.setLayoutParams(lp);
      }
    });
    mIconAnimator.setDuration(DURATION_DEFAULT);
    mIconAnimator.start();
  }

  private void loadIcon(@NonNull Banner banner)
  {
    if (mIcon == null || TextUtils.isEmpty(banner.getIconUrl()))
      return;

    Glide.with(mIcon.getContext())
         .load(banner.getIconUrl())
         .centerCrop()
         .listener(new RequestListener<String, GlideDrawable>()
         {
           @Override
           public boolean onException(Exception e, String model, Target<GlideDrawable> target,
                                      boolean isFirstResource)
           {
             UiUtils.hide(mIcon);
             return false;
           }

           @Override
           public boolean onResourceReady(GlideDrawable resource, String model,
                                          Target<GlideDrawable> target, boolean isFromMemoryCache,
                                          boolean isFirstResource)
           {
             UiUtils.show(mIcon);
             return false;
           }
         })
         .into(mIcon);
  }

  @Override
  public void onClick(View v)
  {
    if (mListener != null && mBanner != null)
      mListener.onBannerClick(mBanner);
  }

  interface OnBannerClickListener
  {
    void onBannerClick(@NonNull Banner banner);
  }
}
