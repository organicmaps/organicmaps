package app.organicmaps.car.screens;

import static android.text.Spanned.SPAN_EXCLUSIVE_EXCLUSIVE;
import static android.text.Spanned.SPAN_INCLUSIVE_INCLUSIVE;
import static java.util.Objects.requireNonNull;

import android.content.Intent;
import android.net.Uri;
import android.text.SpannableString;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.CarToast;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.DurationSpan;
import androidx.car.app.model.ForegroundCarColorSpan;
import androidx.car.app.model.Header;
import androidx.car.app.model.Pane;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Metadata;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.settings.DrivingOptionsScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.OnBackPressedCallback;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.routing.RoutingInfo;
import app.organicmaps.util.Config;
import app.organicmaps.util.log.Logger;

public class PlaceScreen extends BaseMapScreen implements OnBackPressedCallback.Callback, RoutingController.Container
{
  private static final String TAG = PlaceScreen.class.getSimpleName();

  @NonNull
  private MapObject mMapObject;

  @NonNull
  private final RoutingController mRoutingController;

  @NonNull
  private final OnBackPressedCallback mOnBackPressedCallback;

  private PlaceScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext, builder.mSurfaceRenderer);
    mMapObject = requireNonNull(builder.mMapObject);
    mRoutingController = RoutingController.get();
    mOnBackPressedCallback = new OnBackPressedCallback(getCarContext(), this);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MapTemplate.Builder builder = new MapTemplate.Builder();
    builder.setHeader(createHeader());
    builder.setActionStrip(UiHelpers.createSettingsActionStrip(this, getSurfaceRenderer()));
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setPane(createPane());

    return builder.build();
  }

  @NonNull
  public MapObject getMapObject()
  {
    return mMapObject;
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    getCarContext().getOnBackPressedDispatcher().addCallback(this, mOnBackPressedCallback);
    if (!mRoutingController.isBuilding() && !mRoutingController.isBuilt())
      builder.addEndHeaderAction(createBookmarkAction());
    if (mRoutingController.isBuilt())
      builder.addEndHeaderAction(createDrivingOptionsAction());

    return builder.build();
  }

  @NonNull
  private Pane createPane()
  {
    final Pane.Builder builder = new Pane.Builder();

    if (mRoutingController.isBuilding())
    {
      builder.setLoading(true);
      return builder.build();
    }

    builder.addRow(getPlaceDescription());
    if (mRoutingController.isBuilt())
      builder.addRow(getPlaceRouteInfo());

    final Row placeOpeningHours = UiHelpers.getPlaceOpeningHoursRow(mMapObject, getCarContext());
    if (placeOpeningHours != null)
      builder.addRow(placeOpeningHours);

    createPaneActions(builder);

    return builder.build();
  }

  @NonNull
  private Row getPlaceDescription()
  {
    final Row.Builder builder = new Row.Builder();
    builder.setTitle(mMapObject.getTitle());
    if (!mMapObject.getSubtitle().isEmpty())
      builder.addText(mMapObject.getSubtitle());
    String address = mMapObject.getAddress();
    if (address.isEmpty())
      address = Framework.nativeGetAddress(mMapObject.getLat(), mMapObject.getLon());
    if (!address.isEmpty())
      builder.addText(address);
    return builder.build();
  }

  @NonNull
  private Row getPlaceRouteInfo()
  {
    final RoutingInfo routingInfo = RoutingController.get().getCachedRoutingInfo();
    if (routingInfo == null)
      throw new IllegalStateException("routingInfo == null");

    final Row.Builder builder = new Row.Builder();

    builder.setTitle(routingInfo.distToTarget.toString());

    final SpannableString time = new SpannableString(" ");
    time.setSpan(DurationSpan.create(routingInfo.totalTimeInSeconds), 0, 1, SPAN_INCLUSIVE_INCLUSIVE);
    time.setSpan(ForegroundCarColorSpan.create(CarColor.BLUE), 0, 1, SPAN_EXCLUSIVE_EXCLUSIVE);
    builder.addText(time);

    return builder.build();
  }

  private void createPaneActions(@NonNull Pane.Builder builder)
  {
    final String phoneNumber = getFirstPhoneNumber(mMapObject);
    if (!phoneNumber.isEmpty())
    {
      final Action.Builder openDialBuilder = new Action.Builder();
      openDialBuilder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_phone)).build());
      openDialBuilder.setOnClickListener(() -> getCarContext().startCarApp(new Intent(Intent.ACTION_DIAL, Uri.parse("tel:" + phoneNumber))));
      builder.addAction(openDialBuilder.build());
    }

    final Action.Builder startRouteBuilder = new Action.Builder();
    startRouteBuilder.setBackgroundColor(CarColor.GREEN);

    if (mRoutingController.isNavigating())
    {
      startRouteBuilder.setFlags(Action.FLAG_PRIMARY);
      startRouteBuilder.setTitle(getCarContext().getString(R.string.placepage_add_stop));
      startRouteBuilder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_route_via)).build());
      startRouteBuilder.setOnClickListener(() -> mRoutingController.addStop(mMapObject));
    }
    else if (mRoutingController.isBuilt())
    {
      startRouteBuilder.setFlags(Action.FLAG_DEFAULT);
      startRouteBuilder.setTitle(getCarContext().getString(R.string.p2p_start));
      startRouteBuilder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_follow_and_rotate)).build());
      startRouteBuilder.setOnClickListener(() -> {
        getScreenManager().push(new NavigationScreen(getCarContext(), getSurfaceRenderer()));
      });
    }
    else
    {
      startRouteBuilder.setFlags(Action.FLAG_DEFAULT);
      startRouteBuilder.setTitle(getCarContext().getString(R.string.p2p_to_here));
      startRouteBuilder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_route_to)).build());
      startRouteBuilder.setOnClickListener(() -> {
        mRoutingController.setRouterType(Framework.ROUTER_TYPE_VEHICLE);
        mRoutingController.prepare(LocationHelper.INSTANCE.getMyPosition(), mMapObject);
        Framework.nativeDeactivatePopup();
      });
    }

    builder.addAction(startRouteBuilder.build());
  }

  @NonNull
  private Action createBookmarkAction()
  {
    final Action.Builder builder = new Action.Builder();
    final CarIcon.Builder iconBuilder = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_bookmarks));
    if (mMapObject.getMapObjectType() == MapObject.BOOKMARK)
      iconBuilder.setTint(CarColor.YELLOW);
    builder.setIcon(iconBuilder.build());
    builder.setOnClickListener(() -> {
      // TODO(AndrewShkrob): Bookmarks not working while mRoutingController.isPlanning()
      if (MapObject.isOfType(MapObject.BOOKMARK, mMapObject))
        Framework.nativeDeleteBookmarkFromMapObject();
      else
        BookmarkManager.INSTANCE.addNewBookmark(mMapObject.getLat(), mMapObject.getLon());
    });
    return builder.build();
  }

  @NonNull
  private Action createDrivingOptionsAction()
  {
    return new Action.Builder()
        .setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_settings)).build())
        .setOnClickListener(() -> getScreenManager().pushForResult(new DrivingOptionsScreen(getCarContext(), getSurfaceRenderer()), this::onDrivingOptionsResult))
        .build();
  }

  private void onDrivingOptionsResult(@Nullable Object result)
  {
    if (result == null)
      return;

    // Driving Options changed. Let's rebuild the route
    mRoutingController.rebuildLastRoute();
  }

  @NonNull
  private static String getFirstPhoneNumber(@NonNull MapObject mapObject)
  {
    if (!mapObject.hasMetadata())
      return "";

    final String phones = mapObject.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER);
    return phones.split(";", 1)[0];
  }

  @Override
  public void onBuiltRoute()
  {
    invalidate();
  }

  @Override
  public void onPlanningStarted()
  {
    invalidate();
  }

  @Override
  public void onPlanningCancelled()
  {
    invalidate();
  }

  @Override
  public void showRoutePlan(boolean show, @Nullable Runnable completionListener)
  {
    if (completionListener != null)
      completionListener.run();
  }

  @Override
  public void onShowDisclaimer(@Nullable MapObject startPoint, @Nullable MapObject endPoint)
  {
    // TODO(AndrewShkrob): show disclaimer screen or not?
    Config.acceptRoutingDisclaimer();
    mRoutingController.prepare(startPoint, endPoint);
  }

  @Override
  public void onCommonBuildError(int lastResultCode, @NonNull String[] lastMissingMaps)
  {
    // TODO(AndrewShkrob): show download maps request
    Logger.e(TAG, "lastResultCode: " + lastResultCode + ", lastMissingMaps: " + String.join(", ", lastMissingMaps));
    CarToast.makeText(getCarContext(), R.string.unable_to_calc_alert_title, CarToast.LENGTH_LONG).show();
    mRoutingController.cancel();
    Framework.nativeShowFeature(mMapObject.getFeatureId());
  }

  @Override
  public void onDrivingOptionsBuildError()
  {
    Logger.e(TAG, "");
    CarToast.makeText(getCarContext(), R.string.unable_to_calc_alert_title, CarToast.LENGTH_LONG).show();
    mRoutingController.cancel();
    Framework.nativeShowFeature(mMapObject.getFeatureId());
  }

  @Override
  public void onCreate(@NonNull LifecycleOwner owner)
  {
    mRoutingController.attach(this);
    if (mRoutingController.isPlanning() && mRoutingController.getLastRouterType() != Framework.ROUTER_TYPE_VEHICLE)
    {
      mRoutingController.setRouterType(Framework.ROUTER_TYPE_VEHICLE);
      mRoutingController.rebuildLastRoute();
    }
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    mRoutingController.detach();
  }

  @Override
  public void onBackPressed()
  {
    if (!mRoutingController.isNavigating())
      mRoutingController.cancel();
    Framework.nativeDeactivatePopup();
  }

  /** Called when MapObject is updated but FeatureId remains unchanged.
   *  For example when MapObject is added or removed from bookmarks
   *
   *  @param newMapObject updated MapObject with same FeatureId
   */
  public void updateMapObject(@NonNull MapObject newMapObject)
  {
    if (!mMapObject.getFeatureId().equals(newMapObject.getFeatureId()))
    {
      Logger.e(TAG, newMapObject.getFeatureId() + " != " + mMapObject.getFeatureId());
      return;
    }
    mMapObject = newMapObject;
    invalidate();
  }

  /**
   * A builder of {@link PlaceScreen}.
   */
  public static final class Builder
  {
    @NonNull
    private final CarContext mCarContext;
    @NonNull
    private final SurfaceRenderer mSurfaceRenderer;
    @Nullable
    private MapObject mMapObject;

    public Builder(@NonNull final CarContext carContext, @NonNull final SurfaceRenderer surfaceRenderer)
    {
      mCarContext = carContext;
      mSurfaceRenderer = surfaceRenderer;
    }

    public Builder setMapObject(@NonNull MapObject mapObject)
    {
      mMapObject = mapObject;
      return this;
    }

    @NonNull
    public PlaceScreen build()
    {
      if (mMapObject == null)
        throw new IllegalStateException("MapObject must be set");
      return new PlaceScreen(this);
    }
  }
}
