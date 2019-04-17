package com.mapswithme.maps.news;

import android.app.Activity;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;

public enum IntroductionScreenFactory
{
  FREE_GUIDE
      {
        @Override
        public int getTitle()
        {
          return R.string.onboarding_guide_direct_download_title;
        }

        @Override
        public int getSubtitle()
        {
          return R.string.onboarding_guide_direct_download_subtitle;
        }

        @Override
        public int getAction()
        {
          return R.string.onboarding_guide_direct_download_button;
        }

        @Override
        public int getImage()
        {
          return R.drawable.img_onboarding_guide;
        }

        @NonNull
        @Override
        public OnIntroductionButtonClickListener createButtonClickListener()
        {
          return new OnIntroductionButtonClickListener()
          {
            @Override
            public void onIntroductionButtonClick(@NonNull Activity activity, @NonNull String deeplink)
            {
              BookmarkCategoriesActivity.startForResult(activity, BookmarksPageFactory.DOWNLOADED.ordinal(), deeplink);
            }
          };
        }
      };

  @StringRes
  public abstract int getTitle();
  @StringRes
  public abstract int getSubtitle();
  @StringRes
  public abstract int getAction();
  @DrawableRes
  public abstract int getImage();
  @NonNull
  public abstract OnIntroductionButtonClickListener createButtonClickListener();
}
