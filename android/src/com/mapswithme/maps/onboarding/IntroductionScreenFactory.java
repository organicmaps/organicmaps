package com.mapswithme.maps.onboarding;

import android.app.Activity;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
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
          return 0;
        }

        @NonNull
        @Override
        public OnIntroductionButtonClickListener createButtonClickListener()
        {
          return (activity, deeplink) -> BookmarkCategoriesActivity.startForResult(activity,
                                                    BookmarksPageFactory.DOWNLOADED.ordinal(),
                                                    deeplink);
        }
      },
  GUIDES_PAGE
      {
        @Override
        public int getTitle()
        {
          return R.string.onboarding_bydeeplink_guide_title;
        }

        @Override
        public int getSubtitle()
        {
          return R.string.onboarding_bydeeplink_guide_subtitle;
        }

        @Override
        public int getAction()
        {
          return R.string.onboarding_guide_direct_download_button;
        }

        @Override
        public int getImage()
        {
          return 0;
        }

        @NonNull
        @Override
        public OnIntroductionButtonClickListener createButtonClickListener()
        {
          return (activity, deeplink) -> BookmarksCatalogActivity.startForResult(activity,
                                                  BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                                  deeplink);
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
