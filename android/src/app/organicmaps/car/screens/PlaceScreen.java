package app.organicmaps.car.screens;

import static java.util.Objects.requireNonNull;

import android.content.Intent;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.Pane;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;
import androidx.core.graphics.drawable.IconCompat;

import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Metadata;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.OnBackPressedCallback;
import app.organicmaps.util.log.Logger;

public class PlaceScreen extends BaseMapScreen implements OnBackPressedCallback.Callback
{
  private static final String TAG = PlaceScreen.class.getSimpleName();

  @NonNull
  private MapObject mMapObject;

  @NonNull
  private final OnBackPressedCallback mOnBackPressedCallback;

  private PlaceScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext, builder.mSurfaceRenderer);
    mMapObject = requireNonNull(builder.mMapObject);
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
    builder.addEndHeaderAction(createBookmarkAction());

    return builder.build();
  }

  @NonNull
  private Pane createPane()
  {
    final Pane.Builder builder = new Pane.Builder();

    builder.addRow(getPlaceDescription());

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
    startRouteBuilder.setFlags(Action.FLAG_PRIMARY);
    startRouteBuilder.setTitle(getCarContext().getString(R.string.p2p_to_here));
    startRouteBuilder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_route_to)).build());
    startRouteBuilder.setOnClickListener(() -> {
      /* TODO (AndrewShkrob): Will be implemented with route planning */
    });
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
      if (MapObject.isOfType(MapObject.BOOKMARK, mMapObject))
        Framework.nativeDeleteBookmarkFromMapObject();
      else
        BookmarkManager.INSTANCE.addNewBookmark(mMapObject.getLat(), mMapObject.getLon());
    });
    return builder.build();
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
  public void onBackPressed()
  {
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
