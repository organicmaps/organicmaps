package app.organicmaps;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import androidx.activity.result.ActivityResult;
import androidx.activity.result.ActivityResultLauncher;
import androidx.activity.result.IntentSenderRequest;
import androidx.activity.result.contract.ActivityResultContracts;
import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.annotation.UiThread;
import androidx.appcompat.widget.Toolbar;
import androidx.core.app.ActivityCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.FragmentManager;
import androidx.fragment.app.FragmentTransaction;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.Framework.PlacePageActivationListener;
import app.organicmaps.api.Const;
import app.organicmaps.downloader.DownloaderNotifier;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.base.CustomNavigateUpListener;
import app.organicmaps.base.NoConnectionListener;
import app.organicmaps.base.OnBackPressListener;
import app.organicmaps.bookmarks.BookmarkCategoriesActivity;
import app.organicmaps.bookmarks.data.BookmarkInfo;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Track;
import app.organicmaps.downloader.DownloaderActivity;
import app.organicmaps.downloader.DownloaderFragment;
import app.organicmaps.downloader.MapManager;
import app.organicmaps.downloader.OnmapDownloader;
import app.organicmaps.downloader.UpdateInfo;
import app.organicmaps.editor.Editor;
import app.organicmaps.editor.EditorActivity;
import app.organicmaps.editor.EditorHostFragment;
import app.organicmaps.editor.FeatureCategoryActivity;
import app.organicmaps.editor.ReportFragment;
import app.organicmaps.help.HelpActivity;
import app.organicmaps.intent.Factory;
import app.organicmaps.intent.MapTask;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.location.LocationState;
import app.organicmaps.location.SensorHelper;
import app.organicmaps.location.SensorListener;
import app.organicmaps.maplayer.MapButtonsController;
import app.organicmaps.maplayer.MapButtonsViewModel;
import app.organicmaps.maplayer.Mode;
import app.organicmaps.maplayer.ToggleMapLayerFragment;
import app.organicmaps.maplayer.isolines.IsolinesManager;
import app.organicmaps.maplayer.isolines.IsolinesState;
import app.organicmaps.maplayer.subway.SubwayManager;
import app.organicmaps.routing.NavigationController;
import app.organicmaps.routing.NavigationService;
import app.organicmaps.routing.RoutePointInfo;
import app.organicmaps.routing.RoutingBottomMenuListener;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.routing.RoutingErrorDialogFragment;
import app.organicmaps.routing.RoutingOptions;
import app.organicmaps.routing.RoutingPlanFragment;
import app.organicmaps.routing.RoutingPlanInplaceController;
import app.organicmaps.search.FloatingSearchToolbarController;
import app.organicmaps.search.SearchActivity;
import app.organicmaps.search.SearchEngine;
import app.organicmaps.search.SearchFragment;
import app.organicmaps.settings.DrivingOptionsActivity;
import app.organicmaps.settings.RoadType;
import app.organicmaps.settings.SettingsActivity;
import app.organicmaps.settings.UnitLocale;
import app.organicmaps.util.Config;
import app.organicmaps.util.Counters;
import app.organicmaps.util.LocationUtils;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.ThemeSwitcher;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.menu.MainMenu;
import app.organicmaps.widget.placepage.PlacePageController;
import app.organicmaps.widget.placepage.PlacePageData;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;

import java.util.ArrayList;
import java.util.Objects;
import java.util.Stack;

import static android.Manifest.permission.ACCESS_COARSE_LOCATION;
import static android.Manifest.permission.ACCESS_FINE_LOCATION;
import static android.Manifest.permission.POST_NOTIFICATIONS;
import static android.content.pm.PackageManager.PERMISSION_GRANTED;
import static app.organicmaps.location.LocationState.LOCATION_TAG;

