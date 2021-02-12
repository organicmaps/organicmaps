package com.mapswithme.maps;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.location.Location;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.KeyEvent;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewTreeObserver;
import android.view.WindowManager;
import android.widget.ImageButton;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StyleRes;
import androidx.appcompat.app.AlertDialog;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentManager;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.Framework.PlacePageActivationListener;
import com.mapswithme.maps.ads.LikesManager;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.auth.PassportAuthDialogFragment;
import com.mapswithme.maps.background.AppBackgroundTracker;
import com.mapswithme.maps.background.NotificationCandidate;
import com.mapswithme.maps.background.Notifier;
import com.mapswithme.maps.base.BaseMwmFragmentActivity;
import com.mapswithme.maps.base.CustomNavigateUpListener;
import com.mapswithme.maps.base.NoConnectionListener;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.AuthBundleFactory;
import com.mapswithme.maps.bookmarks.BookmarkCategoriesActivity;
import com.mapswithme.maps.bookmarks.BookmarksCatalogActivity;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkInfo;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Track;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.dialog.DefaultConfirmationAlertDialog;
import com.mapswithme.maps.dialog.DialogUtils;
import com.mapswithme.maps.discovery.DiscoveryActivity;
import com.mapswithme.maps.discovery.DiscoveryFragment;
import com.mapswithme.maps.discovery.ItemType;
import com.mapswithme.maps.downloader.DownloaderActivity;
import com.mapswithme.maps.downloader.DownloaderFragment;
import com.mapswithme.maps.downloader.OnmapDownloader;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.EditorActivity;
import com.mapswithme.maps.editor.EditorHostFragment;
import com.mapswithme.maps.editor.FeatureCategoryActivity;
import com.mapswithme.maps.editor.OsmOAuth;
import com.mapswithme.maps.editor.ReportFragment;
import com.mapswithme.maps.gallery.Items;
import com.mapswithme.maps.guides.GuidesGalleryListener;
import com.mapswithme.maps.intent.Factory;
import com.mapswithme.maps.intent.MapTask;
import com.mapswithme.maps.location.CompassData;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.maplayer.MapLayerCompositeController;
import com.mapswithme.maps.maplayer.Mode;
import com.mapswithme.maps.maplayer.OnGuidesLayerToggleListener;
import com.mapswithme.maps.maplayer.OnIsolinesLayerToggleListener;
import com.mapswithme.maps.maplayer.ToggleMapLayerDialog;
import com.mapswithme.maps.maplayer.guides.GuidesManager;
import com.mapswithme.maps.maplayer.guides.GuidesState;
import com.mapswithme.maps.maplayer.isolines.IsolinesManager;
import com.mapswithme.maps.maplayer.isolines.IsolinesState;
import com.mapswithme.maps.maplayer.subway.OnSubwayLayerToggleListener;
import com.mapswithme.maps.maplayer.subway.SubwayManager;
import com.mapswithme.maps.maplayer.traffic.OnTrafficLayerToggleListener;
import com.mapswithme.maps.maplayer.traffic.TrafficManager;
import com.mapswithme.maps.maplayer.traffic.widget.TrafficButton;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.news.OnboardingStep;
import com.mapswithme.maps.onboarding.IntroductionDialogFragment;
import com.mapswithme.maps.onboarding.IntroductionScreenFactory;
import com.mapswithme.maps.onboarding.OnboardingTip;
import com.mapswithme.maps.onboarding.WelcomeDialogFragment;
import com.mapswithme.maps.promo.Promo;
import com.mapswithme.maps.promo.PromoAfterBooking;
import com.mapswithme.maps.promo.PromoBookingDialogFragment;
import com.mapswithme.maps.purchase.AdsRemovalActivationCallback;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseControllerProvider;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;
import com.mapswithme.maps.purchase.FailedPurchaseChecker;
import com.mapswithme.maps.purchase.PurchaseCallback;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.maps.purchase.PurchaseFactory;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.maps.routing.NavigationController;
import com.mapswithme.maps.routing.RoutePointInfo;
import com.mapswithme.maps.routing.RoutingBottomMenuListener;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.routing.RoutingErrorDialogFragment;
import com.mapswithme.maps.routing.RoutingOptions;
import com.mapswithme.maps.routing.RoutingPlanFragment;
import com.mapswithme.maps.routing.RoutingPlanInplaceController;
import com.mapswithme.maps.search.BookingFilterParams;
import com.mapswithme.maps.search.FilterActivity;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.maps.search.FloatingSearchToolbarController;
import com.mapswithme.maps.search.HotelsFilter;
import com.mapswithme.maps.search.NativeSearchListener;
import com.mapswithme.maps.search.SearchActivity;
import com.mapswithme.maps.search.SearchEngine;
import com.mapswithme.maps.search.SearchFilterController;
import com.mapswithme.maps.search.SearchFragment;
import com.mapswithme.maps.search.SearchResult;
import com.mapswithme.maps.settings.DrivingOptionsActivity;
import com.mapswithme.maps.settings.RoadType;
import com.mapswithme.maps.settings.SettingsActivity;
import com.mapswithme.maps.settings.StoragePathManager;
import com.mapswithme.maps.settings.UnitLocale;
import com.mapswithme.maps.sound.TtsPlayer;
import com.mapswithme.maps.taxi.TaxiInfo;
import com.mapswithme.maps.taxi.TaxiManager;
import com.mapswithme.maps.tips.Tutorial;
import com.mapswithme.maps.tips.TutorialAction;
import com.mapswithme.maps.widget.FadeView;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.menu.BaseMenu;
import com.mapswithme.maps.widget.menu.MainMenu;
import com.mapswithme.maps.widget.menu.MainMenuOptionListener;
import com.mapswithme.maps.widget.menu.MenuController;
import com.mapswithme.maps.widget.menu.MenuControllerFactory;
import com.mapswithme.maps.widget.menu.MenuStateObserver;
import com.mapswithme.maps.widget.menu.MyPositionButton;
import com.mapswithme.maps.widget.placepage.PlacePageController;
import com.mapswithme.maps.widget.placepage.PlacePageData;
import com.mapswithme.maps.widget.placepage.PlacePageFactory;
import com.mapswithme.maps.widget.placepage.RoutingModeListener;
import com.mapswithme.util.Counters;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.KeyValue;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.PermissionsUtils;
import com.mapswithme.util.SharedPropertiesUtils;
import com.mapswithme.util.ThemeSwitcher;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UTM;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.permissions.PermissionsResult;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.sharing.SharingHelper;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;
import uk.co.samuelwall.materialtaptargetprompt.MaterialTapTargetPrompt;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Stack;

