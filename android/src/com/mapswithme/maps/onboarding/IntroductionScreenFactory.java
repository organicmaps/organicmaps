package com.mapswithme.maps.onboarding;

import android.app.Activity;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;
import com.mapswithme.util.statistics.StatisticValueConverter;

public enum IntroductionScreenFactory implements StatisticValueConverter<String>
{
  FREE_GUIDE
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "catalogue";
        }

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
              BookmarkCategoriesActivity.startForResult(activity,
                                                        BookmarksPageFactory.DOWNLOADED.ordinal(),
                                                        deeplink);
            }
          };
        }
      },
  GUIDES_PAGE
      {
        @NonNull
        @Override
        public String toStatisticValue()
        {
          return "guides_page";
        }

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
          return R.drawable.img_onboarding_guide;
        }

        @NonNull
        @Override
        public OnIntroductionButtonClickListener createButtonClickListener()
        {
          return new OnIntroductionButtonClickListener()
          {
            @Override
            public void onIntroductionButtonClick(@NonNull Activity activity,
                                                  @NonNull String deeplink)
            {
              BookmarksCatalogActivity.startForResult(activity,
                                                      BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                                      deeplink);
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
