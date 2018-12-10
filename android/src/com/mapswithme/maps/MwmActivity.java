package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Configuration;
import android.graphics.Rect;
import android.location.Location;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StyleRes;
import android.support.v4.app.DialogFragment;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentActivity;
import android.support.v4.app.FragmentManager;
import android.support.v7.app.AlertDialog;
import android.support.v7.widget.Toolbar;
import android.text.TextUtils;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.Toast;

import com.mapswithme.maps.Framework.MapObjectListener;
import com.mapswithme.maps.activity.CustomNavigateUpListener;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.api.ParsedRoutingData;
import com.mapswithme.maps.api.ParsedSearchRequest;
import com.mapswithme.maps.api.ParsedUrlMwmRequest;
import com.mapswithme.maps.api.RoutePoint;
import com.mapswithme.maps.auth.PassportAuthDialogFragment;
import com.mapswithme.maps.background.NotificationCandidate;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.BookmarksPageFactory;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.maps.discovery.DiscoveryActivity;
import com.mapswithme.maps.discovery.DiscoveryFragment;
import com.mapswithme.maps.discovery.ItemType;
import com.mapswithme.maps.downloader.DownloaderActivity;
import com.mapswithme.maps.downloader.DownloaderFragment;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.MigrationFragment;
import com.mapswithme.maps.downloader.OnmapDownloader;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.EditorActivity;
import com.mapswithme.maps.editor.EditorHostFragment;
import com.mapswithme.maps.editor.FeatureCategoryActivity;
import com.mapswithme.maps.editor.ReportFragment;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.location.CompassData;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.maplayer.MapLayerCompositeController;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.maps.maplayer.subway.OnSubwayLayerToggleListener;
import com.mapswithme.maps.maplayer.subway.SubwayManager;
import com.mapswithme.maps.maplayer.traffic.OnTrafficLayerToggleListener;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButton;
import com.mapswithme.maps.purchase.AdsRemovalActivationCallback;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.maps.purchase.FailedPurchaseChecker;
import com.mapswithme.maps.purchase.PurchaseCallback;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.maps.purchase.PurchaseFactory;
import com.mapswithme.maps.routing.NavigationController;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.routing.RoutingBottomMenuListener;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.routing.RoutingPlanFragment;
import com.mapswithme.maps.routing.RoutingPlanInplaceController;
import com.mapswithme.maps.search.BookingFilterParams;
import com.mapswithme.maps.search.FilterActivity;
import com.mapswithme.maps.search.FloatingSearchToolbarController;
import com.mapswithme.maps.search.HotelsFilter;
import com.mapswithme.maps.search.NativeSearchListener;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.search.SearchFilterController;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.tips.TipsApi;
import com.mapswithme.maps.ugc.EditParams;
import com.mapswithme.maps.ugc.UGC;
import com.mapswithme.maps.ugc.UGCEditorActivity;
import com.mapswithme.maps.widget.FadeView;
import com.mapswithme.maps.widget.menu.BaseMenu;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.maps.widget.menu.MyPositionButton;
import com.mapswithme.maps.widget.placepage.BasePlacePageAnimationController;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.Animations;
import com.mapswithme.util.Counters;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.permissions.PermissionsResult;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.PlacePageTracker;
import com.mapswithme.util.statistics.Statistics;

import java.io.Serializable;
import java.util.List;
import java.util.Locale;
import java.util.Stack;

