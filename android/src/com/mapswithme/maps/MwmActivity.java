package com.mapswithme.maps;

import android.annotation.SuppressLint;
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
import android.view.ViewGroup;
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
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.FeatureId;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.downloader.DownloaderActivity;
import com.mapswithme.maps.downloader.DownloaderFragment;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.downloader.MigrationFragment;
import com.mapswithme.maps.downloader.OnmapDownloader;
import com.mapswithme.maps.editor.AuthDialogFragment;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.EditorActivity;
import com.mapswithme.maps.editor.EditorHostFragment;
import com.mapswithme.maps.editor.FeatureCategoryActivity;
import com.mapswithme.maps.editor.ReportFragment;
import com.mapswithme.maps.location.CompassData;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.routing.NavigationController;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.routing.RoutingBottomMenuListener;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.routing.RoutingPlanFragment;
import com.mapswithme.maps.routing.RoutingPlanInplaceController;
import com.mapswithme.maps.search.FloatingSearchToolbarController;
import com.mapswithme.maps.search.HotelsFilter;
import com.mapswithme.maps.search.HotelsFilterView;
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
import com.mapswithme.maps.traffic.TrafficManager;
import com.mapswithme.maps.traffic.widget.TrafficButton;
import com.mapswithme.maps.traffic.widget.TrafficButtonController;
import com.mapswithme.maps.widget.FadeView;
import com.mapswithme.maps.widget.menu.BaseMenu;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.maps.widget.menu.MyPositionButton;
import com.mapswithme.maps.widget.placepage.BasePlacePageAnimationController;
import com.mapswithme.maps.widget.placepage.PlacePageView;
import com.mapswithme.maps.widget.placepage.PlacePageView.State;
import com.mapswithme.util.Animations;
import com.mapswithme.util.BottomSheetHelper;
import com.mapswithme.util.Counters;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.permissions.PermissionsResult;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.PlacePageTracker;
import com.mapswithme.util.statistics.Statistics;