public class MwmActivity extends BaseMwmFragmentActivity
    implements PlacePageActivationListener,
               View.OnTouchListener,
               MapRenderingListener,
               CustomNavigateUpListener,
               RoutingController.Container,
               LocationListener,
    SensorListener,
               LocationState.ModeChangeListener,
               RoutingPlanInplaceController.RoutingPlanListener,
               RoutingBottomMenuListener,
               BookmarkManager.BookmarksLoadingListener,
               FloatingSearchToolbarController.SearchToolbarListener,
               NoConnectionListener,
               MenuBottomSheetFragment.MenuBottomSheetInterfaceWithHeader,
               PlacePageController.PlacePageRouteSettingsListener,
               MapButtonsController.MapButtonClickListener
{
  private static final String TAG = MwmActivity.class.getSimpleName();

  public static final String EXTRA_TASK = "map_task";
  public static final String EXTRA_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
  public static final String EXTRA_BACK_URL = "backurl";
  private static final String EXTRA_CONSUMED = "mwm.extra.intent.processed";

  private static final String[] DOCKED_FRAGMENTS = { SearchFragment.class.getName(),
                                                     DownloaderFragment.class.getName(),
                                                     RoutingPlanFragment.class.getName(),
                                                     EditorHostFragment.class.getName(),
                                                     ReportFragment.class.getName() };

  public static final int REQ_CODE_DRIVING_OPTIONS = 6;

  private static final String MAIN_MENU_ID = "MAIN_MENU_BOTTOM_SHEET";
  private static final String LAYERS_MENU_ID = "LAYERS_MENU_BOTTOM_SHEET";

  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();

  @Nullable
  private MapFragment mMapFragment;

  private View mPointChooser;
  private Toolbar mPointChooserToolbar;

  enum PointChooserMode
  {
    NONE,
    EDITOR,
    API
  }
  @NonNull
  private PointChooserMode mPointChooserMode = PointChooserMode.NONE;

  private RoutingPlanInplaceController mRoutingPlanInplaceController;

  private NavigationController mNavigationController;

  private MainMenu mMainMenu;

  private PanelAnimator mPanelAnimator;
  @Nullable
  private OnmapDownloader mOnmapDownloader;
  private boolean mIsTabletLayout;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private FloatingSearchToolbarController mSearchController;

  private boolean mRestoreRoutingPlanFragmentNeeded;
  @Nullable
  private Bundle mSavedForTabletState;
  private String mDonatesUrl;

  private int mNavBarHeight;

  private PlacePageViewModel mPlacePageViewModel;
  private MapButtonsViewModel mMapButtonsViewModel;
  private MapButtonsController.LayoutMode mPreviousMapLayoutMode;
  private Mode mPreviousLayerMode;

  @Nullable
  private WindowInsetsCompat mCurrentWindowInsets;

  @Nullable
  private Dialog mLocationErrorDialog;

  @Nullable
  private Dialog mAlertDialog;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ActivityResultLauncher<String[]> mLocationPermissionRequest;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ActivityResultLauncher<String> mPostNotificationPermissionRequest;

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private ActivityResultLauncher<IntentSenderRequest> mLocationResolutionRequest;

  public interface LeftAnimationTrackListener
  {
    void onTrackStarted(boolean collapsed);

    void onTrackFinished(boolean collapsed);

    void onTrackLeftAnimation(float offset);
  }

  public static Intent createShowMapIntent(@NonNull Context context, @Nullable String countryId)
  {
    return new Intent(context, DownloadResourcesLegacyActivity.class)
        .putExtra(DownloadResourcesLegacyActivity.EXTRA_COUNTRY, countryId);
  }

  @Override
  public void onRenderingCreated()
  {
    checkMeasurementSystem();
  }

  @Override
  public void onRenderingRestored()
  {
    runTasks();
  }

  @Override
  public void onRenderingInitializationFinished()
  {
    runTasks();
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

  private void showBookmarks()
  {
    BookmarkCategoriesActivity.start(this);
  }

  public void showHelp()
  {
    Intent intent = new Intent(this, HelpActivity.class);
    startActivity(intent);
  }

  public void showSearch(String query)
  {
    closeSearchToolbar(false, true);
    if (mIsTabletLayout)
    {
      final Bundle args = new Bundle();
      args.putString(SearchActivity.EXTRA_QUERY, query);
      replaceFragment(SearchFragment.class, args, null);
    }
    else
    {
      SearchActivity.start(this, query);
    }
  }

  public void showEditor()
  {
    // TODO(yunikkk) think about refactoring. It probably should be called in editor.
    Editor.nativeStartEdit();
    if (mIsTabletLayout)
      replaceFragment(EditorHostFragment.class, null, null);
    else
      EditorActivity.start(this);
  }

  private void shareMyLocation()
  {
    final Location loc = LocationHelper.from(this).getSavedLocation();
    if (loc != null)
    {
      SharingUtils.shareLocation(this, loc);
      return;
    }

    dismissLocationErrorDialog();
    mLocationErrorDialog = new MaterialAlertDialogBuilder(MwmActivity.this, R.style.MwmTheme_AlertDialog)
        .setMessage(R.string.unknown_current_position)
        .setCancelable(true)
        .setPositiveButton(R.string.ok, null)
        .setOnDismissListener(dialog -> mLocationErrorDialog = null)
        .show();
  }

  @Override
  public void showDownloader(boolean openDownloaded)
  {
    final Bundle args = new Bundle();
    args.putBoolean(DownloaderActivity.EXTRA_OPEN_DOWNLOADED, openDownloaded);
    if (mIsTabletLayout)
    {
      closeSearchToolbar(false, true);
      replaceFragment(DownloaderFragment.class, args, null);
    }
    else
    {
      startActivity(new Intent(this, DownloaderActivity.class).putExtras(args));
    }
  }

  @Override
  @StyleRes
  protected int getThemeResourceId(@NonNull String theme)
  {
    Context context = getApplicationContext();

    if (ThemeUtils.isDefaultTheme(context, theme))
      return R.style.MwmTheme_MainActivity;

    if (ThemeUtils.isNightTheme(context, theme))
      return R.style.MwmTheme_Night_MainActivity;

    return super.getThemeResourceId(theme);
  }

  @SuppressLint("InlinedApi")
  @CallSuper
  @Override
  protected void onSafeCreate(@Nullable Bundle savedInstanceState)
  {
    super.onSafeCreate(savedInstanceState);

    mIsTabletLayout = getResources().getBoolean(R.bool.tabletLayout);

    if (!mIsTabletLayout)
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

    setContentView(R.layout.activity_map);
    UiUtils.setupTransparentStatusBar(this);

    mPlacePageViewModel = new ViewModelProvider(this).get(PlacePageViewModel.class);
    mMapButtonsViewModel = new ViewModelProvider(this).get(MapButtonsViewModel.class);
    // We don't need to manually handle removing the observers it follows the activity lifecycle
    mMapButtonsViewModel.getBottomButtonsHeight().observe(this, this::onMapBottomButtonsHeightChange);
    mMapButtonsViewModel.getLayoutMode().observe(this, this::initNavigationButtons);
    mPreviousLayerMode = mMapButtonsViewModel.getMapLayerMode().getValue();
    mMapButtonsViewModel.getMapLayerMode().observe(this, this::onLayerChange);

    mSearchController = new FloatingSearchToolbarController(this, this);
    mSearchController.getToolbar()
                     .getViewTreeObserver();

    boolean isLaunchByDeepLink = getIntent().getBooleanExtra(EXTRA_LAUNCH_BY_DEEP_LINK, false);
    initViews(isLaunchByDeepLink);
    updateViewsInsets();

    // Note: You must call registerForActivityResult() before the fragment or activity is created.
    mLocationPermissionRequest = registerForActivityResult(new ActivityResultContracts.RequestMultiplePermissions(),
        this::onLocationPermissionsResult);
    mLocationResolutionRequest = registerForActivityResult(new ActivityResultContracts.StartIntentSenderForResult(),
        this::onLocationResolutionResult);
    mPostNotificationPermissionRequest = registerForActivityResult(new ActivityResultContracts.RequestPermission(),
        this::onPostNotificationPermissionResult);

    boolean isConsumed = savedInstanceState == null && processIntent(getIntent());
    boolean isFirstLaunch = Counters.isFirstLaunch(this);
    // If the map activity is launched by any incoming intent (deeplink, update maps event, etc)
    // or it's the first launch (onboarding) we haven't to try restoring the route,
    // showing the tips, etc.
    if (isConsumed || isFirstLaunch)
      return;

    if (savedInstanceState == null && RoutingController.get().hasSavedRoute())
      addTask(new Factory.RestoreRouteTask());

    autostartLocation();
  }

  private void refreshLightStatusBar()
  {
    UiUtils.setLightStatusBar(this, !(
        ThemeUtils.isNightTheme(this)
        || RoutingController.get().isPlanning()
        || Framework.nativeIsInChoosePositionMode()
    ));
  }

  private void updateViewsInsets()
  {
    ViewCompat.setOnApplyWindowInsetsListener(mPointChooser, (view, windowInsets) -> {
      UiUtils.setViewInsetsPaddingBottom(mPointChooser, windowInsets);
      UiUtils.setViewInsetsPaddingNoBottom(mPointChooserToolbar, windowInsets);

      mNavBarHeight = isFullscreen() ? 0 : windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).bottom;
      // For the first loading, set compass top margin to status bar size
      // The top inset will be then be updated by the routing controller
      if (mCurrentWindowInsets == null)
        updateCompassOffset(windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top, windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).right);
      else
        updateCompassOffset(-1, windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).right);
      refreshLightStatusBar();
      updateBottomWidgetsOffset(windowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).left);
      mCurrentWindowInsets = windowInsets;
      return windowInsets;
    });
  }

  private int getDownloadMapsCounter()
  {
    UpdateInfo info = MapManager.nativeGetUpdateInfo(null);
    return info == null ? 0 : info.filesCount;
  }

  @Override
  public void onNoConnectionError()
  {
    dismissAlertDialog();
    mAlertDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.common_check_internet_connection_dialog_title)
        .setMessage(R.string.common_check_internet_connection_dialog)
        .setPositiveButton(R.string.ok, null)
        .setOnDismissListener(dialog -> mAlertDialog = null)
        .show();
  }

  private void initViews(boolean isLaunchByDeeplink)
  {
    initMap(isLaunchByDeeplink);
    initNavigationButtons();

    if (!mIsTabletLayout)
    {
      mRoutingPlanInplaceController = new RoutingPlanInplaceController(this, this, this);
      removeCurrentFragment(false);
    }

    mNavigationController = new NavigationController(this, v -> onSettingsOptionSelected(), this::updateBottomWidgetsOffset);
    //TrafficManager.INSTANCE.attach(mNavigationController);

    initMainMenu();
    initOnmapDownloader();
    initPositionChooser();
  }

  private void initPositionChooser()
  {
    mPointChooser = findViewById(R.id.position_chooser);
    if (mPointChooser == null)
      return;

    mPointChooserToolbar = mPointChooser.findViewById(R.id.toolbar_point_chooser);
    UiUtils.showHomeUpButton(mPointChooserToolbar);
    mPointChooserToolbar.setNavigationOnClickListener(v -> {
      closePositionChooser();
      if (mPointChooserMode == PointChooserMode.API)
        finish();
    });
    mPointChooser.findViewById(R.id.done).setOnClickListener(
        v ->
        {
          switch (mPointChooserMode)
          {
          case API:
            final Intent apiResult = new Intent();
            final double[] center = Framework.nativeGetScreenRectCenter();
            apiResult.putExtra(Const.EXTRA_POINT_LAT, center[0]);
            apiResult.putExtra(Const.EXTRA_POINT_LON, center[1]);
            apiResult.putExtra(Const.EXTRA_ZOOM_LEVEL, Framework.nativeGetDrawScale());
            setResult(Activity.RESULT_OK, apiResult);
            finish();
            break;
          case EDITOR:
            if (Framework.nativeIsDownloadedMapAtScreenCenter())
              startActivity(new Intent(MwmActivity.this, FeatureCategoryActivity.class));
            else
            {
                dismissAlertDialog();
                mAlertDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
                    .setTitle(R.string.message_invalid_feature_position)
                    .setPositiveButton(R.string.ok, null)
                    .setOnDismissListener(dialog -> mAlertDialog = null)
                    .show();
            }
            break;
          case NONE:
            throw new IllegalStateException("Unexpected mPositionChooserMode");
          }
          closePositionChooser();
        });
    UiUtils.hide(mPointChooser);
  }

  private void refreshSearchToolbar()
  {
    mSearchController.showProgress(false);
    final CharSequence query = SearchEngine.INSTANCE.getQuery();
    if (!TextUtils.isEmpty(query))
    {
      mSearchController.setQuery(query);
      // Close all panels and tool bars (including search) but do not stop search backend
      closeFloatingToolbars(false, false);
      // Do not show the search tool bar if we are planning or navigating
      if (!RoutingController.get().isNavigating() && !RoutingController.get().isPlanning())
      {
        showSearchToolbar();
      }
    }
    else
    {
      closeSearchToolbar(true, true);
    }
  }

  private void showSearchToolbar()
  {
    mSearchController.show();
  }

  public void showPositionChooserForAPI(String appName)
  {
    showPositionChooser(PointChooserMode.API, false, false);
    if (!TextUtils.isEmpty(appName))
    {
      setTitle(appName);
      ((TextView) mPointChooser.findViewById(R.id.title)).setText(appName);
    }
  }

  public void showPositionChooserForEditor(boolean isBusiness, boolean applyPosition)
  {
    showPositionChooser(PointChooserMode.EDITOR, isBusiness, applyPosition);
  }

  private void showPositionChooser(PointChooserMode mode, boolean isBusiness, boolean applyPosition)
  {
    mPointChooserMode = mode;
    closeFloatingToolbarsAndPanels(false);
    UiUtils.show(mPointChooser);
    mMapButtonsViewModel.setButtonsHidden(true);
    Framework.nativeTurnOnChoosePositionMode(isBusiness, applyPosition);
    refreshLightStatusBar();
  }

  private void hidePositionChooser()
  {
    UiUtils.hide(mPointChooser);
    Framework.nativeTurnOffChoosePositionMode();
    mMapButtonsViewModel.setButtonsHidden(false);
    if (mPointChooserMode == PointChooserMode.API)
      finish();
    mPointChooserMode = PointChooserMode.NONE;
    refreshLightStatusBar();
  }

  private void initMap(boolean isLaunchByDeepLink)
  {
    final FragmentManager manager = getSupportFragmentManager();
    mMapFragment = (MapFragment) manager.findFragmentByTag(MapFragment.class.getName());
    if (mMapFragment == null)
    {
      Bundle args = new Bundle();
      args.putBoolean(Map.ARG_LAUNCH_BY_DEEP_LINK, isLaunchByDeepLink);
      final FragmentFactory factory = manager.getFragmentFactory();
      mMapFragment = (MapFragment) factory.instantiate(getClassLoader(), MapFragment.class.getName());
      mMapFragment.setArguments(args);
      manager
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

  private void initNavigationButtons()
  {
    initNavigationButtons(mMapButtonsViewModel.getLayoutMode().getValue());
  }

  private void initNavigationButtons(MapButtonsController.LayoutMode layoutMode)
  {
    // Recreate the navigation buttons with the correct layout when it changes
    if (mPreviousMapLayoutMode != layoutMode)
    {
      FragmentTransaction transaction = getSupportFragmentManager()
          .beginTransaction().replace(R.id.map_buttons, new MapButtonsController());
      transaction.commit();
      mPreviousMapLayoutMode = layoutMode;
    }
  }

  @Override
  public void onSearchCanceled()
  {
    closeSearchToolbar(true, true);
  }

  @Override
  public void onMapButtonClick(MapButtonsController.MapButtons button)
  {
    switch (button)
    {
      case zoomIn:
        Map.zoomIn();
        break;
      case zoomOut:
        Map.zoomOut();
        break;
      case myPosition:
        LocationState.nativeSwitchToNextMode();
        startLocation();
        break;
      case toggleMapLayer:
        toggleMapLayerBottomSheet();
        break;
      case bookmarks:
        showBookmarks();
        break;
      case search:
        showSearch();
        break;
      case menu:
        closeFloatingPanels();
        showBottomSheet(MAIN_MENU_ID);
        break;
      case help:
        showHelp();
        break;
    }
  }


  private boolean closeBottomSheet(String id)
  {
    MenuBottomSheetFragment bottomSheet =
        (MenuBottomSheetFragment) getSupportFragmentManager().findFragmentByTag(id);
    if (bottomSheet == null || !bottomSheet.isAdded())
      return false;
    bottomSheet.dismiss();
    return true;
  }

  private void showBottomSheet(String id)
  {
    MenuBottomSheetFragment.newInstance(id).show(getSupportFragmentManager(), id);
  }

  private void toggleMapLayerBottomSheet()
  {
    if (!closeBottomSheet(LAYERS_MENU_ID))
      showBottomSheet(LAYERS_MENU_ID);
  }

  /**
   * @return False if the place page was already closed, true otherwise
   */
  public boolean closePlacePage()
  {
    if (mPlacePageViewModel.getMapObject().getValue() == null)
      return false;

    mPlacePageViewModel.setMapObject(null);
    return true;
  }

  /**
   * @return False if the navigation menu was already collapsed or hidden, true otherwise
   */
  public boolean collapseNavMenu()
  {
    if (mNavigationController.isNavMenuCollapsed() || mNavigationController.isNavMenuHidden())
      return false;
    mNavigationController.collapseNavMenu();
    return true;
  }

  /**
   * @return False if the side panel was already closed, true otherwise
   */
  public boolean closeSidePanel()
  {
    if (interceptBackPress())
      return true;

    return removeCurrentFragment(true);
  }

  private void closeAllFloatingPanelsTablet()
  {
    if (!mIsTabletLayout)
      return;

    closePlacePage();
    removeCurrentFragment(true);
  }

  /**
   * @return False if the position chooser was already closed, true otherwise
   */
  private boolean closePositionChooser()
  {
    if (UiUtils.isVisible(mPointChooser))
    {
      hidePositionChooser();
      return true;
    }
    return false;
  }

  /**
   * @param clearText True to clear the search query
   * @param stopSearch True to stop the search engine
   * @return False if the search toolbar was already closed and the search query was empty, true otherwise
   */
  private boolean closeSearchToolbar(boolean clearText, boolean stopSearch)
  {
    if (UiUtils.isVisible(mSearchController.getToolbar()) || !TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()))
    {
      if (stopSearch)
      {
        mSearchController.cancelSearchApiAndHide(clearText);
        mMapButtonsViewModel.setSearchOption(null);
      }
      else
      {
        mSearchController.hide();
        if (clearText)
        {
          mSearchController.clear();
        }
      }
      return true;
    }
    return false;
  }

  private void closeFloatingToolbarsAndPanels(boolean clearSearchText)
  {
    closeFloatingPanels();
    closeFloatingToolbars(clearSearchText, true);
  }

  public void closeFloatingPanels()
  {
    closeBottomSheet(LAYERS_MENU_ID);
    closeBottomSheet(MAIN_MENU_ID);
    closePlacePage();
  }

  private void closeFloatingToolbars(boolean clearSearchText, boolean stopSearch)
  {
    closePositionChooser();
    closeSearchToolbar(clearSearchText, stopSearch);
  }

  public void startLocationToPoint(final @Nullable MapObject endPoint)
  {
    closeFloatingPanels();
    startLocation();

    MapObject startPoint = LocationHelper.from(this).getMyPosition();
    RoutingController.get().prepare(startPoint, endPoint);

    // TODO: check for tablet.
    closePlacePage();
  }

  private void initMainMenu()
  {
    final View menuFrame = findViewById(R.id.menu_frame);
    mMainMenu = new MainMenu(menuFrame, () -> {
      this.updateBottomWidgetsOffset();
      mPlacePageViewModel.setPlacePageDistanceToTop(menuFrame.getTop());
    });

    if (mIsTabletLayout)
    {
      mPanelAnimator = new PanelAnimator(this);
    }
  }

  private void initOnmapDownloader()
  {
    mOnmapDownloader = new OnmapDownloader(this);
    if (mIsTabletLayout)
      mPanelAnimator.registerListener(mOnmapDownloader);
  }

  @Override
  protected void onSaveInstanceState(@NonNull Bundle outState)
  {
    if (!mIsTabletLayout && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.onSaveState(outState);

    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.saveRoutingPanelState(outState);
    }

    RoutingController.get().onSaveState();

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
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);

    if (resultCode != Activity.RESULT_OK)
      return;

    if (requestCode == REQ_CODE_DRIVING_OPTIONS)
      rebuildLastRoute();
  }

  private void rebuildLastRoute()
  {
    RoutingController.get().attach(this);
    rebuildLastRouteInternal();
  }

  private void rebuildLastRouteInternal()
  {
    if (mRoutingPlanInplaceController == null)
      return;

    mRoutingPlanInplaceController.hideDrivingOptionsView();
    RoutingController.get().rebuildLastRoute();
  }

  private void onIsolinesStateChanged(@NonNull IsolinesState type)
  {
    if (type != IsolinesState.EXPIREDDATA)
    {
      type.activate(this, findViewById(R.id.coordinator), findViewById(R.id.menu_frame));
      return;
    }

    dismissAlertDialog();
    mAlertDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.downloader_update_maps)
        .setMessage(R.string.isolines_activation_error_dialog)
        .setPositiveButton(R.string.ok, (dialog, which) -> startActivity(new Intent(this, DownloaderActivity.class)))
        .setNegativeButton(R.string.cancel, null)
        .setOnDismissListener(dialog -> mAlertDialog = null)
        .show();
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

    DownloaderNotifier.processNotificationExtras(getApplicationContext(), intent);

    if (intent.hasExtra(EXTRA_TASK))
    {
      addTask(intent);
      return true;
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
    return mMapFragment != null && Map.isEngineCreated()
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
    refreshSearchToolbar();
    setFullscreen(isFullscreen());
    if (Framework.nativeIsInChoosePositionMode())
    {
      UiUtils.show(mPointChooser);
      mMapButtonsViewModel.setButtonsHidden(true);
    }
    if (mOnmapDownloader != null)
      mOnmapDownloader.onResume();

    mNavigationController.refresh();
    refreshLightStatusBar();

    LocationState.nativeSetLocationPendingTimeoutListener(this::onLocationPendingTimeout);
    SensorHelper.from(this).addListener(this);
  }

  @Override
  public void recreate()
  {
    // Explicitly destroy surface before activity recreation.
    if (mMapFragment != null)
      mMapFragment.destroySurface();
    super.recreate();
  }

  @Override
  protected void onResumeFragments()
  {
    super.onResumeFragments();
    RoutingController.get().restore();
  }

  @Override
  protected void onPause()
  {
    if (mOnmapDownloader != null)
      mOnmapDownloader.onPause();
    LocationState.nativeRemoveLocationPendingTimeoutListener();
    SensorHelper.from(this).removeListener(this);
    dismissLocationErrorDialog();
    dismissAlertDialog();
    super.onPause();
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    Framework.nativePlacePageActivationListener(this);
    BookmarkManager.INSTANCE.addLoadingListener(this);
    RoutingController.get().attach(this);
    IsolinesManager.from(getApplicationContext()).attach(this::onIsolinesStateChanged);
    LocationState.nativeSetListener(this);
    LocationHelper.from(this).addListener(this);
    onMyPositionModeChanged(LocationState.nativeGetMode());
    mSearchController.attach(this);
    if (!Config.isScreenSleepEnabled())
      Utils.keepScreenOn(true, getWindow());
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    Framework.nativeRemovePlacePageActivationListener();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    LocationHelper.from(this).removeListener(this);
    LocationState.nativeRemoveListener();
    RoutingController.get().detach();
    IsolinesManager.from(getApplicationContext()).detach();
    mSearchController.detach();
    Utils.keepScreenOn(false, getWindow());
  }

  @CallSuper
  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    mLocationPermissionRequest.unregister();
    mLocationPermissionRequest = null;
    mLocationResolutionRequest.unregister();
    mLocationResolutionRequest = null;
    mPostNotificationPermissionRequest.unregister();
    mPostNotificationPermissionRequest = null;
  }

  @Override
  public void onBackPressed()
  {
    RoutingController routingController = RoutingController.get();
    if (!closeBottomSheet(MAIN_MENU_ID) && !closeBottomSheet(LAYERS_MENU_ID) && !collapseNavMenu() &&
        !closePlacePage() &&!closeSearchToolbar(true, true) &&
        !closeSidePanel() && !closePositionChooser() &&
        !routingController.resetToPlanningStateIfNavigating() && !routingController.cancel())
    {
      try
      {
        super.onBackPressed();
      }
      catch (IllegalStateException e)
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
      mPanelAnimator.hide(() -> removeFragmentImmediate(fragment));
    else
      removeFragmentImmediate(fragment);

    return true;
  }

  // Called from JNI.
  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    setFullscreen(false);
    // This will open the place page
    mPlacePageViewModel.setMapObject((MapObject) data);
  }

  // Called from JNI.
  @Override
  public void onPlacePageDeactivated(boolean switchFullScreenMode)
  {
    if (switchFullScreenMode)
    {
      if ((mPanelAnimator != null && mPanelAnimator.isVisible()) ||
          UiUtils.isVisible(mSearchController.getToolbar()))
        return;

      setFullscreen(!isFullscreen());
    }
    else
    {
      closePlacePage();
    }
  }

  private void setFullscreen(boolean isFullscreen)
  {
    if (RoutingController.get().isNavigating()
        || RoutingController.get().isBuilding()
        || RoutingController.get().isPlanning())
      return;

    mMapButtonsViewModel.setButtonsHidden(isFullscreen);
    UiUtils.setFullscreen(this, isFullscreen);
  }

  private boolean isFullscreen()
  {
    // Buttons are hidden in position chooser mode but we are not in fullscreen
    return Boolean.TRUE.equals(mMapButtonsViewModel.getButtonsHidden().getValue()) && !Framework.nativeIsInChoosePositionMode();
  }

  @Override
  public boolean onTouch(View view, MotionEvent event)
  {
    return mMapFragment != null && mMapFragment.onTouch(view, event);
  }

  @Override
  public void customOnNavigateUp()
  {
    if (removeCurrentFragment(true))
    {
      refreshSearchToolbar();
    }
  }

  void updateCompassOffset(int offsetY)
  {
    updateCompassOffset(offsetY, -1);
  }

  void updateCompassOffset(int offsetY, int offsetX)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    mMapFragment.updateCompassOffset(offsetX, offsetY);

    final double north = SensorHelper.from(this).getSavedNorth();
    if (!Double.isNaN(north))
      Map.onCompassUpdated(north, true);
  }

  public void onMapBottomButtonsHeightChange(float height) {
    updateBottomWidgetsOffset();
  }

  public void updateBottomWidgetsOffset()
  {
    updateBottomWidgetsOffset(-1);
  }

  public void updateBottomWidgetsOffset(int offsetX)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    int offsetY = mNavBarHeight;
    final Float bottomButtonHeight = mMapButtonsViewModel.getBottomButtonsHeight().getValue();
    if (bottomButtonHeight != null)
      offsetY = Math.max(offsetY, bottomButtonHeight.intValue() + mNavBarHeight);
    if (mMainMenu != null)
      offsetY = Math.max(offsetY, mMainMenu.getMenuHeight());

    final View navBottomSheetLineFrame = findViewById(R.id.line_frame);
    final View navBottomSheetNavBar = findViewById(R.id.nav_bottom_sheet_nav_bar);
    if (navBottomSheetLineFrame != null)
      offsetY = Math.max(offsetY, navBottomSheetLineFrame.getHeight() + navBottomSheetNavBar.getHeight());

    mMapFragment.updateBottomWidgetsOffset(offsetX, offsetY);
    mMapFragment.updateMyPositionRoutingOffset(offsetY);
  }

  @Override
  public void showSearch()
  {
    showSearch("");
  }

  @Override
  public void updateMenu()
  {
    boolean isVisible = adjustMenuLineFrameVisibility();
    if (!isVisible)
      return;

    if (RoutingController.get().isNavigating())
    {
      mNavigationController.show(true);
      closeSearchToolbar(false, false);
      mMainMenu.setState(MainMenu.State.NAVIGATION, isFullscreen());
      return;
    }

    if (RoutingController.get().isPlanning())
    {
      mMainMenu.setState(MainMenu.State.ROUTE_PREPARE, isFullscreen());
      return;
    }

    mMainMenu.setState(MainMenu.State.MENU, isFullscreen());
  }

  private boolean adjustMenuLineFrameVisibility()
  {
    final RoutingController controller = RoutingController.get();

    if (controller.isBuilt())
    {
      showMainMenu(true);
      return true;
    }

    if (controller.isPlanning() || controller.isBuilding() || controller.isErrorEncountered())
    {
      if (showAddStartOrFinishFrame(controller, true))
      {
        return true;
      }

      showMainMenu(false);
      return false;
    }

    hideRoutingActionFrame();
    showMainMenu(true);
    return true;
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

    MapObject myPosition = LocationHelper.from(this).getMyPosition();

    if (myPosition != null && !controller.hasEndPoint())
    {
      showAddFinishFrame();
      if (showFrame)
        showMainMenu(true);
      return true;
    }
    if (!controller.hasStartPoint())
    {
      showAddStartFrame();
      if (showFrame)
        showMainMenu(true);
      return true;
    }
    if (!controller.hasEndPoint())
    {
      showAddFinishFrame();
      if (showFrame)
        showMainMenu(true);
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

  private void showMainMenu(boolean show)
  {
    mMainMenu.show(show);
  }

  @Override
  public void onRoutingPlanStartAnimate(boolean show)
  {
    // TODO This code section may be called when insets are not yet initialized
    // This is only a workaround to prevent crashes but a proper fix should be implemented
    if (mCurrentWindowInsets == null) {
      return;
    }
    int offset = mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top;
    if (show && mRoutingPlanInplaceController != null)
    {
      final int height = mRoutingPlanInplaceController.calcHeight();
      if (height != 0)
        offset = height;
    }
    adjustCompassAndTraffic(offset);
  }

  @Override
  public void showRoutePlan(boolean show, @Nullable Runnable completionListener)
  {
    if (show)
    {
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
      if (mIsTabletLayout && mCurrentWindowInsets != null)
        adjustCompassAndTraffic(mCurrentWindowInsets.getInsets(WindowInsetsCompat.Type.systemBars()).top);
      else if (!mIsTabletLayout)
        mRoutingPlanInplaceController.show(false);

      closeAllFloatingPanelsTablet();

      if (completionListener != null)
        completionListener.run();
    }
  }

  private void adjustCompassAndTraffic(final int offsetY)
  {
    addTask(new MapTask()
    {
      private static final long serialVersionUID = 9177064181621376624L;

      @Override
      public boolean run(@NonNull MwmActivity target)
      {
        updateCompassOffset(offsetY);
        return true;
      }
    });
  }

  @Override
  public void showNavigation(boolean show)
  {
    // TODO:
    // mPlacePage.refreshViews();
    mNavigationController.show(show);
    if (mOnmapDownloader != null)
      mOnmapDownloader.updateState(false);
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
  public void onStartRouteBuilding()
  {
    if (mRoutingPlanInplaceController == null)
      return;

    mRoutingPlanInplaceController.hideDrivingOptionsView();
  }

  @Override
  public void onNavigationCancelled()
  {
    closeFloatingToolbarsAndPanels(true);
    ThemeSwitcher.INSTANCE.restart(isMapRendererActive());
    if (mRoutingPlanInplaceController == null)
      return;

    mRoutingPlanInplaceController.hideDrivingOptionsView();
    NavigationService.stopService(this);
    mMapButtonsViewModel.setSearchOption(null);
    mMapButtonsViewModel.setLayoutMode(MapButtonsController.LayoutMode.regular);
    refreshLightStatusBar();
  }

  @Override
  public void onNavigationStarted()
  {
    closeFloatingToolbarsAndPanels(true);
    ThemeSwitcher.INSTANCE.restart(isMapRendererActive());
    mMapButtonsViewModel.setLayoutMode(MapButtonsController.LayoutMode.navigation);
    refreshLightStatusBar();

    // Don't start the background navigation service without fine location.
    if (ActivityCompat.checkSelfPermission(this, ACCESS_FINE_LOCATION) != PERMISSION_GRANTED)
    {
      Logger.w(LOCATION_TAG, "Permission ACCESS_FINE_LOCATION is not granted, skipping NavigationService");
      return;
    }

    requestPostNotificationsPermission();
    NavigationService.startForegroundService(this);
  }

  @Override
  public void onPlanningCancelled()
  {
    closeFloatingToolbarsAndPanels(true);
    mMapButtonsViewModel.setLayoutMode(MapButtonsController.LayoutMode.regular);
    refreshLightStatusBar();
  }

  @Override
  public void onPlanningStarted()
  {
    closeFloatingToolbarsAndPanels(true);
    mMapButtonsViewModel.setLayoutMode(MapButtonsController.LayoutMode.planning);
    refreshLightStatusBar();
  }

  @Override
  public void onResetToPlanningState()
  {
    closeFloatingToolbarsAndPanels(true);
    ThemeSwitcher.INSTANCE.restart(isMapRendererActive());
    NavigationService.stopService(this);
    mMapButtonsViewModel.setSearchOption(null);
    mMapButtonsViewModel.setLayoutMode(MapButtonsController.LayoutMode.planning);
    refreshLightStatusBar();
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

    closeSearchToolbar(true, true);
  }

  @Override
  public void onDrivingOptionsWarning()
  {
    if (mRoutingPlanInplaceController == null)
      return;

    mRoutingPlanInplaceController.showDrivingOptionView();
  }

  @Override
  public boolean isSubwayEnabled()
  {
    return SubwayManager.from(this).isEnabled();
  }

  @Override
  public void onCommonBuildError(int lastResultCode, @NonNull String[] lastMissingMaps)
  {
    RoutingErrorDialogFragment fragment = RoutingErrorDialogFragment.create(getSupportFragmentManager().getFragmentFactory(),
                                                                            getApplicationContext(), lastResultCode, lastMissingMaps);
    fragment.show(getSupportFragmentManager(), RoutingErrorDialogFragment.class.getSimpleName());
  }

  @Override
  public void onDrivingOptionsBuildError()
  {
    dismissAlertDialog();
    mAlertDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.unable_to_calc_alert_title)
        .setMessage(R.string.unable_to_calc_alert_subtitle)
        .setPositiveButton(R.string.settings, (dialog, which) -> DrivingOptionsActivity.start(this))
        .setNegativeButton(R.string.cancel, null)
        .setOnDismissListener(dialog -> mAlertDialog = null)
        .show();
  }

  @Override
  public void onShowDisclaimer(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    final StringBuilder builder = new StringBuilder();
    for (int resId : new int[]{R.string.dialog_routing_disclaimer_priority, R.string.dialog_routing_disclaimer_precision,
        R.string.dialog_routing_disclaimer_recommendations, R.string.dialog_routing_disclaimer_borders,
        R.string.dialog_routing_disclaimer_beware})
      builder.append(getString(resId)).append("\n\n");

    dismissAlertDialog();
    mAlertDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.dialog_routing_disclaimer_title)
        .setMessage(builder.toString())
        .setCancelable(false)
        .setNegativeButton(R.string.decline, null)
        .setPositiveButton(R.string.accept, (dlg, which) -> {
          Config.acceptRoutingDisclaimer();
          RoutingController.get().prepare(startPoint, endPoint);
        })
        .setOnDismissListener(dialog -> mAlertDialog = null)
        .show();
  }

  private void onSuggestRebuildRoute()
  {
    final RoutingController controller = RoutingController.get();

    // Starting and ending points must be non-null, see {@link #showAddStartOrFinishFrame() }.

    final MapObject endPoint = Objects.requireNonNull(controller.getEndPoint());
    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.p2p_only_from_current)
        .setMessage(R.string.p2p_reroute_from_current)
        .setCancelable(false)
        .setNegativeButton(R.string.cancel, null)
        .setPositiveButton(R.string.ok, MapObject.isOfType(MapObject.MY_POSITION, endPoint) ?
            (dialog, which) -> controller.swapPoints() :
            (dialog, which) -> {
              // The current location may change while this dialog is still shown on the screen.
              final MapObject myPosition = LocationHelper.from(this).getMyPosition();
              controller.setStartPoint(myPosition);
            }
        )
        .setOnDismissListener(dialog -> mAlertDialog = null);
    dismissAlertDialog();
    mAlertDialog = builder.show();
  }

  @Override
  public void onMyPositionModeChanged(int newMode)
  {
    Logger.d(LOCATION_TAG, "newMode = " + newMode);
    mMapButtonsViewModel.setMyPositionMode(newMode);
    RoutingController controller = RoutingController.get();
    if (controller.isPlanning())
      showAddStartOrFinishFrame(controller, true);
  }

  /**
   * Dismiss the active modal dialog from the screen, if any.
   */
  private void dismissAlertDialog()
  {
    if (mAlertDialog != null && mAlertDialog.isShowing())
      mAlertDialog.dismiss();
    mAlertDialog = null;
  }

  /**
   * Dismiss location error dialog from the screen, if any.
   */
  private void dismissLocationErrorDialog()
  {
    if (mLocationErrorDialog != null && mLocationErrorDialog.isShowing())
      mLocationErrorDialog.dismiss();
    mLocationErrorDialog = null;
  }

  /**
   * Called when location is updated.
   * @param location new location
   */
  @Override
  @UiThread
  public void onLocationUpdated(@NonNull Location location)
  {
    dismissLocationErrorDialog();

    final RoutingController routing = RoutingController.get();
    if (!routing.isNavigating())
      return;

    mNavigationController.update(Framework.nativeGetRouteFollowingInfo());
  }

  /**
   * Called when compass data is updated.
   * @param north offset from the north
   */
  @Override
  @UiThread
  public void onCompassUpdated(double north)
  {
    Map.onCompassUpdated(north, false);
    mNavigationController.updateNorth();
  }

  @Override
  @UiThread
  public void onCompassCalibrationRecommended()
  {
    Toast.makeText(this, getString(R.string.compass_calibration_recommended),
        Toast.LENGTH_LONG).show();
  }

  @Override
  @UiThread
  public void onCompassCalibrationRequired()
  {
    Toast.makeText(this, getString(R.string.compass_calibration_required),
        Toast.LENGTH_LONG).show();
  }

  /**
   * Start location services when the user presses a button or starts routing.
   */
  private void startLocation()
  {
    Logger.d(LOCATION_TAG);

    if (ActivityCompat.checkSelfPermission(this, ACCESS_FINE_LOCATION) == PERMISSION_GRANTED)
    {
      Logger.i(LOCATION_TAG, "Permission ACCESS_FINE_LOCATION is granted");
      LocationHelper.from(this).start();
      return;
    }

    // Always try to optimistically request FINE permission when the user presses a button or starts routing.
    // Android will suppress annoying dialogs and skip directly to onLocationPermissionsResult().
    Logger.i(LOCATION_TAG, "Requesting ACCESS_FINE_LOCATION permission");
    dismissLocationErrorDialog();
    mLocationPermissionRequest.launch(new String[]{
        ACCESS_COARSE_LOCATION,
        ACCESS_FINE_LOCATION
    });
  }

  /**
   * Request POST_NOTIFICATIONS permission.
   */
  public void requestPostNotificationsPermission()
  {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU ||
        ActivityCompat.checkSelfPermission(this, POST_NOTIFICATIONS) == PERMISSION_GRANTED)
    {
      Logger.i(TAG, "Permissions POST_NOTIFICATIONS is granted");
      return;
    }

    Logger.i(TAG, "Requesting POST_NOTIFICATIONS permission");
    mPostNotificationPermissionRequest.launch(POST_NOTIFICATIONS);
  }

  /**
   * Start location services explicitly on the start of activity.
   */
  private void autostartLocation()
  {
    if (LocationState.nativeGetMode() == LocationState.NOT_FOLLOW_NO_POSITION)
    {
      Logger.i(LOCATION_TAG, "Location updates are stopped by the user manually.");
      LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
      LocationHelper.from(this).stop();
    }
    else if (ActivityCompat.checkSelfPermission(this, ACCESS_FINE_LOCATION) == PERMISSION_GRANTED)
    {
      Logger.i(LOCATION_TAG, "Permission ACCESS_FINE_LOCATION is granted");
      LocationHelper.from(this).start();
    }
    else if (ActivityCompat.checkSelfPermission(this, ACCESS_COARSE_LOCATION) == PERMISSION_GRANTED)
    {
      Logger.i(LOCATION_TAG, "Permission ACCESS_COARSE_LOCATION is granted");
      LocationHelper.from(this).start();
    }
    else
    {
      Logger.w(LOCATION_TAG, "Permissions ACCESS_COARSE_LOCATION and ACCESS_FINE_LOCATION are not granted");
      LocationState.nativeOnLocationError(LocationState.ERROR_DENIED);
      LocationHelper.from(this).stop();

      Logger.i(LOCATION_TAG, "Requesting ACCESS_FINE_LOCATION + ACCESS_FINE_LOCATION permissions");
      dismissLocationErrorDialog();
      mLocationPermissionRequest.launch(new String[]{
          ACCESS_COARSE_LOCATION,
          ACCESS_FINE_LOCATION
      });
    }
  }

  /**
   * Called on the result of the system location dialog.
   * @param permissions permissions granted or refused.
   */
  @UiThread
  private void onLocationPermissionsResult(java.util.Map<String, Boolean> permissions)
  {
    // Print permissions that have been granted or refused.
    for (java.util.Map.Entry<String, Boolean> entry : permissions.entrySet())
    {
      final String permission = entry.getKey().substring(entry.getKey().lastIndexOf('.') + 1);
      if (entry.getValue())
        Logger.i(LOCATION_TAG, "Permission " + permission + " has been granted");
      else
        Logger.w(LOCATION_TAG, "Permission " + permission + " has been refused");
    }

    // Sic: Android Studio requires explicit calls to checkSelfPermission() for @RequiresPermission in start().
    if (ActivityCompat.checkSelfPermission(this, ACCESS_FINE_LOCATION) == PERMISSION_GRANTED ||
        ActivityCompat.checkSelfPermission(this, ACCESS_COARSE_LOCATION) == PERMISSION_GRANTED)
    {
      LocationHelper.from(this).start();
      return;
    }

    Logger.w(LOCATION_TAG, "Permissions ACCESS_COARSE_LOCATION and ACCESS_FINE_LOCATION have been refused");
    LocationState.nativeOnLocationError(LocationState.ERROR_DENIED);
    LocationHelper.from(this).stop();

    if (mLocationErrorDialog != null && mLocationErrorDialog.isShowing())
    {
      Logger.w(LOCATION_TAG, "Don't show 'location denied' error dialog because another dialog is in progress");
      return;
    }

    mLocationErrorDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.enable_location_services)
        .setMessage(R.string.location_is_disabled_long_text)
        .setOnDismissListener(dialog -> mLocationErrorDialog = null)
        .setNegativeButton(R.string.close, null)
        .show();
  }

  /**
   * Called on the result of the POST_NOTIFICATIONS request.
   * @param granted true if permission has been granted.
   */
  @UiThread
  private void onPostNotificationPermissionResult(boolean granted)
  {
    if (granted)
      Logger.i(TAG, "Permission POST_NOTIFICATIONS has been granted");
    else
      Logger.w(TAG, "Permission POST_NOTIFICATIONS has been refused");
  }

  /**
   * Called by GoogleFusedLocationProvider to request to GPS and/or Wi-Fi.
   * @param pendingIntent an intent to launch.
   */
  @Override
  @UiThread
  public void onLocationResolutionRequired(@NonNull PendingIntent pendingIntent)
  {
    Logger.d(LOCATION_TAG);

    // Cancel our dialog in favor of system dialog.
    dismissLocationErrorDialog();

    // Launch system permission resolution dialog.
    Logger.i(LOCATION_TAG, "Starting location resolution dialog");
    IntentSenderRequest intentSenderRequest = new IntentSenderRequest.Builder(pendingIntent.getIntentSender()).build();
    mLocationResolutionRequest.launch(intentSenderRequest);
  }

  /**
   * Triggered by onLocationResolutionRequired().
   * @param result invocation result.
   */
  @UiThread
  private void onLocationResolutionResult(@NonNull ActivityResult result)
  {
    final int resultCode = result.getResultCode();
    Logger.d(LOCATION_TAG, "resultCode = " + resultCode);

    if (resultCode != Activity.RESULT_OK)
    {
      Logger.w(LOCATION_TAG, "Location resolution has been refused");
      LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
      LocationHelper.from(this).stop();
      return;
    }

    Logger.i(LOCATION_TAG, "Location resolution has been granted, restarting location");
    LocationHelper.from(this).stop();
    startLocation();
  }

  /**
   * Called by AndroidNativeLocationProvider when no suitable location methods are available.
   */
  @Override
  @UiThread
  public void onLocationDisabled()
  {
    Logger.d(LOCATION_TAG, "settings = " + LocationUtils.areLocationServicesTurnedOn(this));

    LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
    LocationHelper.from(this).stop();

    if (mLocationErrorDialog != null && mLocationErrorDialog.isShowing())
    {
      Logger.d(LOCATION_TAG, "Don't show 'location disabled' error dialog because another dialog is in progress");
      return;
    }

    final MaterialAlertDialogBuilder builder = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.enable_location_services)
        .setMessage(R.string.location_is_disabled_long_text)
        .setOnDismissListener(dialog -> mLocationErrorDialog = null)
        .setNegativeButton(R.string.close, null);
    final Intent intent = Utils.makeSystemLocationSettingIntent(this);
    if (intent != null)
    {
      intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
      intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
      intent.addFlags(Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
      builder.setPositiveButton(R.string.connection_settings, (dialog, which) -> startActivity(intent));
    }
    mLocationErrorDialog = builder.show();
  }

  /**
   * Called by the core when location updates were not received after the 30 second deadline.
   */
  @UiThread
  private void onLocationPendingTimeout()
  {
    // Sic: the callback can be called after the activity is destroyed because of being queued.
    if (isDestroyed())
    {
      Logger.w(LOCATION_TAG, "Ignore late callback from core because activity is already destroyed");
      return;
    }

    Logger.d(LOCATION_TAG, "services = " + LocationUtils.areLocationServicesTurnedOn(this));

    //
    // For all cases below we don't stop location provider until user explicitly clicks "Stop" in the dialog.
    //

    if (mLocationErrorDialog != null && mLocationErrorDialog.isShowing())
    {
      Logger.d(LOCATION_TAG, "Don't show 'location timeout' error dialog because another dialog is in progress");
      return;
    }

    mLocationErrorDialog = new MaterialAlertDialogBuilder(this, R.style.MwmTheme_AlertDialog)
        .setTitle(R.string.current_location_unknown_title)
        .setMessage(R.string.current_location_unknown_message)
        .setOnDismissListener(dialog -> mLocationErrorDialog = null)
        .setNegativeButton(R.string.current_location_unknown_stop_button, (dialog, which) ->
        {
          Logger.w(LOCATION_TAG, "Disabled by user");
          LocationState.nativeOnLocationError(LocationState.ERROR_GPS_OFF);
          LocationHelper.from(this).stop();
        })
        .setPositiveButton(R.string.current_location_unknown_continue_button, (dialog, which) ->
        {
          // Do nothing - provider will continue to search location.
        })
        .show();
  }

  @Override
  public void onUseMyPositionAsStart()
  {
    RoutingController.get().setStartPoint(LocationHelper.from(this).getMyPosition());
  }

  @Override
  public void onSearchRoutePoint(@RoutePointInfo.RouteMarkType int pointType)
  {
    RoutingController.get().waitForPoiPick(pointType);
    closeSearchToolbar(true, true);
    showSearch("");
  }

  @Override
  public void onRoutingStart()
  {
    final RoutingController routing = RoutingController.get();
    final MapObject my = LocationHelper.from(this).getMyPosition();
    if (my == null || !MapObject.isOfType(MapObject.MY_POSITION, routing.getStartPoint()))
    {
      onSuggestRebuildRoute();
      return;
    }

    closeFloatingPanels();
    RoutingController.get().start();
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
    Utils.showSnackbar(this, findViewById(R.id.coordinator),
                        success ? R.string.load_kmz_successful : R.string.load_kmz_failed);
  }

  @Override
  public void onSearchClearClick()
  {
    closeSearchToolbar(true, true);
  }

  @Override
  public void onSearchUpClick(@Nullable String query)
  {
    closeFloatingToolbarsAndPanels(true);
    showSearch(query);
  }

  @Override
  public void onSearchQueryClick(@Nullable String query)
  {
    closeFloatingToolbarsAndPanels(true);
    showSearch(query);
  }

  @Override
  public boolean onKeyUp(int keyCode, KeyEvent event)
  {
    switch (keyCode)
    {
      case KeyEvent.KEYCODE_DPAD_DOWN:
        Map.zoomOut();
        return true;
      case KeyEvent.KEYCODE_DPAD_UP:
        Map.zoomIn();
        return true;
      case KeyEvent.KEYCODE_ESCAPE:
        Intent currIntent = getIntent();
        if (currIntent == null || !currIntent.hasExtra(EXTRA_BACK_URL))
          return super.onKeyUp(keyCode, event);

        String backUrl = currIntent.getStringExtra(EXTRA_BACK_URL);
        if (TextUtils.isEmpty(backUrl))
          return super.onKeyUp(keyCode, event);

        Uri back_uri = Uri.parse(backUrl);
        if (back_uri == null)
          return super.onKeyUp(keyCode, event);

        return Utils.openUri(this, back_uri);
      default:
        return super.onKeyUp(keyCode, event);
    }
  }

  public void showTrackOnMap(long trackId)
  {
    Track track = BookmarkManager.INSTANCE.getTrack(trackId);
    Objects.requireNonNull(track);
    Framework.nativeShowTrackRect(trackId);
  }

  public void showBookmarkOnMap(long bookmarkId)
  {
    BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
    Objects.requireNonNull(info);
    BookmarkManager.INSTANCE.showBookmarkOnMap(bookmarkId);
  }

  public void showBookmarkCategoryOnMap(long categoryId)
  {
    BookmarkManager.INSTANCE.showBookmarkCategoryOnMap(categoryId);
  }

  public void onAddPlaceOptionSelected()
  {
    closeFloatingPanels();
    showPositionChooserForEditor(false, false);
  }

  public void onDownloadMapsOptionSelected()
  {
    RoutingController.get().cancel();
    closeFloatingPanels();
    showDownloader(false);
  }

  public void onDonateOptionSelected()
  {
    Utils.openUrl(this, mDonatesUrl);
  }

  public void onSettingsOptionSelected()
  {
    Intent intent = new Intent(this, SettingsActivity.class);
    closeFloatingPanels();
    startActivity(intent);
  }

  public void onShareLocationOptionSelected()
  {
    closeFloatingPanels();
    shareMyLocation();
  }

  public void onLayerChange(Mode mode)
  {
    if (mPreviousLayerMode != mode)
      closeFloatingPanels();
    mPreviousLayerMode = mode;
  }

  @Override
  @Nullable
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id)
  {
    if (id.equals(MAIN_MENU_ID))
    {
      ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
      items.add(new MenuBottomSheetItem(R.string.placepage_add_place_button, R.drawable.ic_plus, this::onAddPlaceOptionSelected));
      items.add(new MenuBottomSheetItem(
          R.string.download_maps,
          R.drawable.ic_download,
          getDownloadMapsCounter(),
          this::onDownloadMapsOptionSelected
      ));
      mDonatesUrl = Config.getDonateUrl();
      if (!TextUtils.isEmpty(mDonatesUrl))
        items.add(new MenuBottomSheetItem(R.string.donate, R.drawable.ic_donate, this::onDonateOptionSelected));
      items.add(new MenuBottomSheetItem(R.string.settings, R.drawable.ic_settings, this::onSettingsOptionSelected));
      items.add(new MenuBottomSheetItem(R.string.share_my_location, R.drawable.ic_share, this::onShareLocationOptionSelected));
      return items;
    }
    return null;
  }

  @Override
  @Nullable
  public Fragment getMenuBottomSheetFragment(String id)
  {
    if (id.equals(LAYERS_MENU_ID))
      return new ToggleMapLayerFragment();
    return null;
  }

  @Override
  public void onPlacePageRequestToggleRouteSettings(@NonNull RoadType roadType)
  {
    closePlacePage();
    RoutingOptions.addOption(roadType);
    rebuildLastRouteInternal();
  }

  @Override
  public void onTrimMemory(int level)
  {
    super.onTrimMemory(level);
    Logger.d(TAG, "trim memory, level = " + level);
    if (level >= TRIM_MEMORY_RUNNING_LOW)
      Framework.nativeMemoryWarning();
  }
}