public class MwmActivity extends BaseMwmFragmentActivity
                      implements MapObjectListener,
                                 View.OnTouchListener,
                                 BasePlacePageAnimationController.OnVisibilityChangedListener,
                                 BasePlacePageAnimationController.OnAnimationListener,
                                 OnClickListener,
                                 MapFragment.MapRenderingListener,
                                 CustomNavigateUpListener,
                                 RoutingController.Container,
                                 LocationHelper.UiCallback,
                                 FloatingSearchToolbarController.VisibilityListener,
                                 NativeSearchListener,
                                 NavigationButtonsAnimationController.OnTranslationChangedListener,
                                 RoutingPlanInplaceController.RoutingPlanListener,
                                 RoutingBottomMenuListener,
                                 BookmarkManager.BookmarksLoadingListener,
                                 DiscoveryFragment.DiscoveryListener,
                                 FloatingSearchToolbarController.SearchToolbarListener,
                                 OnTrafficLayerToggleListener,
                                 OnSubwayLayerToggleListener,
                                 BookmarkManager.BookmarksCatalogListener,
                                 AdsRemovalPurchaseControllerProvider,
                                 AdsRemovalActivationCallback
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = MwmActivity.class.getSimpleName();

  public static final String EXTRA_TASK = "map_task";
  public static final String EXTRA_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
  private static final String EXTRA_CONSUMED = "mwm.extra.intent.processed";

  private static final String[] DOCKED_FRAGMENTS = { SearchFragment.class.getName(),
                                                     DownloaderFragment.class.getName(),
                                                     MigrationFragment.class.getName(),
                                                     RoutingPlanFragment.class.getName(),
                                                     EditorHostFragment.class.getName(),
                                                     ReportFragment.class.getName(),
                                                     DiscoveryFragment.class.getName() };
  // Instance state
  private static final String STATE_PP = "PpState";
  private static final String STATE_MAP_OBJECT = "MapObject";
  private static final String EXTRA_LOCATION_DIALOG_IS_ANNOYING = "LOCATION_DIALOG_IS_ANNOYING";

  private static final int REQ_CODE_LOCATION_PERMISSION = 1;
  private static final int REQ_CODE_DISCOVERY = 2;
  private static final int REQ_CODE_SHOW_SIMILAR_HOTELS = 3;

  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private final StoragePathManager mPathManager = new StoragePathManager();

  @Nullable
  private MapFragment mMapFragment;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageView mPlacePage;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private FadeView mFadeView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPositionChooser;

  private RoutingPlanInplaceController mRoutingPlanInplaceController;

  @Nullable
  private NavigationController mNavigationController;

  private MainMenu mMainMenu;

  private PanelAnimator mPanelAnimator;
  @Nullable
  private OnmapDownloader mOnmapDownloader;

  @Nullable
  private MyPositionButton mNavMyPosition;
  @Nullable
  private NavigationButtonsAnimationController mNavAnimationController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private MapLayerCompositeController mToggleMapLayerController;
  @Nullable
  private SearchFilterController mFilterController;

  private boolean mIsTabletLayout;
  private boolean mIsFullscreen;
  private boolean mIsFullscreenAnimating;
  private boolean mIsAppearMenuLater;
  private boolean mIsLaunchByDeepLink;

  private FloatingSearchToolbarController mSearchController;

  private boolean mPlacePageRestored;

  private boolean mLocationErrorDialogAnnoying = false;
  @Nullable
  private Dialog mLocationErrorDialog;

  private boolean mRestoreRoutingPlanFragmentNeeded;
  @Nullable
  private Bundle mSavedForTabletState;
  @Nullable
  private PlacePageTracker mPlacePageTracker;
  @Nullable
  private PurchaseController<PurchaseCallback> mAdsRemovalPurchaseController;
  @Nullable
  private PurchaseController<FailedPurchaseChecker> mBookmarkPurchaseController;
  @NonNull
  private final OnClickListener mOnMyPositionClickListener = new CurrentPositionClickListener();

  public interface LeftAnimationTrackListener
  {
    void onTrackStarted(boolean collapsed);

    void onTrackFinished(boolean collapsed);

    void onTrackLeftAnimation(float offset);
  }

  public interface VisibleRectListener
  {
    void onVisibleRectChanged(Rect rect);
  }

  class VisibleRectMeasurer implements View.OnLayoutChangeListener
  {
    private VisibleRectListener mRectListener;
    private Rect mScreenFullRect = null;
    private Rect mLastVisibleRect = null;
    private boolean mPlacePageVisible = false;

    public VisibleRectMeasurer(VisibleRectListener listener)
    {
      mRectListener = listener;
    }

    void setPlacePageVisible(boolean visible)
    {
      int orientation = MwmActivity.this.getResources().getConfiguration().orientation;
      if(orientation == Configuration.ORIENTATION_LANDSCAPE)
      {
        mPlacePageVisible = visible;
        recalculateVisibleRect(mScreenFullRect);
      }
    }

    void setPreviewVisible(boolean visible)
    {
      int orientation = MwmActivity.this.getResources().getConfiguration().orientation;
      if(orientation == Configuration.ORIENTATION_PORTRAIT)
      {
        mPlacePageVisible = visible;
        recalculateVisibleRect(mScreenFullRect);
      }
    }

    @Override
    public void onLayoutChange(View v, int left, int top, int right, int bottom,
                               int oldLeft, int oldTop, int oldRight, int oldBottom)
    {
      mScreenFullRect = new Rect(left, top, right, bottom);
      if (mPlacePageVisible && UiUtils.isHidden(mPlacePage.GetPreview()))
        mPlacePageVisible = false;
      recalculateVisibleRect(mScreenFullRect);
    }

    private void recalculateVisibleRect(Rect r)
    {
      if (r == null)
        return;

      int orientation = MwmActivity.this.getResources().getConfiguration().orientation;

      Rect rect = new Rect(r.left, r.top, r.right, r.bottom);
      if (mPlacePageVisible)
      {
        int[] loc = new int[2];
        mPlacePage.GetPreview().getLocationOnScreen(loc);

        if(orientation == Configuration.ORIENTATION_PORTRAIT)
          rect.bottom = loc[1];
        else
          rect.left = mPlacePage.GetPreview().getWidth() + loc[0];
      }

      if (mLastVisibleRect == null || !mLastVisibleRect.equals(rect))
      {
        mLastVisibleRect = new Rect(rect.left, rect.top, rect.right, rect.bottom);
        if (mRectListener != null)
          mRectListener.onVisibleRectChanged(rect);
      }
    }
  }

  private VisibleRectMeasurer mVisibleRectMeasurer;

  public static Intent createShowMapIntent(@NonNull Context context, @Nullable String countryId)
  {
    return new Intent(context, DownloadResourcesLegacyActivity.class)
               .putExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY, countryId);
  }

  @NonNull
  public static Intent createAuthenticateIntent(@NonNull Context context)
  {
    return new Intent(context, MwmActivity.class)
        .addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION)
        .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
        .putExtra(MwmActivity.EXTRA_TASK,
                  new MwmActivity.ShowDialogTask(PassportAuthDialogFragment.class.getName()));
  }

  @NonNull
  public static Intent createLeaveReviewIntent(@NonNull Context context,
                                               @NonNull NotificationCandidate.MapObject mapObject)
  {
    return new Intent(context, MwmActivity.class)
      .addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION)
      .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
      .putExtra(MwmActivity.EXTRA_TASK, new MwmActivity.ShowUGCEditorTask(mapObject));
  }

  @Override
  public void onRenderingInitialized()
  {
    checkMeasurementSystem();
    checkKitkatMigrationMove();

    LocationHelper.INSTANCE.attach(this);
    runTasks();
  }

  @Override
  public void onRenderingRestored()
  {
    runTasks();
  }

  private void myPositionClick()
  {
    mLocationErrorDialogAnnoying = false;
    LocationHelper.INSTANCE.setStopLocationUpdateByUser(false);
    LocationHelper.INSTANCE.switchToNextMode();
    LocationHelper.INSTANCE.restart();
  }

  private void runTasks()
  {
    while (!mTasks.isEmpty())
      mTasks.pop().run(this);
  }

  private static void checkMeasurementSystem()
  {
    UnitLocale.initializeCurrentUnits();
  }

  private void checkKitkatMigrationMove()
  {
    mPathManager.checkKitkatMigration(this);
  }

  @Override
  protected int getFragmentContentResId()
  {
    return (mIsTabletLayout ? R.id.fragment_container
                            : super.getFragmentContentResId());
  }

  @Nullable
  Fragment getFragment(Class<? extends Fragment> clazz)
  {
    if (!mIsTabletLayout)
      throw new IllegalStateException("Must be called for tablets only!");

    return getSupportFragmentManager().findFragmentByTag(clazz.getName());
  }

  void replaceFragmentInternal(Class<? extends Fragment> fragmentClass, Bundle args)
  {
    super.replaceFragment(fragmentClass, args, null);
  }

  @Override
  public void replaceFragment(@NonNull Class<? extends Fragment> fragmentClass, @Nullable Bundle args, @Nullable Runnable completionListener)
  {
    if (mPanelAnimator.isVisible() && getFragment(fragmentClass) != null)
    {
      if (completionListener != null)
        completionListener.run();
      return;
    }

    mPanelAnimator.show(fragmentClass, args, completionListener);
  }

  public boolean containsFragment(@NonNull Class<? extends Fragment> fragmentClass)
  {
    return mIsTabletLayout && getFragment(fragmentClass) != null;
  }

  private void showBookmarks()
  {
    BookmarkCategoriesActivity.startForResult(this);
  }

  private void showTabletSearch(@Nullable Intent data, @NonNull String query)
  {
    if (mFilterController == null || data == null)
      return;

    BookingFilterParams params = data.getParcelableExtra(FilterActivity.EXTRA_FILTER_PARAMS);
    HotelsFilter filter = data.getParcelableExtra(FilterActivity.EXTRA_FILTER);
    mFilterController.setFilterAndParams(filter, params);

    showSearch(query);
  }

  public void showSearch(String query)
  {
    if (mIsTabletLayout)
    {
      mSearchController.hide();

      final Bundle args = new Bundle();
      args.putString(SearchActivity.EXTRA_QUERY, query);
      if (mFilterController != null)
      {
        args.putParcelable(FilterActivity.EXTRA_FILTER, mFilterController.getFilter());
        args.putParcelable(FilterActivity.EXTRA_FILTER_PARAMS, mFilterController.getBookingFilterParams());
      }
      replaceFragment(SearchFragment.class, args, null);
    }
    else
    {
      HotelsFilter filter = null;
      BookingFilterParams params = null;
      if (mFilterController != null)
      {
        filter = mFilterController.getFilter();
        params = mFilterController.getBookingFilterParams();
      }
      SearchActivity.start(this, query, filter, params);
    }
    if (mFilterController != null)
      mFilterController.resetFilter();
  }

  public void showEditor()
  {
    // TODO(yunikkk) think about refactoring. It probably should be called in editor.
    Editor.nativeStartEdit();
    Statistics.INSTANCE.trackEditorLaunch(false);
    if (mIsTabletLayout)
      replaceFragment(EditorHostFragment.class, null, null);
    else
      EditorActivity.start(this);
  }

  private void shareMyLocation()
  {
    final Location loc = LocationHelper.INSTANCE.getSavedLocation();
    if (loc != null)
    {
      final String geoUrl = Framework.nativeGetGe0Url(loc.getLatitude(), loc.getLongitude(), Framework.nativeGetDrawScale(), "");
      final String httpUrl = Framework.getHttpGe0Url(loc.getLatitude(), loc.getLongitude(), Framework.nativeGetDrawScale(), "");
      final String body = getString(R.string.my_position_share_sms, geoUrl, httpUrl);
      ShareOption.ANY.share(this, body);
      return;
    }

    new AlertDialog.Builder(MwmActivity.this)
        .setMessage(R.string.unknown_current_position)
        .setCancelable(true)
        .setPositiveButton(android.R.string.ok, null)
        .show();
  }

  @Override
  public void showDownloader(boolean openDownloaded)
  {
    if (RoutingController.get().checkMigration(this))
      return;

    final Bundle args = new Bundle();
    args.putBoolean(DownloaderActivity.EXTRA_OPEN_DOWNLOADED, openDownloaded);
    if (mIsTabletLayout)
    {
      SearchEngine.INSTANCE.cancel();
      mSearchController.refreshToolbar();
      replaceFragment(MapManager.nativeIsLegacyMode() ? MigrationFragment.class : DownloaderFragment.class, args, null);
    }
    else
    {
      startActivity(new Intent(this, DownloaderActivity.class).putExtras(args));
    }
  }

  @Override
  @StyleRes
  public int getThemeResourceId(@NonNull String theme)
  {
    if (ThemeUtils.isDefaultTheme(theme))
      return R.style.MwmTheme_MainActivity;

    if (ThemeUtils.isNightTheme(theme))
      return R.style.MwmTheme_Night_MainActivity;

    return super.getThemeResourceId(theme);
  }

  @SuppressLint("InlinedApi")
  @CallSuper
  @Override
  protected void safeOnCreate(@Nullable Bundle savedInstanceState)
  {
    super.safeOnCreate(savedInstanceState);
    if (savedInstanceState != null)
      mLocationErrorDialogAnnoying = savedInstanceState.getBoolean(EXTRA_LOCATION_DIALOG_IS_ANNOYING);
    mIsTabletLayout = getResources().getBoolean(R.bool.tabletLayout);

    if (!mIsTabletLayout && (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP))
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

    setContentView(R.layout.activity_map);
    mIsLaunchByDeepLink = getIntent().getBooleanExtra(EXTRA_LAUNCH_BY_DEEP_LINK, false);
    initViews();

    Statistics.INSTANCE.trackConnectionState();

    mSearchController = new FloatingSearchToolbarController(this, this);
    mSearchController.setVisibilityListener(this);

    SharingHelper.INSTANCE.initialize();

    mAdsRemovalPurchaseController = PurchaseFactory.createAdsRemovalPurchaseController(this);
    mAdsRemovalPurchaseController.initialize(this);
    mAdsRemovalPurchaseController.validateExistingPurchases();

    mBookmarkPurchaseController = PurchaseFactory.createFailedBookmarkPurchaseController(this);
    mBookmarkPurchaseController.initialize(this);
    mBookmarkPurchaseController.validateExistingPurchases();

    boolean isConsumed = savedInstanceState == null && processIntent(getIntent());
    // If the map activity is launched by any incoming intent (deeplink, update maps event, etc)
    // we haven't to try restoring the route, showing the tips, etc.
    if (isConsumed)
      return;

    if (savedInstanceState == null && RoutingController.get().hasSavedRoute())
    {
      addTask(new RestoreRouteTask());
      return;
    }

    initTips();
  }

  private void initViews()
  {
    initMap();
    initNavigationButtons();

    mPlacePage = findViewById(R.id.info_box);
    mPlacePage.setOnVisibilityChangedListener(this);
    mPlacePage.setOnAnimationListener(this);
    mPlacePageTracker = new PlacePageTracker(mPlacePage);

    if (!mIsTabletLayout)
    {
      mRoutingPlanInplaceController = new RoutingPlanInplaceController(this, this, this);
      removeCurrentFragment(false);
    }

    mNavigationController = new NavigationController(this);
    initMainMenu();
    initOnmapDownloader();
    initPositionChooser();
    initFilterViews();
  }

  private void initTips()
  {
    TipsApi api = TipsApi.requestCurrent(getClass());
    if (api == TipsApi.STUB)
      return;

    api.showTutorial(getActivity());
  }

  private void initFilterViews()
  {
    View frame = findViewById(R.id.filter_frame);
    if (frame != null)
    {
      mFilterController = new SearchFilterController(frame, new SearchFilterController
          .DefaultFilterListener()
      {
        @Override
        public void onShowOnMapClick()
        {
          showSearch(mSearchController.getQuery());
        }

        @Override
        public void onFilterClick()
        {
          HotelsFilter filter = null;
          BookingFilterParams params = null;
          if (mFilterController != null)
          {
            filter = mFilterController.getFilter();
            params = mFilterController.getBookingFilterParams();
          }
          FilterActivity.startForResult(MwmActivity.this, filter, params,
                                        FilterActivity.REQ_CODE_FILTER);
        }

        @Override
        public void onFilterClear()
        {
          runSearch();
        }
      }, R.string.search_in_table);
    }
  }

  private void runSearch()
  {
    // The previous search should be cancelled before the new one is started, since previous search
    // results are no longer needed.
    SearchEngine.INSTANCE.cancel();

    SearchEngine.INSTANCE.searchInteractive(mSearchController.getQuery(), System.nanoTime(),
                                   false /* isMapAndTable */,
                                   mFilterController != null ? mFilterController.getFilter() : null,
                                   mFilterController != null ? mFilterController.getBookingFilterParams() : null);
    SearchEngine.INSTANCE.setQuery(mSearchController.getQuery());
  }

  private void initPositionChooser()
  {
    mPositionChooser = findViewById(R.id.position_chooser);
    if (mPositionChooser == null)
      return;

    final Toolbar toolbar = mPositionChooser.findViewById(R.id.toolbar_position_chooser);
    UiUtils.extendViewWithStatusBar(toolbar);
    UiUtils.showHomeUpButton(toolbar);
    toolbar.setNavigationOnClickListener(v -> hidePositionChooser());
    mPositionChooser.findViewById(R.id.done).setOnClickListener(
        v ->
        {
          Statistics.INSTANCE.trackEditorLaunch(true);
          hidePositionChooser();
          if (Framework.nativeIsDownloadedMapAtScreenCenter())
            startActivity(new Intent(MwmActivity.this, FeatureCategoryActivity.class));
          else
            DialogUtils.showAlertDialog(MwmActivity.this, R.string.message_invalid_feature_position);
        });
    UiUtils.hide(mPositionChooser);
  }

  public void showPositionChooser(boolean isBusiness, boolean applyPosition)
  {
    UiUtils.show(mPositionChooser);
    setFullscreen(true);
    Framework.nativeTurnOnChoosePositionMode(isBusiness, applyPosition);
    closePlacePage();
    mSearchController.hide();
  }

  private void hidePositionChooser()
  {
    UiUtils.hide(mPositionChooser);
    Framework.nativeTurnOffChoosePositionMode();
    setFullscreen(false);
  }

  private void initMap()
  {
    mFadeView = findViewById(R.id.fade_view);
    mFadeView.setListener(new FadeView.Listener()
    {
      @Override
      public boolean onTouch()
      {
        return getCurrentMenu().close(true);
      }
    });

    mMapFragment = (MapFragment) getSupportFragmentManager().findFragmentByTag(MapFragment.class.getName());
    if (mMapFragment == null)
    {
      Bundle args = new Bundle();
      args.putBoolean(MapFragment.ARG_LAUNCH_BY_DEEP_LINK, mIsLaunchByDeepLink);
      mMapFragment = (MapFragment) MapFragment.instantiate(this, MapFragment.class.getName(), args);
      getSupportFragmentManager()
          .beginTransaction()
          .replace(R.id.map_fragment_container, mMapFragment, MapFragment.class.getName())
          .commit();
    }

    View container = findViewById(R.id.map_fragment_container);
    if (container != null)
    {
      container.setOnTouchListener(this);
    }
  }

  public boolean isMapAttached()
  {
    return mMapFragment != null && mMapFragment.isAdded();
  }

  private void initNavigationButtons()
  {
    View frame = findViewById(R.id.navigation_buttons);
    if (frame == null)
      return;

    View zoomIn = frame.findViewById(R.id.nav_zoom_in);
    zoomIn.setOnClickListener(this);
    View zoomOut = frame.findViewById(R.id.nav_zoom_out);
    zoomOut.setOnClickListener(this);
    View myPosition = frame.findViewById(R.id.my_position);
    mNavMyPosition = new MyPositionButton(myPosition, mOnMyPositionClickListener);

    initToggleMapLayerController(frame);
    mNavAnimationController = new NavigationButtonsAnimationController(
        zoomIn, zoomOut, myPosition, getWindow().getDecorView().getRootView(), this);
  }

  private void initToggleMapLayerController(@NonNull View frame)
  {
    ImageButton trafficBtn = frame.findViewById(R.id.traffic);
    TrafficButton traffic = new TrafficButton(trafficBtn);
    View subway = frame.findViewById(R.id.subway);
    mToggleMapLayerController = new MapLayerCompositeController(traffic, subway, this);
  }

  public boolean closePlacePage()
  {
    if (mPlacePage.isHidden())
      return false;

    mPlacePage.hide();
    Framework.nativeDeactivatePopup();
    return true;
  }

  public boolean closeSidePanel()
  {
    if (interceptBackPress())
      return true;

    if (removeCurrentFragment(true))
    {
      InputUtils.hideKeyboard(mFadeView);
      mFadeView.fadeOut();
      return true;
    }

    return false;
  }

  private void closeAllFloatingPanels()
  {
    if (!mIsTabletLayout)
      return;

    closePlacePage();
    if (removeCurrentFragment(true))
    {
      InputUtils.hideKeyboard(mFadeView);
      mFadeView.fadeOut();
    }
  }

  public void closeMenu(@Nullable Runnable procAfterClose)
  {
    mFadeView.fadeOut();
    mMainMenu.close(true, procAfterClose);
  }
  private boolean closePositionChooser()
  {
    if (UiUtils.isVisible(mPositionChooser))
    {
      hidePositionChooser();
      return true;
    }

    return false;
  }

  public void startLocationToPoint(final @Nullable MapObject endPoint,
                                   final boolean canUseMyPositionAsStart)
  {
    closeMenu(() -> {
      RoutingController.get().prepare(canUseMyPositionAsStart, endPoint);

      if (mPlacePage.isDocked() || !mPlacePage.isFloating())
        closePlacePage();
    });
  }

  private void toggleMenu()
  {
    getCurrentMenu().toggle(true);
    refreshFade();
  }

  public void refreshFade()
  {
    if (getCurrentMenu().isOpen())
      mFadeView.fadeIn();
    else
      mFadeView.fadeOut();
  }

  private void initMainMenu()
  {
    mMainMenu = new MainMenu(findViewById(R.id.menu_frame), this::onItemClickOrSkipAnim);

    if (mIsTabletLayout)
    {
      mPanelAnimator = new PanelAnimator(this);
      mPanelAnimator.registerListener(mMainMenu.getLeftAnimationTrackListener());
      return;
    }

    if (mPlacePage.isDocked())
      mPlacePage.setLeftAnimationTrackListener(mMainMenu.getLeftAnimationTrackListener());
  }

  private void onItemClickOrSkipAnim(@NonNull MainMenu.Item item)
  {
    if (mIsFullscreenAnimating)
      return;

    item.onClicked(this, item);
  }

  public void showDiscovery()
  {
    if (mIsTabletLayout)
    {
      replaceFragment(DiscoveryFragment.class, null, null);
    }
    else
    {
      Intent i = new Intent(MwmActivity.this, DiscoveryActivity.class);
      startActivityForResult(i, REQ_CODE_DISCOVERY);
    }
  }

  private void initOnmapDownloader()
  {
    mOnmapDownloader = new OnmapDownloader(this);
    if (mIsTabletLayout)
      mPanelAnimator.registerListener(mOnmapDownloader);
  }

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    if (!mPlacePage.isHidden())
    {
      outState.putInt(STATE_PP, mPlacePage.getState().ordinal());
      outState.putParcelable(STATE_MAP_OBJECT, mPlacePage.getMapObject());
      if (isChangingConfigurations())
        mPlacePage.setState(State.HIDDEN);
    }

    if (!mIsTabletLayout && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.onSaveState(outState);

    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.saveRoutingPanelState(outState);
    }

    if (mNavigationController != null)
      mNavigationController.onSaveState(outState);

    RoutingController.get().onSaveState();
    outState.putBoolean(EXTRA_LOCATION_DIALOG_IS_ANNOYING, mLocationErrorDialogAnnoying);

    if (mNavMyPosition != null)
      mNavMyPosition.onSaveState(outState);
    if(mNavAnimationController != null)
      mNavAnimationController.onSaveState(outState);

    if (mFilterController != null)
      mFilterController.onSaveState(outState);

    if (!isChangingConfigurations())
      RoutingController.get().saveRoute();
    else
      // We no longer need in a saved route if it's a configuration changing: theme switching,
      // orientation changing, etc. Otherwise, the saved route might be restored at undesirable moment.
      RoutingController.get().deleteSavedRoute();

    super.onSaveInstanceState(outState);
  }

  @Override
  protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState)
  {
    super.onRestoreInstanceState(savedInstanceState);

    final State state = State.values()[savedInstanceState.getInt(STATE_PP, 0)];
    if (state != State.HIDDEN)
    {
      mPlacePageRestored = true;
      MapObject mapObject = savedInstanceState.getParcelable(STATE_MAP_OBJECT);
      mPlacePage.setMapObject(mapObject, true,
                              new PlacePageView.SetMapObjectListener()
      {
        @Override
        public void onSetMapObjectComplete()
        {
          mPlacePage.setState(state);
        }
      });
      if (mPlacePageTracker != null)
        mPlacePageTracker.setMapObject(mapObject);
    }

    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
      {
        fragment.restoreRoutingPanelState(savedInstanceState);
      }
      else if (RoutingController.get().isPlanning())
      {
        mRestoreRoutingPlanFragmentNeeded = true;
        mSavedForTabletState = savedInstanceState;
      }
    }

    if (!mIsTabletLayout && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.restoreState(savedInstanceState);

    if (mNavigationController != null)
      mNavigationController.onRestoreState(savedInstanceState);

    if (mNavMyPosition != null)
      mNavMyPosition.onRestoreState(savedInstanceState);
    if(mNavAnimationController != null)
      mNavAnimationController.onRestoreState(savedInstanceState);

    if (mFilterController != null)
      mFilterController.onRestoreState(savedInstanceState);
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if (resultCode != Activity.RESULT_OK)
      return;

    switch (requestCode)
    {
      case REQ_CODE_DISCOVERY:
        handleDiscoveryResult(data);
        break;
      case FilterActivity.REQ_CODE_FILTER:
      case REQ_CODE_SHOW_SIMILAR_HOTELS:
        if (mIsTabletLayout)
        {
          showTabletSearch(data, getString(R.string.hotel));
          return;
        }
        handleFilterResult(data);
        break;
      case BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY:
        handleDownloadedCategoryResult(data);
        break;
    }
  }

  private void handleDownloadedCategoryResult(@NonNull Intent data)
  {
    BookmarkCategory category = data.getParcelableExtra(BookmarksCatalogActivity.EXTRA_DOWNLOADED_CATEGORY);
    if (category == null)
      throw new IllegalArgumentException("Category not found in bundle");

    MapTask mapTask = target -> showBookmarkCategory(category);
    addTask(mapTask);
  }

  private boolean showBookmarkCategory(@NonNull BookmarkCategory category)
  {
    Framework.nativeShowBookmarkCategory(category.getId());
    return true;
  }

  private void handleDiscoveryResult(@NonNull Intent data)
  {
    String action = data.getAction();
    if (TextUtils.isEmpty(action))
      return;

    switch (action)
    {
      case DiscoveryActivity.ACTION_ROUTE_TO:
        MapObject destination = data.getParcelableExtra(DiscoveryActivity.EXTRA_DISCOVERY_OBJECT);
        if (destination == null)
          return;

        onRouteToDiscoveredObject(destination);
        break;

      case DiscoveryActivity.ACTION_SHOW_ON_MAP:
        destination = data.getParcelableExtra(DiscoveryActivity.EXTRA_DISCOVERY_OBJECT);
        if (destination == null)
          return;

        onShowDiscoveredObject(destination);
        break;

      case DiscoveryActivity.ACTION_SHOW_FILTER_RESULTS:
        handleFilterResult(data);
        break;
    }
  }

  private void handleFilterResult(@Nullable Intent data)
  {
    if (data == null || mFilterController == null)
      return;

    setupSearchQuery(data);

    BookingFilterParams params = data.getParcelableExtra(FilterActivity.EXTRA_FILTER_PARAMS);
    mFilterController.setFilterAndParams(data.getParcelableExtra(FilterActivity.EXTRA_FILTER),
                                         params);
    mFilterController.updateFilterButtonVisibility(params != null);
    runSearch();
  }

  private void setupSearchQuery(@NonNull Intent data)
  {
    if (mSearchController == null)
      return;

    String query = data.getStringExtra(DiscoveryActivity.EXTRA_FILTER_SEARCH_QUERY);
    mSearchController.setQuery(TextUtils.isEmpty(query) ? getString(R.string.hotel) : query);
  }

  @Override
  public void onRouteToDiscoveredObject(@NonNull final MapObject object)
  {
    addTask((MapTask) target ->
    {
      RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
      RoutingController.get().prepare(true, object);
      return false;
    });
  }

  @Override
  public void onShowDiscoveredObject(@NonNull final MapObject object)
  {
    addTask((MapTask) target ->
    {
      Framework.nativeShowFeatureByLatLon(object.getLat(), object.getLon());
      return false;
    });
  }

  @Override
  public void onShowFilter()
  {
    FilterActivity.startForResult(MwmActivity.this, null, null,
                                  FilterActivity.REQ_CODE_FILTER);
  }

  @Override
  public void onShowSimilarObjects(@NonNull Items.SearchItem item, @NonNull ItemType type)
  {
    String query = getString(type.getSearchCategory());
    showSearch(query);
  }

  public void onSearchSimilarHotels(@Nullable HotelsFilter filter)
  {
    BookingFilterParams params = mFilterController != null
                                 ? mFilterController.getBookingFilterParams() : null;
    FilterActivity.startForResult(MwmActivity.this, filter, params,
                                  REQ_CODE_SHOW_SIMILAR_HOTELS);
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                         @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (requestCode != REQ_CODE_LOCATION_PERMISSION || grantResults.length == 0)
      return;

    PermissionsResult result = PermissionsUtils.computePermissionsResult(permissions, grantResults);
    if (result.isLocationGranted())
      myPositionClick();
  }

  @Override
  public void onSubwayLayerSelected()
  {
    mToggleMapLayerController.toggleMode(Mode.SUBWAY);
  }

  @Override
  public void onTrafficLayerSelected()
  {
    mToggleMapLayerController.toggleMode(Mode.TRAFFIC);
  }

  @Override
  public void onImportStarted(@NonNull String serverId)
  {
    // Do nothing by default.
  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {
    if (!successful)
      return;

    Toast.makeText(this, R.string.guide_downloaded_title, Toast.LENGTH_LONG).show();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_GUIDEDOWNLOADTOAST_SHOWN);
  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups)
  {
    //TODO(@alexzatsepin): Implement me if necessary
  }

  @Override
  public void onCustomPropertiesReceived(boolean successful,
                                         @NonNull List<CatalogCustomProperty> properties)
  {
    //TODO(@alexzatsepin): Implement me if necessary
  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {
    //TODO(@alexzatsepin): Implement me if necessary
  }

  @Override
  public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult, @NonNull String description,
                               long originCategoryId, long resultCategoryId)
  {
    //TODO(@alexzatsepin): Implement me if necessary
  }

  @Override
  protected void onNewIntent(Intent intent)
  {
    super.onNewIntent(intent);
    setIntent(intent);
    processIntent(intent);
  }

  private boolean processIntent(Intent intent)
  {
    if (intent == null)
      return false;

    final Notifier notifier = Notifier.from(getApplication());
    notifier.processNotificationExtras(intent);

    if (intent.hasExtra(EXTRA_TASK))
    {
      addTask(intent);
      return true;
    }

    HotelsFilter filter = intent.getParcelableExtra(FilterActivity.EXTRA_FILTER);
    BookingFilterParams params = intent.getParcelableExtra(FilterActivity.EXTRA_FILTER_PARAMS);
    if (mFilterController != null)
    {
      mFilterController.show(filter != null || params != null
                             || !TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()), true);
      mFilterController.setFilterAndParams(filter, params);
      return filter != null || params != null;
    }

    return false;
  }

  private void addTask(Intent intent)
  {
    if (intent != null &&
        !intent.getBooleanExtra(EXTRA_CONSUMED, false) &&
        intent.hasExtra(EXTRA_TASK) &&
        ((intent.getFlags() & Intent.FLAG_ACTIVITY_LAUNCHED_FROM_HISTORY) == 0))
    {
      final MapTask mapTask = (MapTask) intent.getSerializableExtra(EXTRA_TASK);
      mTasks.add(mapTask);
      intent.removeExtra(EXTRA_TASK);

      if (isMapRendererActive())
        runTasks();

      // mark intent as consumed
      intent.putExtra(EXTRA_CONSUMED, true);
    }
  }

  private boolean isMapRendererActive()
  {
    return mMapFragment != null && MapFragment.nativeIsEngineCreated()
           && mMapFragment.isContextCreated();
  }

  private void addTask(MapTask task)
  {
    mTasks.add(task);
    if (isMapRendererActive())
      runTasks();
  }

  @CallSuper
  @Override
  protected void onResume()
  {
    super.onResume();
    mPlacePageRestored = mPlacePage.getState() != State.HIDDEN;
    mSearchController.refreshToolbar();
    mMainMenu.onResume(new Runnable()
    {
      @Override
      public void run()
      {
        if (Framework.nativeIsInChoosePositionMode())
        {
          UiUtils.show(mPositionChooser);
          setFullscreen(true);
        }
      }
    });
    if (mOnmapDownloader != null)
      mOnmapDownloader.onResume();
    if (mNavigationController != null)
      mNavigationController.onResume();
    if (mNavAnimationController != null)
      mNavAnimationController.onResume();
    mPlacePage.onActivityResume();
  }

  @Override
  public void recreate()
  {
    // Explicitly destroy context before activity recreation.
    if (mMapFragment != null)
      mMapFragment.destroyContext();
    super.recreate();
  }

  @Override
  protected void onResumeFragments()
  {
    super.onResumeFragments();
    RoutingController.get().restore();
    mPlacePage.restore();

    if (!LikesManager.INSTANCE.isNewUser() && Counters.isShowReviewForOldUser())
    {
      LikesManager.INSTANCE.showRateDialogForOldUser(this);
      Counters.setShowReviewForOldUser(false);
    }
    else
    {
      LikesManager.INSTANCE.showDialogs(this);
    }
  }

  @Override
  protected void onPause()
  {
    TtsPlayer.INSTANCE.stop();
    LikesManager.INSTANCE.cancelDialogs();
    if (mOnmapDownloader != null)
      mOnmapDownloader.onPause();
    mPlacePage.onActivityPause();
    super.onPause();
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    SearchEngine.INSTANCE.addListener(this);
    Framework.nativeSetMapObjectListener(this);
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addCatalogListener(this);
    RoutingController.get().attach(this);
    if (MapFragment.nativeIsEngineCreated())
      LocationHelper.INSTANCE.attach(this);
    mToggleMapLayerController.attachCore();
    if (mNavigationController != null)
      TrafficManager.INSTANCE.attach(mNavigationController);
    mPlacePage.onActivityStarted();
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    SearchEngine.INSTANCE.removeListener(this);
    Framework.nativeRemoveMapObjectListener();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeCatalogListener(this);
    LocationHelper.INSTANCE.detach(!isFinishing());
    RoutingController.get().detach();
    TrafficManager.INSTANCE.detachAll();
    mToggleMapLayerController.detachCore();
    mPlacePage.onActivityStopped();
  }

  @Override
  protected void onDestroy()
  {
    super.onDestroy();
    if (mAdsRemovalPurchaseController != null)
      mAdsRemovalPurchaseController.destroy();
    if (mBookmarkPurchaseController != null)
      mBookmarkPurchaseController.destroy();
    if (mNavigationController != null)
      mNavigationController.destroy();
  }

  @Override
  public void onBackPressed()
  {
    if (getCurrentMenu().close(true))
    {
      mFadeView.fadeOut();
      return;
    }

    if (mSearchController.hide())
    {
      SearchEngine.INSTANCE.cancelInteractiveSearch();
      if (mFilterController != null)
        mFilterController.resetFilter();
      if (mSearchController != null)
        mSearchController.clear();
      return;
    }

    boolean isRoutingCancelled = RoutingController.get().cancel();
    if (isRoutingCancelled)
    {
      @Framework.RouterType
      int type = RoutingController.get().getLastRouterType();
      Statistics.INSTANCE.trackRoutingFinish(true, type,
                                             TrafficManager.INSTANCE.isEnabled());
    }

    if (!closePlacePage() && !closeSidePanel() && !isRoutingCancelled
        && !closePositionChooser())
    {
      try
      {
        super.onBackPressed();
      } catch (IllegalStateException e)
      {
        // Sometimes this can be called after onSaveState() for unknown reason.
      }
    }
  }

  private boolean interceptBackPress()
  {
    final FragmentManager manager = getSupportFragmentManager();
    for (String tag : DOCKED_FRAGMENTS)
    {
      final Fragment fragment = manager.findFragmentByTag(tag);
      if (fragment != null && fragment.isResumed() && fragment instanceof OnBackPressListener)
        return ((OnBackPressListener) fragment).onBackPressed();
    }

    return false;
  }

  private void removeFragmentImmediate(Fragment fragment)
  {
    FragmentManager fm = getSupportFragmentManager();
    if (fm.isDestroyed())
      return;

    fm.beginTransaction()
      .remove(fragment)
      .commitAllowingStateLoss();
    fm.executePendingTransactions();
  }

  private boolean removeCurrentFragment(boolean animate)
  {
    for (String tag : DOCKED_FRAGMENTS)
      if (removeFragment(tag, animate))
        return true;

    return false;
  }

  private boolean removeFragment(String className, boolean animate)
  {
    if (animate && mPanelAnimator == null)
      animate = false;

    final Fragment fragment = getSupportFragmentManager().findFragmentByTag(className);
    if (fragment == null)
      return false;

    if (animate)
      mPanelAnimator.hide(new Runnable()
      {
        @Override
        public void run()
        {
          removeFragmentImmediate(fragment);
        }
      });
    else
      removeFragmentImmediate(fragment);

    return true;
  }

  @Override
  public void onMapObjectActivated(final MapObject object)
  {
    if (MapObject.isOfType(MapObject.API_POINT, object))
    {
      final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
      if (request == null)
        return;

      request.setPointData(object.getLat(), object.getLon(), object.getTitle(), object.getApiId());
      object.setSubtitle(request.getCallerName(MwmApplication.get()).toString());
    }

    setFullscreen(false);

    mPlacePage.setMapObject(object, true, () -> {
      if (!mPlacePageRestored)
      {
        if (object != null)
        {
          mPlacePage.setState(object.isExtendedView() ? State.DETAILS : State.PREVIEW);
          Framework.logLocalAdsEvent(Framework.LOCAL_ADS_EVENT_OPEN_INFO, object);
        }
      }
      mPlacePageRestored = false;
    });
    if (mPlacePageTracker != null)
      mPlacePageTracker.setMapObject(object);

    if (UiUtils.isVisible(mFadeView))
      mFadeView.fadeOut();
  }

  @Override
  public void onDismiss(boolean switchFullScreenMode)
  {
    if (switchFullScreenMode)
    {
      if ((mPanelAnimator != null && mPanelAnimator.isVisible()) ||
           UiUtils.isVisible(mSearchController.getToolbar()))
        return;

      setFullscreen(!mIsFullscreen);
    }
    else
    {
      mPlacePage.hide();
    }
  }

  private BaseMenu getCurrentMenu()
  {
    return (RoutingController.get().isNavigating() && mNavigationController != null
            ? mNavigationController.getNavMenu() : mMainMenu);
  }

  private void setFullscreen(boolean isFullscreen)
  {
    if (RoutingController.get().isNavigating()
            || RoutingController.get().isBuilding()
            || RoutingController.get().isPlanning())
      return;

    mIsFullscreen = isFullscreen;
    final BaseMenu menu = getCurrentMenu();

    if (isFullscreen)
    {
      if (menu.isAnimating())
        return;

      mIsFullscreenAnimating = true;
      Animations.disappearSliding(menu.getFrame(), Animations.BOTTOM, new Runnable()
      {
        @Override
        public void run()
        {
          final int menuHeight = menu.getFrame().getHeight();
          adjustBottomWidgets(menuHeight);

          mIsFullscreenAnimating = false;
          if (mIsAppearMenuLater)
          {
            appearMenu(menu);
            mIsAppearMenuLater = false;
          }
        }
      });
      if (mNavAnimationController != null)
        mNavAnimationController.disappearZoomButtons();
      if (mNavMyPosition != null)
        mNavMyPosition.hide();
      mToggleMapLayerController.hide();
    }
    else
    {
      if (mPlacePage.isHidden() && mNavAnimationController != null)
        mNavAnimationController.appearZoomButtons();
      if (!mIsFullscreenAnimating)
        appearMenu(menu);
      else
        mIsAppearMenuLater = true;
    }
  }

  private void appearMenu(BaseMenu menu)
  {
    appearMenuFrame(menu);
    showNavMyPositionBtn();
    mToggleMapLayerController.applyLastActiveMode();
  }

  private void showNavMyPositionBtn()
  {
    if (mNavMyPosition != null)
      mNavMyPosition.show();
  }

  private void appearMenuFrame(@NonNull BaseMenu menu)
  {
    Animations.appearSliding(menu.getFrame(), Animations.BOTTOM, new Runnable()
    {
      @Override
      public void run()
      {
        adjustBottomWidgets(0);
      }
    });
  }

  @Override
  public void onPreviewVisibilityChanged(boolean isVisible)
  {
    if (mVisibleRectMeasurer != null)
      mVisibleRectMeasurer.setPreviewVisible(isVisible);

    if (isVisible)
    {
      if (mMainMenu.isAnimating() || mMainMenu.isOpen())
        UiThread.runLater(new Runnable()
        {
          @Override
          public void run()
          {
            if (mMainMenu.close(true))
              mFadeView.fadeOut();
          }
        }, BaseMenu.ANIMATION_DURATION * 2);
    }
    else
    {
      Framework.nativeDeactivatePopup();
      mPlacePage.setMapObject(null, false, null);
      if (mPlacePageTracker != null)
        mPlacePageTracker.onHidden();
    }
  }

  @Override
  public void onPlacePageVisibilityChanged(boolean isVisible)
  {
    if (mVisibleRectMeasurer != null)
      mVisibleRectMeasurer.setPlacePageVisible(isVisible);

    Statistics.INSTANCE.trackEvent(isVisible ? Statistics.EventName.PP_OPEN
                                             : Statistics.EventName.PP_CLOSE);
    AlohaHelper.logClick(isVisible ? AlohaHelper.PP_OPEN
                                   : AlohaHelper.PP_CLOSE);
    if (mPlacePageTracker != null && isVisible)
    {
      mPlacePageTracker.onOpened();
    }
  }

  @Override
  public void onProgress(float translationX, float translationY)
  {
    if (mNavAnimationController != null)
      mNavAnimationController.onPlacePageMoved(translationY);
    if (mPlacePageTracker != null)
      mPlacePageTracker.onMove();
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.nav_zoom_in:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ZOOM_IN);
      AlohaHelper.logClick(AlohaHelper.ZOOM_IN);
      MapFragment.nativeScalePlus();
      break;
    case R.id.nav_zoom_out:
      Statistics.INSTANCE.trackEvent(Statistics.EventName.ZOOM_OUT);
      AlohaHelper.logClick(AlohaHelper.ZOOM_OUT);
      MapFragment.nativeScaleMinus();
      break;
    }
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    return mPlacePage.hideOnTouch() || (mMapFragment != null && mMapFragment.onTouch(view, event));
  }

  @Override
  public void customOnNavigateUp()
  {
    if (removeCurrentFragment(true))
    {
      InputUtils.hideKeyboard(mMainMenu.getFrame());
      mSearchController.refreshToolbar();
    }
  }

  public interface MapTask extends Serializable
  {
    boolean run(@NonNull MwmActivity target);
  }

  public static class ImportBookmarkCatalogueTask implements MapTask
  {
    private static final long serialVersionUID = 5363722491377575159L;

    @NonNull
    private final String mUrl;

    public ImportBookmarkCatalogueTask(@NonNull String url)
    {
      mUrl = url;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      BookmarkCategoriesActivity.startForResult(target, BookmarksPageFactory.DOWNLOADED.ordinal(), mUrl);
      return true;
    }
  }

  public static class OpenUrlTask implements MapTask
  {
    private static final long serialVersionUID = 1L;
    private final String mUrl;

    public OpenUrlTask(String url)
    {
      Utils.checkNotNull(url);
      mUrl = url;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      final @ParsedUrlMwmRequest.ParsingResult int result = Framework.nativeParseAndSetApiUrl(mUrl);
      switch (result)
      {
      case ParsedUrlMwmRequest.RESULT_INCORRECT:
        // TODO: Kernel recognizes "mapsme://", "mwm://" and "mapswithme://" schemas only!!!
        return MapFragment.nativeShowMapForUrl(mUrl);

      case ParsedUrlMwmRequest.RESULT_MAP:
        return MapFragment.nativeShowMapForUrl(mUrl);

      case ParsedUrlMwmRequest.RESULT_ROUTE:
        final ParsedRoutingData data = Framework.nativeGetParsedRoutingData();
        RoutingController.get().setRouterType(data.mRouterType);
        final RoutePoint from = data.mPoints[0];
        final RoutePoint to = data.mPoints[1];
        RoutingController.get().prepare(MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                                                  from.mName, "", from.mLat, from.mLon),
                                        MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                                                  to.mName, "", to.mLat, to.mLon), true);
        return true;
      case ParsedUrlMwmRequest.RESULT_SEARCH:
        final ParsedSearchRequest request = Framework.nativeGetParsedSearchRequest();
        SearchActivity.start(target, request.mQuery, request.mLocale, request.mIsSearchOnMap,
                             null, null);
        return true;
      case ParsedUrlMwmRequest.RESULT_LEAD:
        return true;
      }

      return false;
    }
  }

  public static class ShowCountryTask implements MapTask
  {
    private static final long serialVersionUID = 1L;
    private final String mCountryId;

    public ShowCountryTask(String countryId)
    {
      mCountryId = countryId;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Framework.nativeShowCountry(mCountryId, false);
      return true;
    }
  }

  void adjustCompass(int offsetY)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    int resultOffset = offsetY;
    //If the compass is covered by navigation buttons, we move it beyond the visible screen
    if (mNavAnimationController != null && mNavAnimationController.isConflictWithCompass(offsetY))
    {
      int halfHeight = (int)(UiUtils.dimen(R.dimen.compass_height) * 0.5f);
      int margin = UiUtils.dimen(R.dimen.margin_compass_top)
                   + UiUtils.dimen(R.dimen.nav_frame_padding);
      resultOffset = -(offsetY + halfHeight + margin);
    }

    mMapFragment.setupCompass(resultOffset, true);

    CompassData compass = LocationHelper.INSTANCE.getCompassData();
    if (compass != null)
      MapFragment.nativeCompassUpdated(compass.getMagneticNorth(), compass.getTrueNorth(), true);
  }

  private void adjustBottomWidgets(int offsetY)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    mMapFragment.setupRuler(offsetY, false);
    mMapFragment.setupWatermark(offsetY, true);
  }

  @Override
  public FragmentActivity getActivity()
  {
    return this;
  }

  public MainMenu getMainMenu()
  {
    return mMainMenu;
  }

  @Override
  public void showSearch()
  {
    showSearch("");
  }

  @Override
  public void updateMenu()
  {
    adjustMenuLineFrameVisibility(new Runnable()
    {
      @Override
      public void run()
      {
        if (mNavigationController != null)
        {
          mNavigationController.showSearchButtons(RoutingController.get().isPlanning()
                                                  || RoutingController.get().isBuilt());
        }

        if (RoutingController.get().isNavigating())
        {
          if (mNavigationController != null)
            mNavigationController.show(true);
          mSearchController.hide();
          mMainMenu.setState(MainMenu.State.NAVIGATION, false, mIsFullscreen);
          return;
        }

        if (mIsTabletLayout)
        {
          mMainMenu.setEnabled(MainMenu.Item.POINT_TO_POINT, !RoutingController.get().isPlanning());
          mMainMenu.setEnabled(MainMenu.Item.SEARCH, !RoutingController.get().isWaitingPoiPick());
        }
        else if (RoutingController.get().isPlanning())
        {
          mMainMenu.setState(MainMenu.State.ROUTE_PREPARE, false, mIsFullscreen);
          return;
        }

        mMainMenu.setState(MainMenu.State.MENU, false, mIsFullscreen);
      }
    });
  }

  @Override
  @Nullable
  public PurchaseController<PurchaseCallback> getAdsRemovalPurchaseController()
  {
    return mAdsRemovalPurchaseController;
  }

  @Override
  public void onAdsRemovalActivation()
  {
    closePlacePage();
  }

  private void adjustMenuLineFrameVisibility(@Nullable final Runnable completion)
  {
    final RoutingController controller = RoutingController.get();

    if (controller.isBuilt() || controller.isTaxiRequestHandled())
    {
      showLineFrame();

      if (completion != null)
        completion.run();
      return;
    }

    if (controller.isPlanning() || controller.isBuilding() || controller.isErrorEncountered())
    {
      if (showAddStartOrFinishFrame(controller, true))
      {
        if (completion != null)
          completion.run();
        return;
      }

      showLineFrame(false, new Runnable()
      {
        @Override
        public void run()
        {
          final int menuHeight = getCurrentMenu().getFrame().getHeight();
          adjustBottomWidgets(menuHeight);
          if (completion != null)
            completion.run();
        }
      });

      return;
    }

    hideRoutingActionFrame();
    showLineFrame();

    if (completion != null)
      completion.run();
  }

  private boolean showAddStartOrFinishFrame(@NonNull RoutingController controller,
                                            boolean showFrame)
  {
    // S - start, F - finish, L - my position
    // -S-F-L -> Start
    // -S-F+L -> Finish
    // -S+F-L -> Start
    // -S+F+L -> Start + Use
    // +S-F-L -> Finish
    // +S-F+L -> Finish
    // +S+F-L -> Hide
    // +S+F+L -> Hide

    MapObject myPosition = LocationHelper.INSTANCE.getMyPosition();

    if (myPosition != null && !controller.hasEndPoint())
    {
      showAddFinishFrame();
      if (showFrame)
        showLineFrame();
      return true;
    }
    if (!controller.hasStartPoint())
    {
      showAddStartFrame();
      if (showFrame)
        showLineFrame();
      return true;
    }
    if (!controller.hasEndPoint())
    {
      showAddFinishFrame();
      if (showFrame)
        showLineFrame();
      return true;
    }

    return false;
  }

  private void showAddStartFrame()
  {
    if (!mIsTabletLayout)
    {
      mRoutingPlanInplaceController.showAddStartFrame();
      return;
    }

    RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
    if (fragment != null)
      fragment.showAddStartFrame();
  }

  private void showAddFinishFrame()
  {
    if (!mIsTabletLayout)
    {
      mRoutingPlanInplaceController.showAddFinishFrame();
      return;
    }

    RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
    if (fragment != null)
      fragment.showAddFinishFrame();
  }

  private void hideRoutingActionFrame()
  {
    if (!mIsTabletLayout)
    {
      mRoutingPlanInplaceController.hideActionFrame();
      return;
    }

    RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
    if (fragment != null)
      fragment.hideActionFrame();
  }

  private void showLineFrame()
  {
    showLineFrame(true, new Runnable()
    {
      @Override
      public void run()
      {
        adjustBottomWidgets(0);
      }
    });
  }

  private void showLineFrame(boolean show, @Nullable Runnable completion)
  {
    mMainMenu.showLineFrame(show, completion);
  }

  private void setNavButtonsTopLimit(int limit)
  {
    if (mNavAnimationController == null)
      return;

    mNavAnimationController.setTopLimit(limit);
  }

  @Override
  public void onRoutingPlanStartAnimate(boolean show)
  {
    if (mNavAnimationController == null)
      return;

    mNavAnimationController.setTopLimit(!show ? 0 : mRoutingPlanInplaceController.getHeight());
    mNavAnimationController.setBottomLimit(!show ? 0 : getCurrentMenu().getFrame().getHeight());
    adjustCompassAndTraffic(!show ? UiUtils.getStatusBarHeight(getApplicationContext())
                                  : mRoutingPlanInplaceController.getHeight());
  }

  @Override
  public void showRoutePlan(boolean show, @Nullable Runnable completionListener)
  {
    if (show)
    {
      mSearchController.hide();

      if (mIsTabletLayout)
      {
        replaceFragment(RoutingPlanFragment.class, null, completionListener);
        if (mRestoreRoutingPlanFragmentNeeded && mSavedForTabletState != null)
        {
          RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
          if (fragment != null)
            fragment.restoreRoutingPanelState(mSavedForTabletState);
        }
        showAddStartOrFinishFrame(RoutingController.get(), false);
        int width = UiUtils.dimen(R.dimen.panel_width);
        adjustTraffic(width, UiUtils.getStatusBarHeight(getApplicationContext()));
        if (mNavigationController != null)
          mNavigationController.adjustSearchButtons(width);
      }
      else
      {
        mRoutingPlanInplaceController.show(true);
        if (completionListener != null)
          completionListener.run();
      }
    }
    else
    {
      if (mIsTabletLayout)
      {
        adjustCompassAndTraffic(UiUtils.getStatusBarHeight(getApplicationContext()));
        setNavButtonsTopLimit(0);
        if (mNavigationController != null)
          mNavigationController.adjustSearchButtons(0);
      }
      else
      {
        mRoutingPlanInplaceController.show(false);
      }

      closeAllFloatingPanels();

      if (mNavigationController != null)
        mNavigationController.resetSearchWheel();

      if (completionListener != null)
        completionListener.run();

      updateSearchBar();
    }

    mPlacePage.refreshViews();
  }

  private void adjustCompassAndTraffic(final int offsetY)
  {
    addTask(new MapTask()
    {
      @Override
      public boolean run(@NonNull MwmActivity target)
      {
        adjustCompass(offsetY);
        return true;
      }
    });
    adjustTraffic(0, offsetY);
  }

  private void adjustTraffic(int offsetX, int offsetY)
  {
    mToggleMapLayerController.adjust(offsetX, offsetY);
  }

  @Override
  public void onSearchVisibilityChanged(boolean visible)
  {
    if (mNavAnimationController == null)
      return;

    int toolbarHeight = mSearchController.getToolbar().getHeight();
    int offset = mRoutingPlanInplaceController != null
                 && mRoutingPlanInplaceController.getHeight() > 0
                 ? mRoutingPlanInplaceController.getHeight() : UiUtils.getStatusBarHeight(this);

    adjustCompassAndTraffic(visible ? toolbarHeight : offset);
    setNavButtonsTopLimit(visible ? toolbarHeight : 0);
    if (mFilterController != null)
    {
      boolean show = visible && !TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery())
                     && !RoutingController.get().isNavigating();
      mFilterController.show(show, true);
      mMainMenu.show(!show);
    }
  }

  @Override
  public void onResultsUpdate(SearchResult[] results, long timestamp, boolean isHotel)
  {
    if (mFilterController != null)
      mFilterController.updateFilterButtonVisibility(isHotel);
  }

  @Override
  public void onResultsEnd(long timestamp)
  {
  }

  @Override
  public void showNavigation(boolean show)
  {
    mPlacePage.refreshViews();
    if (mNavigationController != null)
      mNavigationController.show(show);
    refreshFade();
    if (mOnmapDownloader != null)
      mOnmapDownloader.updateState(false);
    if (show)
    {
      mSearchController.clear();
      mSearchController.hide();
      if (mFilterController != null)
        mFilterController.show(false, true);
    }
  }

  @Override
  public void updateBuildProgress(int progress, @Framework.RouterType int router)
  {
    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.updateBuildProgress(progress, router);
    }
    else
    {
      mRoutingPlanInplaceController.updateBuildProgress(progress, router);
    }
  }

  @Override
  public void onTaxiInfoReceived(@NonNull TaxiInfo info)
  {
    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.showTaxiInfo(info);
    }
    else
    {
      mRoutingPlanInplaceController.showTaxiInfo(info);
    }
  }

  @Override
  public void onTaxiError(@NonNull TaxiManager.ErrorCode code)
  {
    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.showTaxiError(code);
    }
    else
    {
      mRoutingPlanInplaceController.showTaxiError(code);
    }
  }

  @Override
  public void onNavigationCancelled()
  {
    if (mNavigationController != null)
      mNavigationController.stop(this);
    updateSearchBar();
    ThemeSwitcher.restart(isMapRendererActive());
  }

  @Override
  public void onNavigationStarted()
  {
    ThemeSwitcher.restart(isMapRendererActive());
  }

  @Override
  public void onAddedStop()
  {
    closePlacePage();
  }

  @Override
  public void onRemovedStop()
  {
    closePlacePage();
  }

  @Override
  public void onBuiltRoute()
  {
    if (!RoutingController.get().isPlanning())
      return;

    if (mNavigationController != null)
      mNavigationController.resetSearchWheel();
  }

  @Override
  public boolean isSubwayEnabled()
  {
    return SubwayManager.from(this).isEnabled();
  }

  private void updateSearchBar()
  {
    if (!TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()))
      mSearchController.refreshToolbar();
  }

  @Override
  public void onMyPositionModeChanged(int newMode)
  {
    if (mNavMyPosition != null)
      mNavMyPosition.update(newMode);

    RoutingController controller = RoutingController.get();
    if (controller.isPlanning())
      showAddStartOrFinishFrame(controller, true);
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    if (!mPlacePage.isHidden())
      mPlacePage.refreshLocation(location);

    if (!RoutingController.get().isNavigating())
      return;

    if (mNavigationController != null)
      mNavigationController.update(Framework.nativeGetRouteFollowingInfo());

    TtsPlayer.INSTANCE.playTurnNotifications(getApplicationContext());
  }

  @Override
  public void onCompassUpdated(@NonNull CompassData compass)
  {
    MapFragment.nativeCompassUpdated(compass.getMagneticNorth(), compass.getTrueNorth(), false);
    mPlacePage.refreshAzimuth(compass.getNorth());
    if (mNavigationController != null)
      mNavigationController.updateNorth(compass.getNorth());
  }

  @Override
  public void onLocationError()
  {
    if (mLocationErrorDialogAnnoying)
      return;

    Intent intent = TargetUtils.makeAppSettingsLocationIntent(getApplicationContext());
    if (intent == null)
      return;
    showLocationErrorDialog(intent);
  }

  @Override
  public void onTranslationChanged(float translation)
  {
    if (mNavigationController != null)
      mNavigationController.updateSearchButtonsTranslation(translation);
  }

  @Override
  public void onFadeInZoomButtons()
  {
    if (mNavigationController != null
        && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating()))
      mNavigationController.fadeInSearchButtons();
  }

  @Override
  public void onFadeOutZoomButtons()
  {
    if (mNavigationController != null
        && (RoutingController.get().isPlanning() || RoutingController.get().isNavigating()))
    {
      if (UiUtils.isLandscape(this))
        mToggleMapLayerController.hide();
      else
        mNavigationController.fadeOutSearchButtons();
    }
  }

  private void showLocationErrorDialog(@NonNull final Intent intent)
  {
    if (mLocationErrorDialog != null && mLocationErrorDialog.isShowing())
      return;

    mLocationErrorDialog = new AlertDialog.Builder(this)
        .setTitle(R.string.enable_location_services)
        .setMessage(R.string.location_is_disabled_long_text)
        .setNegativeButton(R.string.close, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            mLocationErrorDialogAnnoying = true;
          }
        })
        .setOnCancelListener(new DialogInterface.OnCancelListener()
        {
          @Override
          public void onCancel(DialogInterface dialog)
          {
            mLocationErrorDialogAnnoying = true;
          }
        })
        .setPositiveButton(R.string.connection_settings, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            startActivity(intent);
          }
        }).show();
  }

  @Override
  public void onLocationNotFound()
  {
    showLocationNotFoundDialog();
  }

  @Override
  public void onRoutingFinish()
  {
    Statistics.INSTANCE.trackRoutingFinish(false, RoutingController.get().getLastRouterType(),
                                           TrafficManager.INSTANCE.isEnabled());
  }

  private void showLocationNotFoundDialog()
  {
    String message = String.format("%s\n\n%s", getString(R.string.current_location_unknown_message),
                                   getString(R.string.current_location_unknown_title));

    DialogInterface.OnClickListener stopClickListener = (dialog, which) ->
    {
      LocationHelper.INSTANCE.setStopLocationUpdateByUser(true);
    };

    DialogInterface.OnClickListener continueClickListener = (dialog, which) ->
    {
      if (!LocationHelper.INSTANCE.isActive())
        LocationHelper.INSTANCE.start();
    };

    new AlertDialog.Builder(this)
        .setMessage(message)
        .setNegativeButton(R.string.current_location_unknown_stop_button, stopClickListener)
        .setPositiveButton(R.string.current_location_unknown_continue_button, continueClickListener)
        .show();
  }

  @Override
  public void onUseMyPositionAsStart()
  {
    RoutingController.get().setStartPoint(LocationHelper.INSTANCE.getMyPosition());
  }

  @Override
  public void onSearchRoutePoint(@RoutePointInfo.RouteMarkType int pointType)
  {
    if (mNavigationController != null)
    {
      RoutingController.get().waitForPoiPick(pointType);
      mNavigationController.performSearchClick();
      Statistics.INSTANCE.trackRoutingTooltipEvent(pointType, true);
    }
  }

  @Override
  public void onRoutingStart()
  {
    @Framework.RouterType
    int routerType = RoutingController.get().getLastRouterType();
    Statistics.INSTANCE.trackRoutingStart(routerType, TrafficManager.INSTANCE.isEnabled());
    closeMenu(() -> RoutingController.get().start());
  }

  @Override
  public void onBookmarksLoadingStarted()
  {
    // Do nothing
  }

  @Override
  public void onBookmarksLoadingFinished()
  {
    // Do nothing
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    Utils.toastShortcut(MwmActivity.this, success ? R.string.load_kmz_successful :
        R.string.load_kmz_failed);
  }

  @Override
  public void onSearchClearClick()
  {
    if (mFilterController != null)
      mFilterController.resetFilter();
  }

  @Override
  public void onSearchUpClick(@Nullable String query)
  {
    showSearch(query);
  }

  @Override
  public void onSearchQueryClick(@Nullable String query)
  {
    showSearch(query);
  }

  public static class ShowDialogTask implements MapTask
  {
    @NonNull
    private String mDialogName;

    public ShowDialogTask(@NonNull String dialogName)
    {
      mDialogName = dialogName;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Fragment f = target.getSupportFragmentManager().findFragmentByTag(mDialogName);
      if (f != null)
        return true;

      final DialogFragment fragment = (DialogFragment) Fragment.instantiate(target, mDialogName);
      fragment.show(target.getSupportFragmentManager(), mDialogName);
      return true;
    }
  }

  public static abstract class BaseUserMarkTask implements MapTask
  {
    private static final long serialVersionUID = 1L;

    final long mCategoryId;
    final long mId;

    BaseUserMarkTask(long categoryId, long id)
    {
      mCategoryId = categoryId;
      mId = id;
    }
  }

  public static class ShowBookmarkTask extends BaseUserMarkTask
  {
    public ShowBookmarkTask(long categoryId, long bookmarkId)
    {
      super(categoryId, bookmarkId);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      BookmarkManager.INSTANCE.showBookmarkOnMap(mId);
      return true;
    }
  }

  public static class ShowTrackTask extends BaseUserMarkTask
  {
    public ShowTrackTask(long categoryId, long trackId)
    {
      super(categoryId, trackId);
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      Framework.nativeShowTrackRect(mId);
      return true;
    }
  }

  public static class ShowPointTask implements MapTask
  {
    private final double mLat;
    private final double mLon;

    public ShowPointTask(double lat, double lon)
    {
      mLat = lat;
      mLon = lon;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      MapFragment.nativeShowMapForUrl(String.format(Locale.US,
                                                    "mapsme://map?ll=%f,%f", mLat, mLon));
      return true;
    }
  }

  public static class BuildRouteTask implements MapTask
  {
    private final double mLatTo;
    private final double mLonTo;
    @Nullable
    private final Double mLatFrom;
    @Nullable
    private final Double mLonFrom;
    @Nullable
    private final String mSaddr;
    @Nullable
    private final String mDaddr;
    private final String mRouter;

    @NonNull
    private static MapObject fromLatLon(double lat, double lon, @Nullable String addr)
    {
      return MapObject.createMapObject(FeatureId.EMPTY, MapObject.API_POINT,
                                       TextUtils.isEmpty(addr) ? "" : addr, "", lat, lon);
    }

    public BuildRouteTask(double latTo, double lonTo, @Nullable String router)
    {
      this(latTo, lonTo, null, null, null, null, router);
    }

    public BuildRouteTask(double latTo, double lonTo, @Nullable String saddr,
                   @Nullable Double latFrom, @Nullable Double lonFrom, @Nullable String daddr)
    {
      this(latTo, lonTo, saddr, latFrom, lonFrom, daddr, null);
    }

    public BuildRouteTask(double latTo, double lonTo, @Nullable String saddr,
                   @Nullable Double latFrom, @Nullable Double lonFrom, @Nullable String daddr,
                   @Nullable String router)
    {
      mLatTo = latTo;
      mLonTo = lonTo;
      mLatFrom = latFrom;
      mLonFrom = lonFrom;
      mSaddr = saddr;
      mDaddr = daddr;
      mRouter = router;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      @Framework.RouterType int routerType = -1;
      if (!TextUtils.isEmpty(mRouter))
      {
        switch (mRouter)
        {
          case "vehicle":
            routerType = Framework.ROUTER_TYPE_VEHICLE;
            break;
          case "pedestrian":
            routerType = Framework.ROUTER_TYPE_PEDESTRIAN;
            break;
          case "bicycle":
            routerType = Framework.ROUTER_TYPE_BICYCLE;
            break;
          case "taxi":
            routerType = Framework.ROUTER_TYPE_TAXI;
            break;
          case "transit":
            routerType = Framework.ROUTER_TYPE_TRANSIT;
            break;
        }
      }

      if (mLatFrom != null && mLonFrom != null && routerType >= 0)
      {
        RoutingController.get().prepare(fromLatLon(mLatFrom, mLonFrom, mSaddr),
                                        fromLatLon(mLatTo, mLonTo, mDaddr), routerType,
                                        true /* fromApi */);
      }
      else if (mLatFrom != null && mLonFrom != null)
      {
        RoutingController.get().prepare(fromLatLon(mLatFrom, mLonFrom, mSaddr),
                                        fromLatLon(mLatTo, mLonTo, mDaddr), true /* fromApi */);
      }
      else if (routerType > 0)
      {
        RoutingController.get().prepare(true /* canUseMyPositionAsStart */,
                                        fromLatLon(mLatTo, mLonTo, mDaddr), routerType,
                                        true /* fromApi */);
      }
      else
      {
        RoutingController.get().prepare(true /* canUseMyPositionAsStart */,
                                        fromLatLon(mLatTo, mLonTo, mDaddr), true /* fromApi */);
      }
      return true;
    }
  }

  private static class RestoreRouteTask implements MapTask
  {

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      RoutingController.get().restoreRoute();
      return true;
    }
  }

  public static class ShowUGCEditorTask implements MapTask
  {
    private static final long serialVersionUID = 1636712824900113568L;
    @NonNull
    private final NotificationCandidate.MapObject mMapObject;

    ShowUGCEditorTask(@NonNull NotificationCandidate.MapObject mapObject)
    {
      mMapObject = mapObject;
    }

    @Override
    public boolean run(@NonNull MwmActivity target)
    {
      MapObject mapObject = Framework.nativeGetMapObject(mMapObject);

      if (mapObject == null)
        return false;

      EditParams.Builder builder = EditParams.Builder.fromMapObject(mapObject)
                                                     .setDefaultRating(UGC.RATING_NONE)
                                                     .setFromNotification(true);
      UGCEditorActivity.start(target, builder.build());
      return true;
    }
  }

  private class CurrentPositionClickListener implements OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MY_POSITION);
      AlohaHelper.logClick(AlohaHelper.TOOLBAR_MY_POSITION);

      if (!PermissionsUtils.isLocationGranted())
      {
        if (PermissionsUtils.isLocationExplanationNeeded(MwmActivity.this))
          PermissionsUtils.requestLocationPermission(MwmActivity.this, REQ_CODE_LOCATION_PERMISSION);
        else
          Toast.makeText(MwmActivity.this, R.string.enable_location_services, Toast.LENGTH_SHORT)
               .show();
        return;
      }

      myPositionClick();
    }
  }

  static abstract class AbstractClickMenuDelegate implements ClickMenuDelegate
  {
    @NonNull
    private final MwmActivity mActivity;
    @NonNull
    private final MainMenu.Item mItem;

    AbstractClickMenuDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      mActivity = activity;
      mItem = item;
    }

    @NonNull
    public MwmActivity getActivity()
    {
      return mActivity;
    }

    @NonNull
    public MainMenu.Item getItem()
    {
      return mItem;
    }

    @Override
    public final void onMenuItemClick()
    {
      TipsApi api = TipsApi.requestCurrent(getActivity().getClass());
      LOGGER.d(TAG, "TipsApi = " + api);
      if (getItem() == api.getSiblingMenuItem())
        api.createClickInterceptor().onInterceptClick(getActivity());
      else
        onMenuItemClickInternal();
    }

    public abstract void onMenuItemClickInternal();
  }

  public static class MenuClickDelegate extends AbstractClickMenuDelegate
  {
    public MenuClickDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      if (!getActivity().mMainMenu.isOpen())
      {
        Statistics.INSTANCE.trackToolbarClick(getItem());
        if (getActivity().mPlacePage.isDocked() && getActivity().closePlacePage())
          return;

        if (getActivity().closeSidePanel())
          return;
      }
      getActivity().toggleMenu();
    }
  }

  public static class AddPlaceDelegate extends AbstractClickMenuDelegate
  {
    public AddPlaceDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarMenu(getItem());
      getActivity().closePlacePage();
      if (getActivity().mIsTabletLayout)
        getActivity().closeSidePanel();
      getActivity().closeMenu(() -> getActivity().showPositionChooser(false, false));
    }
  }

  public static class SearchClickDelegate extends AbstractClickMenuDelegate
  {
    public SearchClickDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarClick(getItem());
      RoutingController.get().cancel();
      getActivity().closeMenu(() -> getActivity().showSearch(getActivity().mSearchController.getQuery()));
    }
  }

  public static class SettingsDelegate extends AbstractClickMenuDelegate
  {
    public SettingsDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarMenu(getItem());
      Intent intent = new Intent(getActivity(), SettingsActivity.class);
      getActivity().closeMenu(() -> getActivity().startActivity(intent));
    }
  }

  public static class DownloadGuidesDelegate extends AbstractClickMenuDelegate
  {
    public DownloadGuidesDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarMenu(getItem());
      int requestCode = BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY;
      getActivity().closeMenu(() -> BookmarksCatalogActivity.startForResult(getActivity(), requestCode));
    }
  }

  public static class DownloadMapsDelegate extends AbstractClickMenuDelegate
  {
    public DownloadMapsDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarMenu(getItem());
      RoutingController.get().cancel();
      getActivity().closeMenu(() -> getActivity().showDownloader(false));
    }
  }

  public static class BookmarksDelegate extends AbstractClickMenuDelegate
  {
    public BookmarksDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarClick(getItem());
      getActivity().closeMenu(getActivity()::showBookmarks);
    }
  }

  public static class ShareMyLocationDelegate extends AbstractClickMenuDelegate
  {
    public ShareMyLocationDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarMenu(getItem());
      getActivity().closeMenu(getActivity()::shareMyLocation);
    }
  }

  public static class DiscoveryDelegate extends AbstractClickMenuDelegate
  {
    public DiscoveryDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarClick(getItem());
      getActivity().showDiscovery();
    }
  }

  public static class PointToPointDelegate extends AbstractClickMenuDelegate
  {
    public PointToPointDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarClick(getItem());
      getActivity().startLocationToPoint(null, false);
    }
  }
}