import java.io.File;
import java.io.Serializable;
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
                                 BookmarkManager.BookmarksLoadingListener
{
  public static final String EXTRA_TASK = "map_task";
  public static final String EXTRA_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
  private static final String EXTRA_CONSUMED = "mwm.extra.intent.processed";
  private static final String EXTRA_UPDATE_COUNTRIES = ".extra.update.countries";

  private static final String[] DOCKED_FRAGMENTS = { SearchFragment.class.getName(),
                                                     DownloaderFragment.class.getName(),
                                                     MigrationFragment.class.getName(),
                                                     RoutingPlanFragment.class.getName(),
                                                     EditorHostFragment.class.getName(),
                                                     ReportFragment.class.getName() };
  // Instance state
  private static final String STATE_PP = "PpState";
  private static final String STATE_MAP_OBJECT = "MapObject";
  private static final String EXTRA_LOCATION_DIALOG_IS_ANNOYING = "LOCATION_DIALOG_IS_ANNOYING";

  private static final int LOCATION_REQUEST = 1;

  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private final StoragePathManager mPathManager = new StoragePathManager();

  @Nullable
  private MapFragment mMapFragment;
  @Nullable
  private PlacePageView mPlacePage;

  private RoutingPlanInplaceController mRoutingPlanInplaceController;
  @Nullable
  private NavigationController mNavigationController;

  private MainMenu mMainMenu;

  private PanelAnimator mPanelAnimator;
  @Nullable
  private OnmapDownloader mOnmapDownloader;

  private FadeView mFadeView;

  @Nullable
  private MyPositionButton mNavMyPosition;
  private TrafficButton mTraffic;
  @Nullable
  private NavigationButtonsAnimationController mNavAnimationController;
  @Nullable
  private TrafficButtonController mTrafficButtonController;

  private View mPositionChooser;

  private ViewGroup mRootView;

  @Nullable
  private SearchFilterController mFilterController;

  private boolean mIsFragmentContainer;
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

  @NonNull
  private final OnClickListener mOnMyPositionClickListener = new OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MY_POSITION);
      AlohaHelper.logClick(AlohaHelper.TOOLBAR_MY_POSITION);

      if (!PermissionsUtils.isLocationGranted())
      {
        if (PermissionsUtils.isLocationExplanationNeeded(MwmActivity.this))
          PermissionsUtils.requestLocationPermission(MwmActivity.this, LOCATION_REQUEST);
        else
          Toast.makeText(MwmActivity.this, R.string.enable_location_services, Toast.LENGTH_SHORT)
               .show();
        return;
      }

      myPositionClick();
    }
  };

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
    private VisibleRectListener m_listener;
    private Rect mScreenFullRect = null;
    private Rect mLastVisibleRect = null;
    private boolean mPlacePageVisible = false;

    public VisibleRectMeasurer(VisibleRectListener listener)
    {
      m_listener = listener;
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
      if (mPlacePageVisible && (mPlacePage == null || UiUtils.isHidden(mPlacePage.GetPreview())))
        mPlacePageVisible = false;
      recalculateVisibleRect(mScreenFullRect);
    }

    private void recalculateVisibleRect(Rect r)
    {
      if (r == null)
        return;

      int orientation = MwmActivity.this.getResources().getConfiguration().orientation;

      Rect rect = new Rect(r.left, r.top, r.right, r.bottom);
      if (mPlacePage != null && mPlacePageVisible)
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
        if (m_listener != null)
          m_listener.onVisibleRectChanged(rect);
      }
    }
  }

  private VisibleRectMeasurer mVisibleRectMeasurer;

  public static Intent createShowMapIntent(Context context, String countryId, boolean doAutoDownload)
  {
    return new Intent(context, DownloadResourcesLegacyActivity.class)
               .putExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY, countryId)
               .putExtra(DownloadResourcesLegacyActivity.EXTRA_AUTODOWNLOAD, doAutoDownload);
  }

  public static Intent createUpdateMapsIntent()
  {
    return new Intent(MwmApplication.get(), MwmActivity.class)
               .putExtra(EXTRA_UPDATE_COUNTRIES, true);
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
    return (mIsFragmentContainer ? R.id.fragment_container
                                 : super.getFragmentContentResId());
  }

  @Nullable
  Fragment getFragment(Class<? extends Fragment> clazz)
  {
    if (!mIsFragmentContainer)
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
    return mIsFragmentContainer && getFragment(fragmentClass) != null;
  }

  private void showBookmarks()
  {
    startActivity(new Intent(this, BookmarkCategoriesActivity.class));
  }

  public void showSearch(String query)
  {
    if (mIsFragmentContainer)
    {
      mSearchController.hide();

      final Bundle args = new Bundle();
      args.putString(SearchActivity.EXTRA_QUERY, query);
      if (mFilterController != null)
        args.putParcelable(SearchActivity.EXTRA_HOTELS_FILTER, mFilterController.getFilter());
      replaceFragment(SearchFragment.class, args, null);
    }
    else
    {
      SearchActivity.start(this, query, mFilterController != null ? mFilterController.getFilter() : null);
    }
  }

  public void showEditor()
  {
    // TODO(yunikkk) think about refactoring. It probably should be called in editor.
    Editor.nativeStartEdit();
    Statistics.INSTANCE.trackEditorLaunch(false);
    if (mIsFragmentContainer)
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
    if (mIsFragmentContainer)
    {
      SearchEngine.cancelAllSearches();
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
    mIsFragmentContainer = getResources().getBoolean(R.bool.tabletLayout);

    if (!mIsFragmentContainer && (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP))
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

    setContentView(R.layout.activity_map);
    mIsLaunchByDeepLink = getIntent().getBooleanExtra(EXTRA_LAUNCH_BY_DEEP_LINK, false);
    initViews();

    Statistics.INSTANCE.trackConnectionState();

    mSearchController = new FloatingSearchToolbarController(this);
    mSearchController.setVisibilityListener(this);
    SearchEngine.INSTANCE.addListener(this);

    SharingHelper.prepare();

    //TODO: uncomment after correct visible rect calculation.
    //mVisibleRectMeasurer = new VisibleRectMeasurer(new VisibleRectListener() {
    //  @Override
    //  public void onVisibleRectChanged(Rect rect) {
    //    Framework.nativeSetVisibleRect(rect.left, rect.top, rect.right, rect.bottom);
    //  }
    //});
    //getWindow().getDecorView().addOnLayoutChangeListener(mVisibleRectMeasurer);
    boolean isConsumed = processIntent(getIntent());
    // If the map activity is launched by any incoming intent (deeplink, update maps event, etc)
    // we haven't to try restoring the route.
    if (!isConsumed)
      addTask(new RestoreRouteTask());
  }

  private void initViews()
  {
    initMap();
    initNavigationButtons();

    mPlacePage = (PlacePageView) findViewById(R.id.info_box);
    if (mPlacePage != null)
    {
      mPlacePage.setOnVisibilityChangedListener(this);
      mPlacePage.setOnAnimationListener(this);
      mPlacePageTracker = new PlacePageTracker(mPlacePage);
    }

    if (!mIsFragmentContainer)
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

  private void initFilterViews()
  {
    HotelsFilterView hotelsFilterView = (HotelsFilterView) findViewById(R.id.hotels_filter);
    View frame = findViewById(R.id.filter_frame);
    if (frame != null && hotelsFilterView != null)
    {
      mFilterController = new SearchFilterController(
          frame, hotelsFilterView, new SearchFilterController.DefaultFilterListener()
      {
        @Override
        public void onViewClick()
        {
          showSearch(mSearchController.getQuery());
        }

        @Override
        public void onFilterClear()
        {
          runSearch();
        }

        @Override
        public void onFilterDone()
        {
          runSearch();
        }
      }, R.string.search_in_table);
    }
  }

  private void runSearch()
  {
    SearchEngine.searchInteractive(mSearchController.getQuery(), System.nanoTime(),
                                   false /* isMapAndTable */,
                                   mFilterController != null ? mFilterController.getFilter() : null,
                                   null /* bookingParams */);
    SearchEngine.showAllResults(mSearchController.getQuery());
  }

  private void initPositionChooser()
  {
    mPositionChooser = findViewById(R.id.position_chooser);
    if (mPositionChooser == null)
      return;

    final Toolbar toolbar = (Toolbar) mPositionChooser.findViewById(R.id.toolbar_position_chooser);
    UiUtils.extendViewWithStatusBar(toolbar);
    UiUtils.showHomeUpButton(toolbar);
    toolbar.setNavigationOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        hidePositionChooser();
      }
    });
    mPositionChooser.findViewById(R.id.done).setOnClickListener(new OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        Statistics.INSTANCE.trackEditorLaunch(true);
        hidePositionChooser();
        if (Framework.nativeIsDownloadedMapAtScreenCenter())
          startActivity(new Intent(MwmActivity.this, FeatureCategoryActivity.class));
        else
          UiUtils.showAlertDialog(MwmActivity.this, R.string.message_invalid_feature_position);
      }
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
    mFadeView = (FadeView) findViewById(R.id.fade_view);
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
      mRootView = (ViewGroup) container.getParent();
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
    ImageButton traffic = (ImageButton) frame.findViewById(R.id.traffic);
    mTraffic = new TrafficButton(this, traffic);
    mTrafficButtonController = new TrafficButtonController(mTraffic, this);
    mNavAnimationController = new NavigationButtonsAnimationController(
        zoomIn, zoomOut, myPosition, getWindow().getDecorView().getRootView(), this);
  }

  public boolean closePlacePage()
  {
    if (mPlacePage == null || mPlacePage.isHidden())
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
    if (!mIsFragmentContainer)
      return;

    closePlacePage();
    if (removeCurrentFragment(true))
    {
      InputUtils.hideKeyboard(mFadeView);
      mFadeView.fadeOut();
    }
  }

  public void closeMenu(String statEvent, String alohaStatEvent, @Nullable Runnable procAfterClose)
  {
    Statistics.INSTANCE.trackEvent(statEvent);
    AlohaHelper.logClick(alohaStatEvent);

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

  public void startLocationToPoint(String statisticsEvent, String alohaEvent,
                                   final @Nullable MapObject endPoint,
                                   final boolean canUseMyPositionAsStart)
  {
    closeMenu(statisticsEvent, alohaEvent, new Runnable()
    {
      @Override
      public void run()
      {
        RoutingController.get().prepare(canUseMyPositionAsStart, endPoint);

        if (mPlacePage != null && (mPlacePage.isDocked() || !mPlacePage.isFloating()))
          closePlacePage();
      }
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
    mMainMenu = new MainMenu(findViewById(R.id.menu_frame), new BaseMenu.ItemClickListener<MainMenu.Item>()
    {
      @Override
      public void onItemClick(MainMenu.Item item)
      {
        if (mIsFullscreenAnimating)
          return;

        switch (item)
        {
        case TOGGLE:
          if (!mMainMenu.isOpen())
          {
            if (mPlacePage == null || (mPlacePage.isDocked() && closePlacePage()))
              return;

            if (closeSidePanel())
              return;
          }

          Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MENU);
          AlohaHelper.logClick(AlohaHelper.TOOLBAR_MENU);
          toggleMenu();
          break;

        case ADD_PLACE:
          closePlacePage();
          if (mIsFragmentContainer)
            closeSidePanel();
          closeMenu(Statistics.EventName.MENU_ADD_PLACE, AlohaHelper.MENU_ADD_PLACE, new Runnable()
          {
            @Override
            public void run()
            {
              Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_ADD_CLICK,
                                             Statistics.params().add(Statistics.EventParam.FROM, "main_menu"));
              showPositionChooser(false, false);
            }
          });
          break;

        case SEARCH:
          RoutingController.get().cancel();
          closeMenu(Statistics.EventName.TOOLBAR_SEARCH, AlohaHelper.TOOLBAR_SEARCH, new Runnable()
          {
            @Override
            public void run()
            {
              showSearch(mSearchController.getQuery());
            }
          });
          break;

        case P2P:
          startLocationToPoint(Statistics.EventName.MENU_P2P, AlohaHelper.MENU_POINT2POINT,
                               null /* endPoint */, false /* canUseMyPositionAsStart */);
          break;

        case BOOKMARKS:
          closeMenu(Statistics.EventName.TOOLBAR_BOOKMARKS, AlohaHelper.TOOLBAR_BOOKMARKS, new Runnable()
          {
            @Override
            public void run()
            {
              showBookmarks();
            }
          });
          break;

        case SHARE:
          closeMenu(Statistics.EventName.MENU_SHARE, AlohaHelper.MENU_SHARE, new Runnable()
          {
            @Override
            public void run()
            {
              shareMyLocation();
            }
          });
          break;

        case DOWNLOADER:
          RoutingController.get().cancel();
          closeMenu(Statistics.EventName.MENU_DOWNLOADER, AlohaHelper.MENU_DOWNLOADER, new Runnable()
          {
            @Override
            public void run()
            {
              showDownloader(false);
            }
          });
          break;

        case SETTINGS:
          closeMenu(Statistics.EventName.MENU_SETTINGS, AlohaHelper.MENU_SETTINGS, new Runnable()
          {
            @Override
            public void run()
            {
              startActivity(new Intent(MwmActivity.this, SettingsActivity.class));
            }
          });
          break;
        }
      }
    });

    if (mIsFragmentContainer)
    {
      mPanelAnimator = new PanelAnimator(this);
      mPanelAnimator.registerListener(mMainMenu.getLeftAnimationTrackListener());
      return;
    }

    if (mPlacePage != null && mPlacePage.isDocked())
      mPlacePage.setLeftAnimationTrackListener(mMainMenu.getLeftAnimationTrackListener());
  }

  private void initOnmapDownloader()
  {
    mOnmapDownloader = new OnmapDownloader(this);
    if (mIsFragmentContainer)
      mPanelAnimator.registerListener(mOnmapDownloader);
  }

  @Override
  public void onDestroy()
  {
    if (!isInitializationCompleted())
    {
      super.onDestroy();
      return;
    }

    BottomSheetHelper.free();
    SearchEngine.INSTANCE.removeListener(this);

    super.onDestroy();
  }

  @Override
  protected void onSaveInstanceState(Bundle outState)
  {
    if (mPlacePage != null && !mPlacePage.isHidden())
    {
      outState.putInt(STATE_PP, mPlacePage.getState().ordinal());
      outState.putParcelable(STATE_MAP_OBJECT, mPlacePage.getMapObject());
      if (isChangingConfigurations())
        mPlacePage.setState(State.HIDDEN);
    }

    if (!mIsFragmentContainer && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.onSaveState(outState);

    if (mIsFragmentContainer)
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
    if (mPlacePage != null && state != State.HIDDEN)
    {
      mPlacePageRestored = true;
      MapObject mapObject = (MapObject) savedInstanceState.getParcelable(STATE_MAP_OBJECT);
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

    if (mIsFragmentContainer)
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

    if (!mIsFragmentContainer && RoutingController.get().isPlanning())
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
  public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                         @NonNull int[] grantResults)
  {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    if (requestCode != LOCATION_REQUEST || grantResults.length == 0)
      return;

    PermissionsResult result = PermissionsUtils.computePermissionsResult(permissions, grantResults);
    if (result.isLocationGranted())
      myPositionClick();
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

    if (intent.hasExtra(EXTRA_TASK))
    {
      addTask(intent);
      return true;
    }

    if (intent.hasExtra(EXTRA_UPDATE_COUNTRIES))
    {
      showDownloader(true);
      return true;
    }

    HotelsFilter filter = intent.getParcelableExtra(SearchActivity.EXTRA_HOTELS_FILTER);
    if (mFilterController != null)
    {
      mFilterController.show(filter != null || !TextUtils.isEmpty(SearchEngine.getQuery()), true);
      mFilterController.setFilter(filter);
      return filter != null;
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
    mPlacePageRestored = mPlacePage != null && mPlacePage.getState() != State.HIDDEN;
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
    if (mPlacePage != null)
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
    if (mPlacePage != null)
      mPlacePage.onActivityPause();
    super.onPause();
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    Framework.nativeSetMapObjectListener(this);
    BookmarkManager.INSTANCE.addListener(this);
    RoutingController.get().attach(this);
    if (MapFragment.nativeIsEngineCreated())
      LocationHelper.INSTANCE.attach(this);
    if (mTrafficButtonController != null)
      TrafficManager.INSTANCE.attach(mTrafficButtonController);
    if (mNavigationController != null)
      TrafficManager.INSTANCE.attach(mNavigationController);
    mPlacePage.onActivityStarted();
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    Framework.nativeRemoveMapObjectListener();
    BookmarkManager.INSTANCE.removeListener(this);
    LocationHelper.INSTANCE.detach(!isFinishing());
    RoutingController.get().detach();
    TrafficManager.INSTANCE.detachAll();
    if (mTrafficButtonController != null)
      mTrafficButtonController.destroy();
    mPlacePage.onActivityStopped();
  }

  @Override
  public void onBackPressed()
  {
    if (mFilterController != null && mFilterController.onBackPressed())
      return;

    if (getCurrentMenu().close(true))
    {
      mFadeView.fadeOut();
      return;
    }

    if (mSearchController.hide())
    {
      SearchEngine.cancelInteractiveSearch();
      return;
    }

    if (!closePlacePage() && !closeSidePanel() && !RoutingController.get().cancel()
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

    if (mPlacePage != null)
    {
      mPlacePage.setMapObject(object, true, new PlacePageView.SetMapObjectListener()
      {
        @Override
        public void onSetMapObjectComplete()
        {
          if (!mPlacePageRestored)
          {
            if (object != null)
            {
              mPlacePage.setState(object.isExtendedView() ? State.DETAILS : State.PREVIEW);
              Framework.logLocalAdsEvent(Framework.LOCAL_ADS_EVENT_OPEN_INFO, object);
            }
          }
          mPlacePageRestored = false;
        }
      });
      if (mPlacePageTracker != null)
        mPlacePageTracker.setMapObject(object);
    }

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
      if (mPlacePage != null)
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
          adjustRuler(0, menuHeight);

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
      mTraffic.hide();
    }
    else
    {
      if (mPlacePage != null && mPlacePage.isHidden() && mNavAnimationController != null)
        mNavAnimationController.appearZoomButtons();
      if (!mIsFullscreenAnimating)
        appearMenu(menu);
      else
        mIsAppearMenuLater = true;
    }
  }

  private void appearMenu(BaseMenu menu)
  {
    Animations.appearSliding(menu.getFrame(), Animations.BOTTOM, new Runnable()
    {
      @Override
      public void run()
      {
        adjustRuler(0, 0);
      }
    });
    if (mNavMyPosition != null)
      mNavMyPosition.show();
    mTraffic.show();
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
      if (mPlacePage != null)
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
    return (mPlacePage != null && mPlacePage.hideOnTouch()) ||
           (mMapFragment != null && mMapFragment.onTouch(view, event));
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
    boolean run(MwmActivity target);
  }

  public static class OpenUrlTask implements MapTask
  {
    private static final long serialVersionUID = 1L;
    private final String mUrl;

    OpenUrlTask(String url)
    {
      Utils.checkNotNull(url);
      mUrl = url;
    }

    @Override
    public boolean run(MwmActivity target)
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
        SearchActivity.start(target, request.mQuery, request.mLocale, request.mIsSearchOnMap, null);
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
    private final boolean mDoAutoDownload;

    public ShowCountryTask(String countryId, boolean doAutoDownload)
    {
      mCountryId = countryId;
      mDoAutoDownload = doAutoDownload;
    }

    @Override
    public boolean run(MwmActivity target)
    {
      if (mDoAutoDownload)
        MapManager.warn3gAndDownload(target, mCountryId, null);

      Framework.nativeShowCountry(mCountryId, mDoAutoDownload);
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

  private void adjustRuler(int offsetX, int offsetY)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    mMapFragment.setupRuler(offsetX, offsetY, true);
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

        if (mIsFragmentContainer)
        {
          mMainMenu.setEnabled(MainMenu.Item.P2P, !RoutingController.get().isPlanning());
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
          adjustRuler(0, menuHeight);
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
    if (!mIsFragmentContainer)
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
    if (!mIsFragmentContainer)
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
    if (!mIsFragmentContainer)
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
        adjustRuler(0, 0);
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

      if (mIsFragmentContainer)
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
      if (mIsFragmentContainer)
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

    if (mPlacePage != null)
      mPlacePage.refreshViews();
  }

  private void adjustCompassAndTraffic(final int offsetY)
  {
    addTask(new MapTask()
    {
      @Override
      public boolean run(MwmActivity target)
      {
        adjustCompass(offsetY);
        return true;
      }
    });
    adjustTraffic(0, offsetY);
  }

  private void adjustTraffic(int offsetX, int offsetY)
  {
    mTraffic.setOffset(offsetX, offsetY);
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
      boolean show = visible && !TextUtils.isEmpty(SearchEngine.getQuery())
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
    if (mPlacePage != null)
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
    if (mIsFragmentContainer)
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
    if (mIsFragmentContainer)
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
    if (mIsFragmentContainer)
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

  private void updateSearchBar()
  {
    if (!TextUtils.isEmpty(SearchEngine.getQuery()))
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
    if (mPlacePage != null && !mPlacePage.isHidden())
      mPlacePage.refreshLocation(location);

    if (!RoutingController.get().isNavigating())
      return;

    if (mNavigationController != null)
      mNavigationController.update(Framework.nativeGetRouteFollowingInfo());

    TtsPlayer.INSTANCE.playTurnNotifications();
  }

  @Override
  public void onCompassUpdated(@NonNull CompassData compass)
  {
    MapFragment.nativeCompassUpdated(compass.getMagneticNorth(), compass.getTrueNorth(), false);
    if (mPlacePage != null)
      mPlacePage.refreshAzimuth(compass.getNorth());
    if (mNavigationController != null)
      mNavigationController.updateNorth(compass.getNorth());
  }

  @Override
  public void onLocationError()
  {
    if (mLocationErrorDialogAnnoying)
      return;

    Intent intent = new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS);
    if (intent.resolveActivity(MwmApplication.get().getPackageManager()) == null)
    {
      intent = new Intent(android.provider.Settings.ACTION_SECURITY_SETTINGS);
      if (intent.resolveActivity(MwmApplication.get().getPackageManager()) == null)
        return;
    }

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
        mTraffic.hide();
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

  private void showLocationNotFoundDialog()
  {
    String message = String.format("%s\n\n%s", getString(R.string.current_location_unknown_message),
                                   getString(R.string.current_location_unknown_title));
    new AlertDialog.Builder(this)
        .setMessage(message)
        .setNegativeButton(R.string.current_location_unknown_stop_button, null)
        .setPositiveButton(R.string.current_location_unknown_continue_button, new DialogInterface.OnClickListener()
        {
          @Override
          public void onClick(DialogInterface dialog, int which)
          {
            if (!LocationHelper.INSTANCE.isActive())
              LocationHelper.INSTANCE.start();
          }
        }).show();
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

  public static class ShowAuthorizationTask implements MapTask
  {
    @Override
    public boolean run(MwmActivity target)
    {
      final DialogFragment fragment = (DialogFragment) Fragment.instantiate(target, AuthDialogFragment.class.getName());
      fragment.show(target.getSupportFragmentManager(), AuthDialogFragment.class.getName());
      return true;
    }
  }

  public static abstract class BaseUserMarkTask implements MapTask
  {
    private static final long serialVersionUID = 1L;

    final int mCategoryId;
    final int mId;

    BaseUserMarkTask(int categoryId, int id)
    {
      mCategoryId = categoryId;
      mId = id;
    }
  }

  public static class ShowBookmarkTask extends BaseUserMarkTask
  {
    public ShowBookmarkTask(int categoryId, int bookmarkId)
    {
      super(categoryId, bookmarkId);
    }

    @Override
    public boolean run(MwmActivity target)
    {
      BookmarkManager.INSTANCE.nativeShowBookmarkOnMap(mCategoryId, mId);
      return true;
    }
  }

  public static class ShowTrackTask extends BaseUserMarkTask
  {
    public ShowTrackTask(int categoryId, int trackId)
    {
      super(categoryId, trackId);
    }

    @Override
    public boolean run(MwmActivity target)
    {
      Framework.nativeShowTrackRect(mCategoryId, mId);
      return true;
    }
  }

  static class ShowPointTask implements MapTask
  {
    private final double mLat;
    private final double mLon;

    ShowPointTask(double lat, double lon)
    {
      mLat = lat;
      mLon = lon;
    }

    @Override
    public boolean run(MwmActivity target)
    {
      MapFragment.nativeShowMapForUrl(String.format(Locale.US,
                                                    "mapsme://map?ll=%f,%f", mLat, mLon));
      return true;
    }
  }

  static class BuildRouteTask implements MapTask
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

    BuildRouteTask(double latTo, double lonTo)
    {
      this(latTo, lonTo, null, null, null, null, null);
    }

    BuildRouteTask(double latTo, double lonTo, @Nullable String saddr,
                   @Nullable Double latFrom, @Nullable Double lonFrom, @Nullable String daddr)
    {
      this(latTo, lonTo, saddr, latFrom, lonFrom, daddr, null);
    }

    BuildRouteTask(double latTo, double lonTo, @Nullable String saddr,
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
    public boolean run(MwmActivity target)
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
    public boolean run(MwmActivity target)
    {
      RoutingController.get().restoreRoute();
      return true;
    }
  }
}
