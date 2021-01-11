package com.mapswithme.maps.tips;

import android.app.Activity;
import android.content.Context;
import android.graphics.Typeface;
import android.view.View;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import uk.co.samuelwall.materialtaptargetprompt.MaterialTapTargetPrompt;

import java.util.Arrays;
import java.util.List;

public enum Tutorial
{
  BOOKMARKS(R.string.tips_bookmarks_catalog_title,
            R.string.tips_bookmarks_catalog_message,
            R.id.bookmarks, MainMenu.Item.BOOKMARKS, MwmActivity.class)
      {
        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return ClickInterceptorFactory.createOpenBookmarksCatalogListener();
        }
      },

  SEARCH(R.string.tips_book_hotel_title,
         R.string.tips_book_hotel_message,
         R.id.search, MainMenu.Item.SEARCH, MwmActivity.class)
      {
        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return ClickInterceptorFactory.createSearchHotelsListener();
        }
      },

  DISCOVERY(R.string.tips_discover_button_title,
            R.string.tips_discover_button_message,
            R.id.discovery, MainMenu.Item.DISCOVERY, MwmActivity.class)
      {
        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return ClickInterceptorFactory.createOpenDiscoveryScreenListener();
        }
      },

  SUBWAY(R.string.tips_map_layers_title,
         R.string.tips_map_layers_message,
         R.id.subway, null, MwmActivity.class)
      {
        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return ClickInterceptorFactory.createActivateSubwayLayerListener();
        }
      },

  ISOLINES(R.string.tips_map_layers_title_countour,
           R.string.tips_map_layers_message_countour,
           R.id.isolines, null, MwmActivity.class)
      {

        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return ClickInterceptorFactory.createActivateIsolinesLayerListener();
        }
      },

  STUB
      {
        @Override
        public void show(@NonNull Activity activity,
                         @Nullable MaterialTapTargetPrompt.PromptStateChangeListener listener)
        {
          throw new UnsupportedOperationException("Not supported here!");
        }

        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return activity -> {};
        }
      };

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = Tutorial.class.getSimpleName();

  @StringRes
  private final int mPrimaryText;
  @StringRes
  private final int mSecondaryText;
  @IdRes
  private final int mAnchorViewId;
  @NonNull
  private final List<Class<?>> mAllowedScreens;
  @Nullable
  private final MainMenu.Item mSiblingMenuItem;

  Tutorial(@StringRes int primaryText, @StringRes int secondaryText, @IdRes int anchorViewId,
           @Nullable MainMenu.Item siblingMenuItem, @NonNull Class<?>... allowedScreens)
  {
    mPrimaryText = primaryText;
    mSecondaryText = secondaryText;
    mAnchorViewId = anchorViewId;
    mSiblingMenuItem = siblingMenuItem;
    mAllowedScreens = Arrays.asList(allowedScreens);
  }

  Tutorial()
  {
    this(UiUtils.NO_ID, UiUtils.NO_ID, UiUtils.NO_ID, null);
  }

  private boolean isScreenAllowed(@NonNull Class<?> screenClass)
  {
    return mAllowedScreens.contains(screenClass);
  }

  public void show(@NonNull Activity activity,
                   @Nullable  MaterialTapTargetPrompt.PromptStateChangeListener listener)
  {
    View target = activity.findViewById(mAnchorViewId);
    MaterialTapTargetPrompt.Builder builder = new MaterialTapTargetPrompt
        .Builder(activity)
        .setTarget(target)
        .setFocalRadius(R.dimen.focal_radius)
        .setPrimaryText(mPrimaryText)
        .setPrimaryTextSize(R.dimen.text_size_toolbar)
        .setPrimaryTextColour(ThemeUtils.getColor(activity, R.attr.tipsPrimaryTextColor))
        .setPrimaryTextTypeface(Typeface.DEFAULT_BOLD)
        .setSecondaryText(mSecondaryText)
        .setSecondaryTextColour(ThemeUtils.getColor(activity, R.attr.tipsSecondaryTextColor))
        .setSecondaryTextSize(R.dimen.text_size_body_3)
        .setSecondaryTextTypeface(Typeface.DEFAULT)
        .setBackgroundColour(ThemeUtils.getColor(activity, R.attr.tipsBgColor))
        .setFocalColour(activity.getResources().getColor(android.R.color.transparent))
        .setPromptBackground(new ImmersiveModeCompatPromptBackground(activity.getWindowManager()))
        .setPromptStateChangeListener(listener)
        .setIgnoreStatusBar(true);
    builder.show();
  }

  @Nullable
  public MainMenu.Item getSiblingMenuItem()
  {
    return mSiblingMenuItem;
  }

  @NonNull
  public abstract ClickInterceptor createClickInterceptor();

  @NonNull
  public static <T> Tutorial requestCurrent(@NonNull Context context,
                                            @NonNull Class<T> requiredScreenClass)
  {
    if (MwmApplication.from(context).isFirstLaunch())
      return STUB;

    int index = Framework.nativeGetCurrentTipIndex();
    Tutorial value = index >= 0 ? values()[index] : STUB;
    Tutorial tipsApi = value != STUB && value.isScreenAllowed(requiredScreenClass) ? value
                                                                                   : STUB;
    LOGGER.d(TAG, "tipsApi = " + tipsApi);
    return tipsApi;
  }

  public boolean isLayer()
  {
    return this == SUBWAY || this == ISOLINES;
  }
}