public class MwmActivity extends BaseMwmFragmentActivity
    implements PlacePageActivationListener,
               View.OnTouchListener,
               OnClickListener,
               MapRenderingListener,
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
               AdsRemovalActivationCallback,
               PlacePageController.SlideListener,
               AlertDialogCallback, RoutingModeListener,
               AppBackgroundTracker.OnTransitionListener,
               MaterialTapTargetPrompt.PromptStateChangeListener,
               WelcomeDialogFragment.OnboardingStepPassedListener,
               OnIsolinesLayerToggleListener,
               OnGuidesLayerToggleListener,
               GuidesGalleryListener,
               NoConnectionListener,
               MapWidgetOffsetsProvider,
               SearchToolbarController.RoomsGuestsMenuStateCallback
{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = MwmActivity.class.getSimpleName();

  public static final String EXTRA_TASK = "map_task";
  public static final String EXTRA_LAUNCH_BY_DEEP_LINK = "launch_by_deep_link";
  public static final String EXTRA_BACK_URL = "back_url";
  private static final String EXTRA_CONSUMED = "mwm.extra.intent.processed";
  private static final String EXTRA_ONBOARDING_TIP = "extra_onboarding_tip";

  private static final String[] DOCKED_FRAGMENTS = { SearchFragment.class.getName(),
                                                     DownloaderFragment.class.getName(),
                                                     RoutingPlanFragment.class.getName(),
                                                     EditorHostFragment.class.getName(),
                                                     ReportFragment.class.getName(),
                                                     DiscoveryFragment.class.getName() };

  private static final String EXTRA_LOCATION_DIALOG_IS_ANNOYING = "LOCATION_DIALOG_IS_ANNOYING";
  private static final int REQ_CODE_LOCATION_PERMISSION = 1;
  private static final int REQ_CODE_DISCOVERY = 2;
  private static final int REQ_CODE_SHOW_SIMILAR_HOTELS = 3;
  public static final int REQ_CODE_ERROR_DRIVING_OPTIONS_DIALOG = 5;
  public static final int REQ_CODE_DRIVING_OPTIONS = 6;
  public static final int REQ_CODE_CATALOG_UNLIMITED_ACCESS = 7;
  private static final int REQ_CODE_ISOLINES_ERROR = 8;
  private static final int REQ_CODE_GUIDES_FATAL_ERROR = 9;

  public static final String ERROR_DRIVING_OPTIONS_DIALOG_TAG = "error_driving_options_dialog_tag";
  public static final String GUIDES_FATAL_ERROR_DIALOG_TAG = "guides_fatal_error_dialog_tag";
  public static final String CATALOG_UNLIMITED_ACCESS_DIALOG_TAG = "catalog_unlimited_access_dialog_tag";
  private static final String ISOLINES_ERROR_DIALOG_TAG = "isolines_dialog_tag";

  // Map tasks that we run AFTER rendering initialized
  private final Stack<MapTask> mTasks = new Stack<>();
  private final StoragePathManager mPathManager = new StoragePathManager();

  @Nullable
  private MapFragment mMapFragment;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private FadeView mFadeView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPositionChooser;

  private RoutingPlanInplaceController mRoutingPlanInplaceController;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private NavigationController mNavigationController;

  @SuppressWarnings("NullableProblems")
  @NonNull
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

  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private FloatingSearchToolbarController mSearchController;

  private boolean mLocationErrorDialogAnnoying = false;
  @Nullable
  private Dialog mLocationErrorDialog;

  private boolean mRestoreRoutingPlanFragmentNeeded;
  @Nullable
  private Bundle mSavedForTabletState;
  @Nullable
  private PurchaseController<PurchaseCallback> mAdsRemovalPurchaseController;
  @Nullable
  private PurchaseController<FailedPurchaseChecker> mBookmarkInappPurchaseController;
  @Nullable
  private PurchaseController<PurchaseCallback> mBookmarksAllSubscriptionController;
  @Nullable
  private PurchaseController<PurchaseCallback> mBookmarksSightsSubscriptionController;
  @NonNull
  private final OnClickListener mOnMyPositionClickListener = new CurrentPositionClickListener();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlacePageController mPlacePageController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private MenuController mMainMenuController;
  @Nullable
  private Tutorial mTutorial;
  @Nullable
  private OnboardingTip mOnboardingTip;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Toolbar mPlacePageToolbar;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private Toolbar mBookmarkCategoryToolbar;

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

  @NonNull
  public static Intent createAuthenticateIntent(@NonNull Context context)
  {
    ArrayList<KeyValue> params = new ArrayList<>();
    params.add(new KeyValue(Statistics.EventParam.FROM, Statistics.ParamValue.NOTIFICATION));

    return new Intent(context, MwmActivity.class)
        .addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION)
        .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
        .putExtra(MwmActivity.EXTRA_TASK,
                  new Factory.ShowDialogTask(PassportAuthDialogFragment.class.getName(), params));
  }

  @NonNull
  public static Intent createLeaveReviewIntent(@NonNull Context context,
                                               @NonNull NotificationCandidate.UgcReview nc)
  {
    return new Intent(context, MwmActivity.class)
        .addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION)
        .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP)
        .putExtra(MwmActivity.EXTRA_TASK, new Factory.ShowUGCEditorTask(nc));
  }

  @Override
  public void onRenderingCreated()
  {
    checkMeasurementSystem();
    checkKitkatMigrationMove();

    LocationHelper.INSTANCE.attach(this);
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

    HotelsFilter filter = data.getParcelableExtra(FilterActivity.EXTRA_FILTER);
    mFilterController.setFilter(filter);
    BookingFilterParams params = data.getParcelableExtra(FilterUtils.EXTRA_FILTER_PARAMS);
    mFilterController.setFilterParams(params);

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
        args.putParcelable(FilterActivity.EXTRA_FILTER, mFilterController.getFilter());
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
      mFilterController.resetFilterAndParams();
  }

  public void showEditor()
  {
    // TODO(yunikkk) think about refactoring. It probably should be called in editor.
    Editor.nativeStartEdit();
    Statistics.INSTANCE.trackEditorLaunch(false,
                                          String.valueOf(OsmOAuth.isAuthorized(getApplicationContext())));
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
      final String geoUrl = Framework.nativeGetGe0Url(loc.getLatitude(), loc.getLongitude(), Framework
          .nativeGetDrawScale(), "");
      final String httpUrl = Framework.getHttpGe0Url(loc.getLatitude(), loc.getLongitude(), Framework
          .nativeGetDrawScale(), "");
      final String body = getString(R.string.my_position_share_sms, geoUrl, httpUrl);
      ShareOption.AnyShareOption.ANY.share(this, body);
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
    final Bundle args = new Bundle();
    args.putBoolean(DownloaderActivity.EXTRA_OPEN_DOWNLOADED, openDownloaded);
    if (mIsTabletLayout)
    {
      SearchEngine.INSTANCE.cancel();
      mSearchController.refreshToolbar();
      replaceFragment(DownloaderFragment.class, args, null);
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
    if (savedInstanceState != null)
    {
      mLocationErrorDialogAnnoying = savedInstanceState.getBoolean(EXTRA_LOCATION_DIALOG_IS_ANNOYING);
      mOnboardingTip = savedInstanceState.getParcelable(EXTRA_ONBOARDING_TIP);
    }
    mIsTabletLayout = getResources().getBoolean(R.bool.tabletLayout);

    if (!mIsTabletLayout && (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP))
      getWindow().addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);

    setContentView(R.layout.activity_map);

    mPlacePageController = PlacePageFactory.createCompositePlacePageController(
        this, this, this, this);
    mPlacePageController.initialize(this);
    mPlacePageController.onActivityCreated(this, savedInstanceState);

    mMainMenuController = MenuControllerFactory.createMainMenuController(new MainMenuStateObserver(),
                                                                         new MainMenuOptionSelectedListener(),
                                                                         this);
    mMainMenuController.initialize(findViewById(R.id.coordinator));

    mSearchController = new FloatingSearchToolbarController(this, this, this);
    mSearchController.getToolbar()
                     .getViewTreeObserver()
                     .addOnGlobalLayoutListener(new ToolbarLayoutChangeListener());
    mSearchController.setVisibilityListener(this);

    mPlacePageToolbar = findViewById(R.id.pp_toolbar);
    mBookmarkCategoryToolbar = findViewById(R.id.bookmark_category_toolbar);
    mBookmarkCategoryToolbar.inflateMenu(R.menu.menu_bookmark_catalog);
    mBookmarkCategoryToolbar.setOnMenuItemClickListener(this::onBookmarkToolbarMenuClicked);
    UiUtils.extendViewWithStatusBar(mBookmarkCategoryToolbar);

    boolean isLaunchByDeepLink = getIntent().getBooleanExtra(EXTRA_LAUNCH_BY_DEEP_LINK, false);
    initViews(isLaunchByDeepLink);

    Statistics.INSTANCE.trackConnectionState();

    SearchEngine.INSTANCE.addListener(this);

    SharingHelper.INSTANCE.initialize(this);

    initControllersAndValidatePurchases(savedInstanceState);

    boolean isConsumed = savedInstanceState == null && processIntent(getIntent());
    // If the map activity is launched by any incoming intent (deeplink, update maps event, etc)
    // or it's the first launch (onboarding) we haven't to try restoring the route,
    // showing the tips, etc.
    if (isConsumed || MwmApplication.from(this).isFirstLaunch())
      return;

    if (savedInstanceState == null && RoutingController.get().hasSavedRoute())
    {
      addTask(new Factory.RestoreRouteTask());
      return;
    }

    if (savedInstanceState == null)
      tryToShowAdditionalViewOnTop();
  }

  private boolean onBookmarkToolbarMenuClicked(@NonNull MenuItem item)
  {
    if (item.getItemId() == R.id.close)
    {
      hideBookmarkCategoryToolbar();
      return true;
    }

    return false;
  }

  @Override
  public void onNoConnectionError()
  {
    DialogInterface.OnClickListener listener = (dialog, which) -> {
    };
    DialogUtils.showAlertDialog(this, R.string.common_check_internet_connection_dialog_title,
                                R.string.common_check_internet_connection_dialog,
                                R.string.ok, listener);
  }

  private void initControllersAndValidatePurchases(@Nullable Bundle savedInstanceState)
  {
    mAdsRemovalPurchaseController = PurchaseFactory.createAdsRemovalPurchaseController(this);
    mAdsRemovalPurchaseController.initialize(this);

    mBookmarkInappPurchaseController = PurchaseFactory.createFailedBookmarkPurchaseController(this);
    mBookmarkInappPurchaseController.initialize(this);

    mBookmarksAllSubscriptionController
        = PurchaseFactory.createBookmarksAllSubscriptionController(this);
    mBookmarksAllSubscriptionController.initialize(this);

    mBookmarksSightsSubscriptionController
        = PurchaseFactory.createBookmarksSightsSubscriptionController(this);
    mBookmarksSightsSubscriptionController.initialize(this);

    // To reduce number of parasite validation requests during orientation change.
    if (savedInstanceState == null)
    {
      mAdsRemovalPurchaseController.validateExistingPurchases();
      mBookmarkInappPurchaseController.validateExistingPurchases();
      mBookmarksAllSubscriptionController.validateExistingPurchases();
      mBookmarksSightsSubscriptionController.validateExistingPurchases();
    }
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

    mNavigationController = new NavigationController(this);
    TrafficManager.INSTANCE.attach(mNavigationController);

    initMainMenu();
    initOnmapDownloader();
    initPositionChooser();
    initFilterViews();
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
          Statistics.INSTANCE.trackSearchContextAreaClick(Statistics.ParamValue.LIST);
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
          Statistics.INSTANCE.trackSearchContextAreaClick(Statistics.EventParam.FILTER);
        }

        @Override
        public void onFilterClear()
        {
          runSearch();
        }

        @Override
        public void onFilterParamsChanged()
        {
          FilterUtils.trackFiltersApplying(mFilterController);
          runSearch();
        }
      }, R.string.search_in_table, mSearchController);
    }
  }

  private void runSearch()
  {
    // The previous search should be cancelled before the new one is started, since previous search
    // results are no longer needed.
    SearchEngine.INSTANCE.cancel();

    SearchEngine.INSTANCE.searchInteractive(this, mSearchController.getQuery(), System.nanoTime(),
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
          Statistics.INSTANCE.trackEditorLaunch(true,
                                                String.valueOf(OsmOAuth.isAuthorized(getApplicationContext())));
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
    hideBookmarkCategoryToolbar();
  }

  private void hidePositionChooser()
  {
    UiUtils.hide(mPositionChooser);
    Framework.nativeTurnOffChoosePositionMode();
    setFullscreen(false);
  }

  private void initMap(boolean isLaunchByDeepLink)
  {
    mFadeView = findViewById(R.id.fade_view);
    mFadeView.setListener(this::onFadeViewTouch);

    mMapFragment = (MapFragment) getSupportFragmentManager().findFragmentByTag(MapFragment.class.getName());
    if (mMapFragment == null)
    {
      Bundle args = new Bundle();
      args.putBoolean(MapFragment.ARG_LAUNCH_BY_DEEP_LINK, isLaunchByDeepLink);
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

  private boolean onFadeViewTouch()
  {
    if (!getMainMenuController().isClosed())
      getMainMenuController().close();
    mSearchController.closeBottomMenu();
    return getCurrentMenu().close(true);
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
    View openSubsScreenBtnContainer = frame.findViewById(R.id.subs_screen_btn_container);
    final OnboardingTip tip = OnboardingTip.get();
    boolean hasOnBoardingView = mOnboardingTip == null && tip != null
                                && MwmApplication.from(this).isFirstLaunch();

    mNavAnimationController = new NavigationButtonsAnimationController(
        zoomIn, zoomOut, myPosition, getWindow().getDecorView().getRootView(), this,
        hasOnBoardingView ? openSubsScreenBtnContainer : null);

    UiUtils.showIf(hasOnBoardingView, openSubsScreenBtnContainer);
    if (hasOnBoardingView)
    {
      openSubsScreenBtnContainer.findViewById(R.id.onboarding_btn)
                                .setOnClickListener(v -> onBoardingBtnClicked(tip));
      Statistics.ParameterBuilder builder = Statistics.makeGuidesSubscriptionBuilder();
      Statistics.INSTANCE.trackEvent(Statistics.EventName.MAP_SPONSORED_BUTTON_SHOW, builder);
    }
  }

  private void onBoardingBtnClicked(@NonNull OnboardingTip tip)
  {
    Statistics.ParameterBuilder builder = Statistics.makeGuidesSubscriptionBuilder();
    Statistics.INSTANCE.trackEvent(Statistics.EventName.MAP_SPONSORED_BUTTON_CLICK, builder);
    if (mNavAnimationController == null)
      return;

    mNavAnimationController.hideOnBoardingTipBtn();
    mOnboardingTip = tip;
    OnboardingStep step = com.mapswithme.maps.onboarding.Utils.getOnboardingStepByTip(mOnboardingTip);
    WelcomeDialogFragment.showOnboardinStep(this, step);
  }

  private void initToggleMapLayerController(@NonNull View frame)
  {
    ImageButton trafficBtn = frame.findViewById(R.id.traffic);
    TrafficButton traffic = new TrafficButton(trafficBtn);
    View subway = frame.findViewById(R.id.subway);
    View isoLines = frame.findViewById(R.id.isolines);
    View guides = frame.findViewById(R.id.guides);
    mToggleMapLayerController = new MapLayerCompositeController(traffic,
                                                                subway,
                                                                isoLines,
                                                                guides,
                                                                this);
    mToggleMapLayerController.attachCore();
  }

  public boolean closePlacePage()
  {
    if (mPlacePageController.isClosed())
      return false;

    mPlacePageController.close(true);
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
    getMainMenuController().close();
    if (procAfterClose != null)
      procAfterClose.run();
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

      // TODO: check for tablet.
      closePlacePage();
    });
  }

  public void refreshFade()
  {
    if (getCurrentMenu().isOpen() || !mMainMenuController.isClosed())
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
      return;
    }
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
    mPlacePageController.onSave(outState);
    if (!mIsTabletLayout && RoutingController.get().isPlanning())
      mRoutingPlanInplaceController.onSaveState(outState);

    if (mIsTabletLayout)
    {
      RoutingPlanFragment fragment = (RoutingPlanFragment) getFragment(RoutingPlanFragment.class);
      if (fragment != null)
        fragment.saveRoutingPanelState(outState);
    }

    mNavigationController.onActivitySaveInstanceState(this, outState);

    RoutingController.get().onSaveState();
    outState.putBoolean(EXTRA_LOCATION_DIALOG_IS_ANNOYING, mLocationErrorDialogAnnoying);

    if (mFilterController != null)
      mFilterController.onSaveState(outState);

    if (!isChangingConfigurations())
      RoutingController.get().saveRoute();
    else
      // We no longer need in a saved route if it's a configuration changing: theme switching,
      // orientation changing, etc. Otherwise, the saved route might be restored at undesirable moment.
      RoutingController.get().deleteSavedRoute();

    outState.putParcelable(EXTRA_ONBOARDING_TIP, mOnboardingTip);
    super.onSaveInstanceState(outState);
  }

  @Override
  protected void onRestoreInstanceState(@NonNull Bundle savedInstanceState)
  {
    super.onRestoreInstanceState(savedInstanceState);
    mPlacePageController.onRestore(savedInstanceState);
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

    mNavigationController.onRestoreState(savedInstanceState, this);

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
          showTabletSearch(data, FilterUtils.getHotelCategoryString(this));
          return;
        }
        handleFilterResult(data);
        break;
      case BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY:
        handleDownloadedCategoryResult(data);
        break;
      case REQ_CODE_DRIVING_OPTIONS:
        rebuildLastRoute();
        break;
      case PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION:
        showCatalogUnlimitedAccessDialog();
        break;
    }
  }

  private void showCatalogUnlimitedAccessDialog()
  {
    com.mapswithme.maps.dialog.AlertDialog dialog =
        new com.mapswithme.maps.dialog.AlertDialog.Builder()
            .setTitleId(R.string.popup_subscription_success_map_title)
            .setMessageId(R.string.popup_subscription_success_map_message)
            .setPositiveBtnId(R.string.popup_subscription_success_map_start_button)
            .setNegativeBtnId(R.string.popup_subscription_success_map_not_now_button)
            .setDialogViewStrategyType(com.mapswithme.maps.dialog.AlertDialog.DialogViewStrategyType.CONFIRMATION_DIALOG)
            .setDialogFactory(DefaultConfirmationAlertDialog::new)
            .setReqCode(REQ_CODE_CATALOG_UNLIMITED_ACCESS)
            .setNegativeBtnTextColor(ThemeUtils.getResource(this, R.attr.buttonDialogTextColor))
            .setFragManagerStrategyType(com.mapswithme.maps.dialog.AlertDialog
                                            .FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
            .build();
    dialog.show(this, CATALOG_UNLIMITED_ACCESS_DIALOG_TAG);
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

  @Override
  public void toggleRouteSettings(@NonNull RoadType roadType)
  {
    mPlacePageController.close(true);
    RoutingOptions.addOption(roadType);
    rebuildLastRouteInternal();
  }

  private void handleDownloadedCategoryResult(@NonNull Intent data)
  {
    BookmarkCategory category = data.getParcelableExtra(BookmarksCatalogActivity.EXTRA_DOWNLOADED_CATEGORY);
    if (category == null)
      throw new IllegalArgumentException("Category not found in bundle");

    addTask(new MapTask()
    {
      private static final long serialVersionUID = -7417385158050827655L;

      @Override
      public boolean run(@NonNull MwmActivity target)
      {
        target.showBookmarkCategory(category);
        return true;
      }
    });
    closePlacePage();
  }

  private boolean showBookmarkCategory(@NonNull BookmarkCategory category)
  {
    Framework.nativeShowBookmarkCategory(category.getId());
    return true;
  }

  private void handleDiscoveryResult(@NonNull Intent data)
  {
    BookmarkCategory category =
        data.getParcelableExtra(BookmarksCatalogActivity.EXTRA_DOWNLOADED_CATEGORY);
    if (category != null)
    {
      handleDownloadedCategoryResult(data);
      return;
    }

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

    String query;
    if (data.hasExtra(DiscoveryActivity.EXTRA_FILTER_SEARCH_QUERY))
      query = data.getStringExtra(DiscoveryActivity.EXTRA_FILTER_SEARCH_QUERY);
    else
      query = FilterUtils.getHotelCategoryString(this) + " ";
    mSearchController.setQuery(query);
    mFilterController.setFilter(data.getParcelableExtra(FilterActivity.EXTRA_FILTER));
    if (query.trim().equals(FilterUtils.getHotelCategoryString(this)))
    {
      BookingFilterParams params = mSearchController.getFilterParams() == null
                                   ? FilterUtils.createDefaultParams()
                                   : mSearchController.getFilterParams();
      mFilterController.setFilterParams(params);
    }
    mFilterController.updateFilterButtonsVisibility(mFilterController.isSatisfiedForSearch());
    runSearch();
  }

  @Override
  public void onRouteToDiscoveredObject(@NonNull final MapObject object)
  {
    addTask(new MapTask()
    {
      private static final long serialVersionUID = -219799471997583494L;

      @Override
      public boolean run(@NonNull MwmActivity target)
      {
        RoutingController.get().attach(target);
        RoutingController.get().setRouterType(Framework.ROUTER_TYPE_PEDESTRIAN);
        RoutingController.get().prepare(true, object);
        return false;
      }
    });
  }

  @Override
  public void onShowDiscoveredObject(@NonNull final MapObject object)
  {
    addTask(new MapTask()
    {
      private static final long serialVersionUID = 7499190617762270631L;

      @Override
      public boolean run(@NonNull MwmActivity target)
      {
        Framework.nativeShowFeature(object.getFeatureId());
        return false;
      }
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
    toggleLayer(Mode.SUBWAY, Statistics.ParamValue.MAP);
  }

  @Override
  public void onTrafficLayerSelected()
  {
    toggleLayer(Mode.TRAFFIC, Statistics.ParamValue.MAP);
  }

  @Override
  public void onIsolinesLayerSelected()
  {
    toggleLayer(Mode.ISOLINES, Statistics.ParamValue.MAP);
  }

  @Override
  public void onGuidesLayerSelected()
  {
    toggleLayer(Mode.GUIDES, Statistics.ParamValue.MAP);
  }

  private void onIsolinesStateChanged(@NonNull IsolinesState type)
  {
    if (type != IsolinesState.EXPIREDDATA)
    {
      type.activate(this, findViewById(R.id.coordinator), findViewById(R.id.menu_frame));
      return;
    }

    com.mapswithme.maps.dialog.AlertDialog dialog = new com.mapswithme.maps.dialog.AlertDialog.Builder()
        .setTitleId(R.string.downloader_update_maps)
        .setMessageId(R.string.isolines_activation_error_dialog)
        .setPositiveBtnId(R.string.ok)
        .setNegativeBtnId(R.string.cancel)
        .setFragManagerStrategyType(com.mapswithme.maps.dialog.AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .setReqCode(REQ_CODE_ISOLINES_ERROR)
        .build();
    dialog.show(this, ISOLINES_ERROR_DIALOG_TAG);
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

    Utils.showSnackbar(this, findViewById(R.id.coordinator),
                       findViewById(R.id.menu_frame), R.string.guide_downloaded_title);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_GUIDEDOWNLOADTOAST_SHOWN);
  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups,
                             int tagsLimit)
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
    BookingFilterParams params = intent.getParcelableExtra(FilterUtils.EXTRA_FILTER_PARAMS);
    if (mFilterController != null && (filter != null || params != null))
    {
      mFilterController.updateFilterButtonsVisibility(true);
      mFilterController.show(!TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery()));
      mFilterController.setFilter(filter);
      mFilterController.setFilterParams(params);
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
    mSearchController.refreshToolbar();
    mMainMenu.onResume(null);
    if (Framework.nativeIsInChoosePositionMode())
    {
      UiUtils.show(mPositionChooser);
      setFullscreen(true);
    }
    if (mOnmapDownloader != null)
      mOnmapDownloader.onResume();

    mNavigationController.onActivityResumed(this);

    if (mNavAnimationController != null)
      mNavAnimationController.onResume();
    mPlacePageController.onActivityResumed(this);
    refreshFade();
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

    Context context = getApplicationContext();

    if (!LikesManager.INSTANCE.isNewUser(context) && Counters.isShowReviewForOldUser(context))
    {
      LikesManager.INSTANCE.showRateDialogForOldUser(this);
      Counters.setShowReviewForOldUser(context, false);
    }
    else
    {
      LikesManager.INSTANCE.showDialogs(this);
    }
  }

  @Override
  protected void onPause()
  {
    if (!RoutingController.get().isNavigating())
      TtsPlayer.INSTANCE.stop();
    LikesManager.INSTANCE.cancelDialogs();
    if (mOnmapDownloader != null)
      mOnmapDownloader.onPause();
    mPlacePageController.onActivityPaused(this);
    mNavigationController.onActivityPaused(this);
    super.onPause();
  }

  @Override
  protected void onStart()
  {
    super.onStart();
    Framework.nativePlacePageActivationListener(this);
    BookmarkManager.INSTANCE.addLoadingListener(this);
    BookmarkManager.INSTANCE.addCatalogListener(this);
    RoutingController.get().attach(this);
    IsolinesManager.from(getApplicationContext()).attach(this::onIsolinesStateChanged);
    GuidesManager.from(getApplicationContext()).attach(this::onGuidesStateChanged);
    if (MapFragment.nativeIsEngineCreated())
      LocationHelper.INSTANCE.attach(this);
    mPlacePageController.onActivityStarted(this);
    mSearchController.attach(this);
    MwmApplication.backgroundTracker(getActivity()).addListener(this);
  }

  @Override
  protected void onStop()
  {
    super.onStop();
    Framework.nativeRemovePlacePageActivationListener();
    BookmarkManager.INSTANCE.removeLoadingListener(this);
    BookmarkManager.INSTANCE.removeCatalogListener(this);
    LocationHelper.INSTANCE.detach(!isFinishing());
    RoutingController.get().detach();
    mPlacePageController.onActivityStopped(this);
    MwmApplication.backgroundTracker(getActivity()).removeListener(this);
    IsolinesManager.from(getApplicationContext()).detach();
    GuidesManager.from(getApplicationContext()).detach();
    mSearchController.detach();
  }

  private void onGuidesStateChanged(@NonNull GuidesState state)
  {
    if (state == GuidesState.FATAL_NETWORK_ERROR)
      onGuidesFatalError();
    else if (state == GuidesState.DISABLED)
      onGuidesDisabled();
    else if (state == GuidesState.ENABLED)
      onGuidesEnabled();
    else
      state.activate(this, findViewById(R.id.coordinator), findViewById(R.id.menu_frame));
  }

  private void onGuidesEnabled()
  {
    mToggleMapLayerController.turnOnView(Mode.GUIDES);
  }

  private void onGuidesDisabled()
  {
    mToggleMapLayerController.turnOffCurrentView();
    notifyGuidesAdapters();
  }

  private void onGuidesFatalError()
  {
    mToggleMapLayerController.turnOff();
    showGuidesFatalErrorDialog();
    notifyGuidesAdapters();
  }

  private void notifyGuidesAdapters()
  {
    RecyclerView bottomSheetRecycler = findViewById(R.id.layers_recycler);
    Objects.requireNonNull(bottomSheetRecycler.getAdapter()).notifyDataSetChanged();
    ToggleMapLayerDialog frag = ToggleMapLayerDialog.getInstance(this);
    if (frag == null)
      return;

    RecyclerView recycler = frag.getRootView().findViewById(R.id.recycler);
    Objects.requireNonNull(recycler.getAdapter()).notifyDataSetChanged();
  }

  private void showGuidesFatalErrorDialog()
  {
    com.mapswithme.maps.dialog.AlertDialog dialog =
        new com.mapswithme.maps.dialog.AlertDialog.Builder()
            .setTitleId(R.string.connection_error_dialog_guides_title)
            .setMessageId(R.string.common_check_internet_connection_dialog)
            .setPositiveBtnId(R.string.ok)
            .setDialogViewStrategyType(com.mapswithme.maps.dialog.AlertDialog.DialogViewStrategyType.DEFAULT)
            .setReqCode(REQ_CODE_GUIDES_FATAL_ERROR)
            .setFragManagerStrategyType(com.mapswithme.maps.dialog.AlertDialog
                                            .FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
            .build();
    dialog.show(this, GUIDES_FATAL_ERROR_DIALOG_TAG);
  }

  @CallSuper
  @Override
  protected void onSafeDestroy()
  {
    super.onSafeDestroy();
    if (mAdsRemovalPurchaseController != null)
      mAdsRemovalPurchaseController.destroy();
    if (mBookmarkInappPurchaseController != null)
      mBookmarkInappPurchaseController.destroy();
    if (mBookmarksAllSubscriptionController != null)
      mBookmarksAllSubscriptionController.destroy();
    if (mBookmarksSightsSubscriptionController != null)
      mBookmarksSightsSubscriptionController.destroy();
    mNavigationController.destroy();
    mToggleMapLayerController.detachCore();
    TrafficManager.INSTANCE.detachAll();
    mPlacePageController.destroy();
    getMainMenuController().destroy();
    SearchEngine.INSTANCE.removeListener(this);
  }

  @Override
  public void onBackPressed()
  {
    if (getCurrentMenu().close(true))
    {
      mFadeView.fadeOut();
      return;
    }

    if (!getMainMenuController().isClosed())
    {
      getMainMenuController().close();
      return;
    }

    if (mSearchController.closeBottomMenu())
      return;

    if (mSearchController.hide())
    {
      SearchEngine.INSTANCE.cancelInteractiveSearch();
      if (mFilterController != null)
        mFilterController.resetFilterAndParams();
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

    if (UiUtils.isVisible(mBookmarkCategoryToolbar) && mPlacePageController.isClosed())
    {
      hideBookmarkCategoryToolbar();
      return;
    }

    if (!closePlacePage() && !closeSidePanel() && !isRoutingCancelled
        && !closePositionChooser())
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

  // Called from JNI.
  @Override
  public void onPlacePageActivated(@NonNull PlacePageData data)
  {
    if (data instanceof MapObject)
    {
      MapObject object = (MapObject) data;
      if (MapObject.isOfType(MapObject.API_POINT, object))
      {
        final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
        if (request == null)
          return;

        request.setPointData(object.getLat(), object.getLon(), object.getTitle(), object.getApiId());
        object.setSubtitle(request.getCallerName(MwmApplication.from(this)).toString());
      }
    }

    setFullscreen(false);

    mPlacePageController.openFor(data);

    if (UiUtils.isVisible(mFadeView))
      mFadeView.fadeOut();
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

      setFullscreen(!mIsFullscreen);
    }
    else
    {
      mPlacePageController.close(true);
    }
  }

  @NonNull
  private BaseMenu getCurrentMenu()
  {
    return (RoutingController.get().isNavigating()
            ? mNavigationController.getNavMenu()
            : mMainMenu);
  }

  private void setFullscreen(boolean isFullscreen)
  {
    if (RoutingController.get().isNavigating()
        || RoutingController.get().isBuilding()
        || RoutingController.get().isPlanning())
      return;

    if (UiUtils.isVisible(mBookmarkCategoryToolbar))
      return;

    mIsFullscreen = isFullscreen;
    final BaseMenu menu = getCurrentMenu();

    if (isFullscreen)
    {
      if (menu.isAnimating())
        return;

      mIsFullscreenAnimating = true;

      showLineFrame(false);

      mIsFullscreenAnimating = false;
      if (mIsAppearMenuLater)
      {
        appearMenu(menu);
        mIsAppearMenuLater = false;
      }

      if (mNavAnimationController != null)
        mNavAnimationController.disappearZoomButtons();
      if (mNavMyPosition != null)
        mNavMyPosition.hide();
      mToggleMapLayerController.hide();
    }
    else
    {
      if (mPlacePageController.isClosed() && mNavAnimationController != null)
        mNavAnimationController.appearZoomButtons();
      if (!mIsFullscreenAnimating)
        appearMenu(menu);
      else
        mIsAppearMenuLater = true;
    }
  }

  private void appearMenu(BaseMenu menu)
  {
    showLineFrame(true);
    showNavMyPositionBtn();
    mToggleMapLayerController.applyLastActiveMode();
  }

  private void showNavMyPositionBtn()
  {
    if (mNavMyPosition != null)
      mNavMyPosition.show();
  }

  @Override
  public void onPlacePageSlide(int top)
  {
    if (mNavAnimationController != null)
      mNavAnimationController.move(top);
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
    return mMapFragment != null && mMapFragment.onTouch(view, event);
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

  void adjustCompass(int offsetY)
  {
    Context context = getApplicationContext();
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    int resultOffset = offsetY;
    //If the compass is covered by navigation buttons, we move it beyond the visible screen
    if (mNavAnimationController != null && mNavAnimationController.isConflictWithCompass(offsetY))
    {
      int halfHeight = (int) (UiUtils.dimen(context, R.dimen.compass_height) * 0.5f);
      int margin = UiUtils.dimen(context, R.dimen.margin_compass_top)
                   + UiUtils.dimen(context, R.dimen.nav_frame_padding);
      resultOffset = -(offsetY + halfHeight + margin);
    }

    mMapFragment.setupCompass(resultOffset, true);

    CompassData compass = LocationHelper.INSTANCE.getCompassData();
    if (compass != null)
      MapFragment.nativeCompassUpdated(compass.getNorth(), true);
  }

  private void adjustBottomWidgets(int offsetY)
  {
    if (mMapFragment == null || !mMapFragment.isAdded())
      return;

    mMapFragment.setupRuler(offsetY, false);
    mMapFragment.setupWatermark(offsetY, true);
  }

  @Override
  public int getRulerOffsetY()
  {
    return getBottomMapWidgetOffsetY();
  }

  private int getBottomMapWidgetOffsetY()
  {
    View menuView = getCurrentMenu().getFrame();
    return UiUtils.isVisible(menuView) ? 0 : menuView.getHeight();
  }

  @Override
  public int getWaterMarkOffsetY()
  {
    return getBottomMapWidgetOffsetY();
  }

  @Override
  public FragmentActivity getActivity()
  {
    return this;
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

    mNavigationController.showSearchButtons(RoutingController.get().isPlanning()
                                            || RoutingController.get().isBuilt());

    if (RoutingController.get().isNavigating())
    {
      mNavigationController.show(true);
      mSearchController.hide();
      mMainMenu.setState(MainMenu.State.NAVIGATION, false, mIsFullscreen);
      return;
    }

    if (RoutingController.get().isPlanning())
    {
      mMainMenu.setState(MainMenu.State.ROUTE_PREPARE, false, mIsFullscreen);
      return;
    }

    mMainMenu.setState(MainMenu.State.MENU, false, mIsFullscreen);
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

  private boolean adjustMenuLineFrameVisibility()
  {
    final RoutingController controller = RoutingController.get();

    if (controller.isBuilt() || controller.isTaxiRequestHandled())
    {
      showLineFrame(true);
      return true;
    }

    if (controller.isPlanning() || controller.isBuilding() || controller.isErrorEncountered())
    {
      if (showAddStartOrFinishFrame(controller, true))
      {
        return true;
      }

      showLineFrame(false);
      return false;
    }

    if (UiUtils.isVisible(mBookmarkCategoryToolbar))
    {
      showLineFrame(false);
      return false;
    }

    hideRoutingActionFrame();
    showLineFrame(true);
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

    MapObject myPosition = LocationHelper.INSTANCE.getMyPosition();

    if (myPosition != null && !controller.hasEndPoint())
    {
      showAddFinishFrame();
      if (showFrame)
        showLineFrame(true);
      return true;
    }
    if (!controller.hasStartPoint())
    {
      showAddStartFrame();
      if (showFrame)
        showLineFrame(true);
      return true;
    }
    if (!controller.hasEndPoint())
    {
      showAddFinishFrame();
      if (showFrame)
        showLineFrame(true);
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

  private void showLineFrame(boolean show)
  {
    UiUtils.showIf(show, getCurrentMenu().getFrame());
    adjustBottomWidgets(show ? 0 : getBottomMapWidgetOffsetY());
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

    int totalHeight = calcFloatingViewsOffset();

    mNavAnimationController.setTopLimit(!show ? 0 : totalHeight);
    mNavAnimationController.setBottomLimit(!show ? 0 : getCurrentMenu().getFrame().getHeight());
    adjustCompassAndTraffic(!show ? UiUtils.getStatusBarHeight(getApplicationContext())
                                  : totalHeight);
  }

  @Override
  public void showRoutePlan(boolean show, @Nullable Runnable completionListener)
  {
    Context context = getApplicationContext();
    if (show)
    {
      mSearchController.hide();
      hideBookmarkCategoryToolbar();
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
        int width = UiUtils.dimen(context, R.dimen.panel_width);
        adjustTraffic(width, UiUtils.getStatusBarHeight(context));
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
        mNavigationController.adjustSearchButtons(0);
      }
      else
      {
        mRoutingPlanInplaceController.show(false);
      }

      closeAllFloatingPanels();
      mNavigationController.resetSearchWheel();

      if (completionListener != null)
        completionListener.run();

      updateSearchBar();
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

    adjustCompassAndTraffic(visible ? calcFloatingViewsOffset()
                                    : UiUtils.getStatusBarHeight(getApplicationContext()));
    int toolbarHeight = mSearchController.getToolbar().getHeight();
    setNavButtonsTopLimit(visible ? toolbarHeight : 0);
    if (mFilterController != null)
    {
      boolean show = visible && !TextUtils.isEmpty(SearchEngine.INSTANCE.getQuery())
                     && !RoutingController.get().isNavigating();
      mFilterController.show(show);
      mMainMenu.show(!show);
    }
  }

  private int calcFloatingViewsOffset()
  {
    int offset;
    if (mRoutingPlanInplaceController == null
        || (offset = mRoutingPlanInplaceController.calcHeight()) == 0)
      return UiUtils.getStatusBarHeight(this);

    return offset;
  }

  @Override
  public void onResultsUpdate(SearchResult[] results, long timestamp, boolean isHotel)
  {
    if (mFilterController != null)
      mFilterController.updateFilterButtonsVisibility(isHotel);
  }

  @Override
  public void onResultsEnd(long timestamp, boolean isHotel)
  {
    if (mFilterController != null)
      mFilterController.updateFilterButtonsVisibility(isHotel);
  }

  @Override
  public void showNavigation(boolean show)
  {
    // TODO:
//    mPlacePage.refreshViews();
    mNavigationController.show(show);
    refreshFade();
    if (mOnmapDownloader != null)
      mOnmapDownloader.updateState(false);
    if (show)
    {
      mSearchController.clear();
      mSearchController.hide();
      if (mFilterController != null)
        mFilterController.show(false);
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
  public void onStartRouteBuilding()
  {
    if (mRoutingPlanInplaceController == null)
      return;

    mRoutingPlanInplaceController.hideDrivingOptionsView();
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
    updateSearchBar();
    ThemeSwitcher.INSTANCE.restart(isMapRendererActive());
    if (mRoutingPlanInplaceController == null)
      return;

    mRoutingPlanInplaceController.hideDrivingOptionsView();
    mNavigationController.stop(this);
  }

  @Override
  public void onNavigationStarted()
  {
    ThemeSwitcher.INSTANCE.restart(isMapRendererActive());
    mNavigationController.start(this);
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

    mNavigationController.resetSearchWheel();
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
    RoutingErrorDialogFragment fragment = RoutingErrorDialogFragment.create(getApplicationContext(),
                                                                            lastResultCode, lastMissingMaps);
    fragment.show(getSupportFragmentManager(), RoutingErrorDialogFragment.class.getSimpleName());
  }

  @Override
  public void onDrivingOptionsBuildError()
  {
    com.mapswithme.maps.dialog.AlertDialog dialog =
        new com.mapswithme.maps.dialog.AlertDialog.Builder()
            .setTitleId(R.string.unable_to_calc_alert_title)
            .setMessageId(R.string.unable_to_calc_alert_subtitle)
            .setPositiveBtnId(R.string.settings)
            .setNegativeBtnId(R.string.cancel)
            .setReqCode(REQ_CODE_ERROR_DRIVING_OPTIONS_DIALOG)
            .setFragManagerStrategyType(com.mapswithme.maps.dialog.AlertDialog
                                            .FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
            .build();
    dialog.show(this, ERROR_DRIVING_OPTIONS_DIALOG_TAG);
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
    if (!RoutingController.get().isNavigating())
      return;

    mNavigationController.update(Framework.nativeGetRouteFollowingInfo());

    TtsPlayer.INSTANCE.playTurnNotifications(getApplicationContext());
  }

  @Override
  public void onCompassUpdated(@NonNull CompassData compass)
  {
    MapFragment.nativeCompassUpdated(compass.getNorth(), false);
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
    mNavigationController.updateSearchButtonsTranslation(translation);
  }

  @Override
  public void onFadeInZoomButtons()
  {
    if (RoutingController.get().isPlanning() || RoutingController.get().isNavigating())
      mNavigationController.fadeInSearchButtons();
  }

  @Override
  public void onFadeOutZoomButtons()
  {
    if (RoutingController.get().isPlanning() || RoutingController.get().isNavigating())
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
        .setOnCancelListener(dialog -> mLocationErrorDialogAnnoying = true)
        .setPositiveButton(R.string.connection_settings, (dialog, which) -> startActivity(intent)).show();
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
      LocationHelper.INSTANCE.switchToNextMode();
    };

    new AlertDialog.Builder(this)
        .setMessage(message)
        .setNegativeButton(R.string.current_location_unknown_stop_button, stopClickListener)
        .setPositiveButton(R.string.current_location_unknown_continue_button, continueClickListener)
        .setCancelable(false)
        .show();
  }

  @Override
  public void onPromptStateChanged(@NonNull MaterialTapTargetPrompt prompt, int state)
  {
    if (mTutorial == null)
      return;

    if (state != MaterialTapTargetPrompt.STATE_DISMISSED
        && state != MaterialTapTargetPrompt.STATE_FINISHED)
    {
      return;
    }

    UserActionsLogger.logTipClickedEvent(mTutorial, TutorialAction.GOT_IT_CLICKED);
    Statistics.INSTANCE.trackTipsClose(mTutorial.ordinal());
    mTutorial = null;
  }

  @Nullable
  public Tutorial getTutorial()
  {
    return mTutorial;
  }

  public void setTutorial(@NonNull Tutorial tutorial)
  {
    mTutorial = tutorial;
    if (mTutorial.isLayer())
      mToggleMapLayerController.turnOff();
    mToggleMapLayerController.setTutorial(tutorial);
  }

  private void tryToShowTutorial()
  {
    addTask(new Factory.ShowTutorialTask());
  }

  private boolean tryToShowPromoAfterBooking()
  {
    NetworkPolicy policy = NetworkPolicy.newInstance(NetworkPolicy.getCurrentNetworkUsageStatus());
    PromoAfterBooking promo = Promo.nativeGetPromoAfterBooking(policy);
    if (promo == null)
      return false;

    String dialogName = PromoBookingDialogFragment.class.getName();
    if (getSupportFragmentManager().findFragmentByTag(dialogName) != null)
      return true;

    final Bundle args = new Bundle();
    args.putString(PromoBookingDialogFragment.EXTRA_CITY_GUIDES_URL, promo.getGuidesUrl());
    args.putString(PromoBookingDialogFragment.EXTRA_CITY_IMAGE_URL, promo.getImageUrl());

    final DialogFragment fragment = (DialogFragment) Fragment.instantiate(this, dialogName, args);
    fragment.show(getSupportFragmentManager(), dialogName);

    UserActionsLogger.logPromoAfterBookingShown(promo.getId());
    Statistics.INSTANCE.trackEvent(Statistics.EventName.INAPP_SUGGESTION_SHOWN,
                                   Statistics.makeInAppSuggestionParamBuilder());
    return true;
  }

  private void tryToShowAdditionalViewOnTop()
  {
    if (tryToShowPromoAfterBooking())
      return;

    tryToShowTutorial();
  }

  @Override
  public void onTransit(boolean foreground)
  {
    if (foreground)
      tryToShowAdditionalViewOnTop();
  }

  @Override
  public void onUseMyPositionAsStart()
  {
    RoutingController.get().setStartPoint(LocationHelper.INSTANCE.getMyPosition());
  }

  @Override
  public void onSearchRoutePoint(@RoutePointInfo.RouteMarkType int pointType)
  {
    RoutingController.get().waitForPoiPick(pointType);
    mNavigationController.resetSearchWheel();
    showSearch("");
    Statistics.INSTANCE.trackRoutingTooltipEvent(pointType, true);
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
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    if (requestCode == REQ_CODE_ERROR_DRIVING_OPTIONS_DIALOG)
      DrivingOptionsActivity.start(this);
    else if (requestCode == REQ_CODE_CATALOG_UNLIMITED_ACCESS)
      BookmarksCatalogActivity.startForResult(this,
                                              BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                              BookmarkManager.INSTANCE.getCatalogFrontendUrl(UTM.UTM_NONE));
    else if (requestCode == REQ_CODE_ISOLINES_ERROR)
      startActivity(new Intent(this, DownloaderActivity.class));
    else if (requestCode == FilterUtils.REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
      Utils.showSystemConnectionSettings(this);
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    // Do nothing
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    // Do nothing
  }

  @Override
  public void onBookmarksFileLoaded(boolean success)
  {
    Utils.showSnackbar(this, findViewById(R.id.coordinator), findViewById(R.id.menu_frame),
                        success ? R.string.load_kmz_successful : R.string.load_kmz_failed);
  }

  @Override
  public void onSearchClearClick()
  {
    if (mFilterController != null)
      mFilterController.resetFilterAndParams();
    closePlacePage();
  }

  @Override
  public void onSearchUpClick(@Nullable String query)
  {
    closePlacePage();
    showSearch(query);
  }

  @Override
  public void onSearchQueryClick(@Nullable String query)
  {
    showSearch(query);
  }

  @Override
  public void onRoomsGuestsMenuStateChange(boolean isOpen)
  {
    if (isOpen)
      closePlacePage();
  }

  public void showIntroductionScreenForDeeplink(@NonNull String deepLink,
                                                @NonNull IntroductionScreenFactory factory)
  {
    IntroductionDialogFragment.show(getSupportFragmentManager(), deepLink, factory);
  }

  @Override
  public boolean onKeyUp(int keyCode, KeyEvent event)
  {
    switch (keyCode)
    {
      case KeyEvent.KEYCODE_DPAD_DOWN:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ZOOM_OUT);
        MapFragment.nativeScaleMinus();
        return true;
      case KeyEvent.KEYCODE_DPAD_UP:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.ZOOM_IN);
        MapFragment.nativeScalePlus();
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

  public void onOnboardingStepPassed(@NonNull OnboardingStep step)
  {
    if (mOnboardingTip == null)
      throw new AssertionError("Onboarding tip must be non-null at this point!");

    switch (step)
    {
      case DISCOVER_GUIDES:
      case CHECK_OUT_SIGHTS:
        BookmarksCatalogActivity.startForResult(this,
                                                BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY,
                                                mOnboardingTip.getUrl());
        break;
      case SUBSCRIBE_TO_CATALOG:
        BookmarksAllSubscriptionActivity.startForResult(this);
        break;
      default:
        throw new UnsupportedOperationException("Onboarding step '" + step + "' not supported " +
                                                "for sponsored button");
    }
  }

  @Override
  public void onLastOnboardingStepPassed()
  {
    // Do nothing by default.
  }

  @Override
  public void onOnboardingStepCancelled()
  {
    // Do nothing by default.
  }

  @Override
  public void onGalleryGuideSelected(@NonNull String url)
  {
    BookmarksCatalogActivity.startForResult(
        this, BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY, url,
        AuthBundleFactory.guideCatalogue());
  }

  private void toggleLayer(@NonNull Mode mode, @NonNull String from)
  {
    boolean isEnabled = mode.isEnabled(getApplicationContext());
    Statistics.INSTANCE.trackMapLayerClick(mode, from, isEnabled);
    mToggleMapLayerController.toggleMode(mode);
  }

  @NonNull
  private MenuController getMainMenuController()
  {
    return mMainMenuController;
  }

  public void showTrackOnMap(long trackId)
  {
    Track track = BookmarkManager.INSTANCE.getTrack(trackId);
    Objects.requireNonNull(track);
    setupBookmarkCategoryToolbar(track.getCategoryId());
    Framework.nativeShowTrackRect(trackId);
  }

  public void showBookmarkOnMap(long bookmarkId)
  {
    BookmarkInfo info = BookmarkManager.INSTANCE.getBookmarkInfo(bookmarkId);
    Objects.requireNonNull(info);
    setupBookmarkCategoryToolbar(info.getCategoryId());
    BookmarkManager.INSTANCE.showBookmarkOnMap(bookmarkId);
  }

  public void showBookmarkCategoryOnMap(long categoryId)
  {
    setupBookmarkCategoryToolbar(categoryId);
    BookmarkManager.INSTANCE.showBookmarkCategoryOnMap(categoryId);
  }

  private void setupBookmarkCategoryToolbar(long categoryId)
  {
    final BookmarkCategory category = BookmarkManager.INSTANCE.getCategoryById(categoryId);
    mBookmarkCategoryToolbar.setTitle(category.getName());
    UiUtils.setupNavigationIcon(mBookmarkCategoryToolbar, v -> {
      BookmarkCategoriesActivity.startForResult(MwmActivity.this, category);
      closePlacePage();
      hideBookmarkCategoryToolbar();
    });

    showBookmarkCategoryToolbar();
  }

  private void showBookmarkCategoryToolbar()
  {
    UiUtils.show(mBookmarkCategoryToolbar);
    adjustCompassAndTraffic(mBookmarkCategoryToolbar.getHeight());
    adjustMenuLineFrameVisibility();
  }

  private void hideBookmarkCategoryToolbar()
  {
    UiUtils.hide(mBookmarkCategoryToolbar);
    adjustCompassAndTraffic(UiUtils.getStatusBarHeight(MwmActivity.this));
    adjustMenuLineFrameVisibility();
  }

  private class CurrentPositionClickListener implements OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      Statistics.INSTANCE.trackEvent(Statistics.EventName.TOOLBAR_MY_POSITION);
      AlohaHelper.logClick(AlohaHelper.TOOLBAR_MY_POSITION);

      if (!PermissionsUtils.isLocationGranted(getApplicationContext()))
      {
        if (PermissionsUtils.isLocationExplanationNeeded(MwmActivity.this))
          PermissionsUtils.requestLocationPermission(MwmActivity.this, REQ_CODE_LOCATION_PERMISSION);
        else
          Utils.showSnackbar(getActivity(), findViewById(R.id.coordinator), findViewById(R.id.menu_frame),
                             R.string.enable_location_services);
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
      Tutorial api = Tutorial.requestCurrent(getActivity(), getActivity().getClass());
      LOGGER.d(TAG, "Tutorial = " + api);
      if (getItem() == api.getSiblingMenuItem())
      {
        api.createClickInterceptor().onInterceptClick(getActivity());
        Statistics.INSTANCE.trackTipsEvent(Statistics.EventName.TIPS_TRICKS_CLICK, api.ordinal());
      }
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
      Statistics.INSTANCE.trackToolbarClick(getItem());
      getActivity().closePlacePage();
      getActivity().closeSidePanel();
      MenuController controller = getActivity().getMainMenuController();
      if (controller.isClosed())
        controller.open();
      else
        controller.close();
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

  public abstract static class StatisticClickMenuDelegate extends AbstractClickMenuDelegate
  {
    StatisticClickMenuDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    public void onMenuItemClickInternal()
    {
      Statistics.INSTANCE.trackToolbarMenu(getItem());
      onPostStatisticMenuItemClick();
    }

    abstract void onPostStatisticMenuItemClick();
  }

  public static class BookmarksDelegate extends StatisticClickMenuDelegate
  {
    public BookmarksDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    void onPostStatisticMenuItemClick()
    {
      getActivity().closeMenu(getActivity()::showBookmarks);
    }
  }

  public static class DiscoveryDelegate extends StatisticClickMenuDelegate
  {
    public DiscoveryDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    void onPostStatisticMenuItemClick()
    {
      getActivity().showDiscovery();
    }
  }

  public static class PointToPointDelegate extends StatisticClickMenuDelegate
  {
    public PointToPointDelegate(@NonNull MwmActivity activity, @NonNull MainMenu.Item item)
    {
      super(activity, item);
    }

    @Override
    void onPostStatisticMenuItemClick()
    {
      getActivity().startLocationToPoint(null, false);
    }
  }

  private class ToolbarLayoutChangeListener implements ViewTreeObserver.OnGlobalLayoutListener
  {
    @Override
    public void onGlobalLayout()
    {
      mSearchController.getToolbar().getViewTreeObserver()
                       .removeGlobalOnLayoutListener(this);

      adjustCompassAndTraffic(UiUtils.isVisible(mSearchController.getToolbar())
                              ? calcFloatingViewsOffset()
                              : UiUtils.getStatusBarHeight(getApplicationContext()));
    }
  }

  private class MainMenuStateObserver implements MenuStateObserver
  {

    @Override
    public void onMenuOpen()
    {
      mFadeView.fadeIn();
      if (!SharedPropertiesUtils.shouldShowLayerTutorialToast(getApplicationContext()))
        return;
      Utils.showSnackbar(getApplicationContext(), findViewById(R.id.coordinator),
                         findViewById(R.id.menu_frame), R.string.routes_layer_in_menu_toast);
    }

    @Override
    public void onMenuClosed()
    {
      mFadeView.fadeOut();
      getCurrentMenu().updateMarker();
    }
  }

  private class MainMenuOptionSelectedListener implements MainMenuOptionListener
  {
    @Override
    public void onAddPlaceOptionSelected()
    {
      Statistics.INSTANCE.trackToolbarMenu(MainMenu.Item.ADD_PLACE);
      closePlacePage();
      closeMenu(() -> showPositionChooser(false, false));
    }

    @Override
    public void onSearchGuidesOptionSelected()
    {
      Statistics.INSTANCE.trackToolbarMenu(MainMenu.Item.DOWNLOAD_GUIDES);
      int requestCode = BookmarkCategoriesActivity.REQ_CODE_DOWNLOAD_BOOKMARK_CATEGORY;
      String catalogUrl = BookmarkManager.INSTANCE.getCatalogFrontendUrl(UTM.UTM_TOOLBAR_BUTTON);
      closeMenu(() -> BookmarksCatalogActivity.startForResult(getActivity(), requestCode,
                                                              catalogUrl));
    }

    @Override
    public void onDownloadMapsOptionSelected()
    {
      Statistics.INSTANCE.trackToolbarMenu(MainMenu.Item.DOWNLOAD_MAPS);
      RoutingController.get().cancel();
      closeMenu(() -> showDownloader(false));
    }

    @Override
    public void onSettingsOptionSelected()
    {
      Statistics.INSTANCE.trackToolbarMenu(MainMenu.Item.SETTINGS);
      Intent intent = new Intent(getActivity(), SettingsActivity.class);
      closeMenu(() -> getActivity().startActivity(intent));
    }

    @Override
    public void onShareLocationOptionSelected()
    {
      Statistics.INSTANCE.trackToolbarMenu(MainMenu.Item.SHARE_MY_LOCATION);
      closeMenu(MwmActivity.this::shareMyLocation);
    }

    @Override
    public void onSubwayLayerOptionSelected()
    {
      toggleLayer(Mode.SUBWAY, Statistics.ParamValue.MENU);
    }

    @Override
    public void onTrafficLayerOptionSelected()
    {
      toggleLayer(Mode.TRAFFIC, Statistics.ParamValue.MENU);
    }

    @Override
    public void onIsolinesLayerOptionSelected()
    {
      toggleLayer(Mode.ISOLINES, Statistics.ParamValue.MENU);
    }

    @Override
    public void onGuidesLayerOptionSelected()
    {
      toggleLayer(Mode.GUIDES, Statistics.ParamValue.MENU);
    }
  }
}
