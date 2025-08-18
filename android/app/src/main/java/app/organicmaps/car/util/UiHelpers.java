package app.organicmaps.car.util;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.OnScreenResultListener;
import androidx.car.app.Screen;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarColor;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Row;
import androidx.car.app.navigation.model.MapController;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.settings.SettingsScreen;
import app.organicmaps.sdk.Map;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.editor.OpeningHours;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.location.LocationState;
import app.organicmaps.sdk.util.LocationUtils;
import app.organicmaps.util.Utils;
import java.util.Calendar;

public final class UiHelpers
{
  @NonNull
  public static ActionStrip createSettingsActionStrip(@NonNull BaseMapScreen mapScreen,
                                                      @NonNull SurfaceRenderer surfaceRenderer)
  {
    return new ActionStrip.Builder().addAction(createSettingsAction(mapScreen, surfaceRenderer)).build();
  }

  @NonNull
  public static ActionStrip createMapActionStrip(@NonNull CarContext context, @NonNull SurfaceRenderer surfaceRenderer)
  {
    final CarIcon iconPlus = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_plus)).build();
    final CarIcon iconMinus = new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_minus)).build();

    final Action panAction = new Action.Builder(Action.PAN).build();
    final Action location = createLocationButton(context);
    final Action zoomIn = new Action.Builder().setIcon(iconPlus).setOnClickListener(surfaceRenderer::onZoomIn).build();
    final Action zoomOut =
        new Action.Builder().setIcon(iconMinus).setOnClickListener(surfaceRenderer::onZoomOut).build();
    return new ActionStrip.Builder()
        .addAction(panAction)
        .addAction(zoomIn)
        .addAction(zoomOut)
        .addAction(location)
        .build();
  }

  @NonNull
  public static MapController createMapController(@NonNull CarContext context, @NonNull SurfaceRenderer surfaceRenderer)
  {
    return new MapController.Builder().setMapActionStrip(createMapActionStrip(context, surfaceRenderer)).build();
  }

  @NonNull
  public static Action createSettingsAction(@NonNull BaseMapScreen mapScreen, @NonNull SurfaceRenderer surfaceRenderer)
  {
    return createSettingsAction(mapScreen, surfaceRenderer, null);
  }

  @NonNull
  public static Action createSettingsActionForResult(@NonNull BaseMapScreen mapScreen,
                                                     @NonNull SurfaceRenderer surfaceRenderer,
                                                     @NonNull OnScreenResultListener onScreenResultListener)
  {
    return createSettingsAction(mapScreen, surfaceRenderer, onScreenResultListener);
  }

  @NonNull
  private static Action createSettingsAction(@NonNull BaseMapScreen mapScreen, @NonNull SurfaceRenderer surfaceRenderer,
                                             @Nullable OnScreenResultListener onScreenResultListener)
  {
    final CarContext context = mapScreen.getCarContext();
    final CarIcon iconSettings =
        new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_settings)).build();

    return new Action.Builder()
        .setIcon(iconSettings)
        .setOnClickListener(() -> {
          // Action.onClickListener for the Screen A maybe called even if the Screen B is shown now.
          // We need to check it
          // This may happen when we use PopToRootHack:
          //   * ScreenManager.popToRoot()
          //   * The root screen (A) is shown for a while
          //   * User clicks on some action
          //   * ScreenManager.push(new Screen())
          //   * New screen (B) is displayed now
          //   * Action.onClickListener is called for action from root screen (A)
          if (mapScreen.getScreenManager().getTop() != mapScreen)
            return;
          final Screen settingsScreen = new SettingsScreen(context, surfaceRenderer);
          if (onScreenResultListener != null)
            mapScreen.getScreenManager().pushForResult(settingsScreen, onScreenResultListener);
          else
            mapScreen.getScreenManager().push(settingsScreen);
        })
        .build();
  }

  @Nullable
  public static Row getPlaceOpeningHoursRow(@NonNull MapObject place, @NonNull CarContext context)
  {
    final String ohStr = place.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
    final boolean isEmptyTT = (timetables == null || timetables.length == 0);

    if (ohStr.isEmpty() && isEmptyTT)
      return null;

    final Row.Builder builder = new Row.Builder();
    builder.setImage(
        new CarIcon.Builder(IconCompat.createWithResource(context, R.drawable.ic_operating_hours)).build());

    if (isEmptyTT)
      builder.setTitle(ohStr);
    else if (timetables[0].isFullWeek())
    {
      if (timetables[0].isFullday)
        builder.setTitle(context.getString(R.string.twentyfour_seven));
      else
        builder.setTitle(timetables[0].workingTimespan.toWideString());
    }
    else
    {
      boolean containsCurrentWeekday = false;
      final int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
      for (final Timetable tt : timetables)
      {
        if (tt.containsWeekday(currentDay))
        {
          containsCurrentWeekday = true;
          String openTime;

          if (tt.isFullday)
            openTime = Utils.unCapitalize(context.getString(R.string.editor_time_allday));
          else
            openTime = tt.workingTimespan.toWideString();

          builder.setTitle(openTime);

          break;
        }
      }
      // Show that place is closed today.
      if (!containsCurrentWeekday)
        builder.setTitle(context.getString(R.string.day_off_today));
    }

    return builder.build();
  }

  @NonNull
  private static Action createLocationButton(@NonNull CarContext context)
  {
    final Action.Builder builder = new Action.Builder();
    final int locationMode = Map.isEngineCreated() ? LocationState.getMode() : LocationState.NOT_FOLLOW_NO_POSITION;
    CarColor tintColor = Colors.DEFAULT;

    @DrawableRes
    int drawableRes;
    switch (locationMode)
    {
    case LocationState.PENDING_POSITION, LocationState.NOT_FOLLOW_NO_POSITION ->
      drawableRes = R.drawable.ic_location_off;
    case LocationState.NOT_FOLLOW -> drawableRes = R.drawable.ic_not_follow;
    case LocationState.FOLLOW ->
    {
      drawableRes = R.drawable.ic_follow;
      tintColor = Colors.LOCATION_TINT;
    }
    case LocationState.FOLLOW_AND_ROTATE ->
    {
      drawableRes = R.drawable.ic_follow_and_rotate;
      tintColor = Colors.LOCATION_TINT;
    }
    default -> throw new IllegalArgumentException("Invalid button mode: " + locationMode);
    }

    final CarIcon icon =
        new CarIcon.Builder(IconCompat.createWithResource(context, drawableRes)).setTint(tintColor).build();
    builder.setIcon(icon);
    builder.setOnClickListener(() -> {
      LocationState.nativeSwitchToNextMode();
      final LocationHelper locationHelper = MwmApplication.from(context).getLocationHelper();
      if (!locationHelper.isActive() && LocationUtils.checkFineLocationPermission(context))
        locationHelper.start();
    });
    return builder.build();
  }
}
