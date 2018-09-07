package com.mapswithme.maps.tips;

import android.app.Activity;
import android.graphics.Typeface;
import android.support.annotation.IdRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.view.View;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import uk.co.samuelwall.materialtaptargetprompt.MaterialTapTargetPrompt;
import uk.co.samuelwall.materialtaptargetprompt.extras.backgrounds.FullscreenPromptBackground;

import java.util.Arrays;
import java.util.List;

public enum TipsProvider
{
  BOOKMARKS(R.string.tips_bookmarks_catalog_title,
            R.string.tips_bookmarks_catalog_message,
            R.id.bookmarks, MainMenu.Item.BOOKMARKS, MwmActivity.class)
      {
        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return new ClickInterceptor.OpenBookmarksCatalog();
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
          return new ClickInterceptor.SearchHotels();
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
          return new ClickInterceptor.OpenDiscoveryScreen();
        }
      },

  MAP_LAYERS(R.string.tips_map_layers_title,
             R.string.tips_map_layers_message,
             R.id.subway, null, MwmActivity.class)
      {

        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return new ClickInterceptor.ActivateSubwayLayer();
        }
      },

  STUB
      {
        @NonNull
        @Override
        public ClickInterceptor createClickInterceptor()
        {
          return params -> {};
        }
      };

  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = TipsProvider.class.getSimpleName();

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

  TipsProvider(@StringRes int primaryText, @StringRes int secondaryText, @IdRes int anchorViewId,
               @Nullable MainMenu.Item siblingMenuItem, @NonNull Class<?>... allowedScreens)
  {
    mPrimaryText = primaryText;
    mSecondaryText = secondaryText;
    mAnchorViewId = anchorViewId;
    mSiblingMenuItem = siblingMenuItem;
    mAllowedScreens = Arrays.asList(allowedScreens);
  }

  TipsProvider()
  {
    this(UiUtils.NO_ID, UiUtils.NO_ID, UiUtils.NO_ID, null);
  }

  private boolean isScreenAllowed(@NonNull Class<?> screenClass)
  {
    return mAllowedScreens.contains(screenClass);
  }
  @SuppressWarnings("UnusedReturnValue")
  @Nullable
  public MaterialTapTargetPrompt showTutorial(@NonNull Activity activity)
  {
    View target = activity.findViewById(mAnchorViewId);
    return new MaterialTapTargetPrompt.Builder(activity)
        .setTarget(target)
        .setFocalRadius(R.dimen.nav_street_height)
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
        .setPromptBackground(new FullscreenPromptBackground())
        .show();
  }

  @Nullable
  public MainMenu.Item getSiblingMenuItem()
  {
    return mSiblingMenuItem;
  }

  @NonNull
  public abstract ClickInterceptor createClickInterceptor();

  @NonNull
  public static <T> TipsProvider requestCurrent(@NonNull Class<T> requiredScreenClass)
  {
    int index = Framework.nativeGetCurrentTipsApi();
    TipsProvider value = index >= 0 ? values()[index] : STUB;
    TipsProvider tipsProvider = value != STUB && value.isScreenAllowed(requiredScreenClass) ? value
                                                                                            : STUB;
    LOGGER.d(TAG, "tipsProvider = " + tipsProvider);
    return tipsProvider;
  }

  public interface ClickInterceptor
  {
    void onInterceptClick(@NonNull MwmActivity params);

    abstract class AbstractClickInterceptor implements ClickInterceptor
    {
      @NonNull
      private final TipsProvider mTipsProvider;

      AbstractClickInterceptor(@NonNull TipsProvider tipsProvider)
      {
        mTipsProvider = tipsProvider;
      }

      @NonNull
      TipsProvider getType()
      {
        return mTipsProvider;
      }

      @Override
      public final void onInterceptClick(@NonNull MwmActivity params)
      {
        Framework.tipsShown(getType());
        onInterceptClickInternal(params);
      }

      protected abstract void onInterceptClickInternal(@NonNull MwmActivity params);
    }

    class OpenBookmarksCatalog extends AbstractClickInterceptor
    {
      OpenBookmarksCatalog()
      {
        super(BOOKMARKS);
      }

      @Override
      public void onInterceptClickInternal(@NonNull MwmActivity params)
      {
        BookmarksCatalogActivity.startForResult(params,
                                                BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY);
      }
    }

    class ActivateSubwayLayer extends AbstractClickInterceptor
    {
      ActivateSubwayLayer()
      {
        super(MAP_LAYERS);
      }

      @Override
      public void onInterceptClickInternal(@NonNull MwmActivity params)
      {
        Mode.SUBWAY.setEnabled(params, true);
        params.onSubwayLayerSelected();
      }
    }

    class SearchHotels extends AbstractClickInterceptor
    {
      SearchHotels()
      {
        super(SEARCH);
      }

      @Override
      public void onInterceptClickInternal(@NonNull MwmActivity params)
      {
        params.showSearch(params.getString(R.string.hotel));
      }
    }

    class OpenDiscoveryScreen extends AbstractClickInterceptor
    {
      OpenDiscoveryScreen()
      {
        super(DISCOVERY);
      }

      @Override
      public void onInterceptClickInternal(@NonNull MwmActivity params)
      {
        params.showDiscovery();
      }
    }
  }
}
