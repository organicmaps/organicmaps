package app.organicmaps.car.screens;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.CarToast;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.NavigationManager;
import androidx.car.app.navigation.model.NavigationTemplate;
import androidx.car.app.navigation.model.Step;
import androidx.car.app.navigation.model.TravelEstimate;
import androidx.car.app.navigation.model.Trip;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.settings.DrivingOptionsScreen;
import app.organicmaps.car.util.Colors;
import app.organicmaps.car.util.RoutingUtils;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.sound.TtsPlayer;
import app.organicmaps.util.ThemeSwitcher;
import app.organicmaps.util.log.Logger;

import java.util.List;
import java.util.Objects;

public class NavigationScreen extends BaseMapScreen implements RoutingController.Container
{
  private static final String TAG = NavigationScreen.class.getSimpleName();

  @NonNull
  private final NavigationManager mNavigationManager;
  @NonNull
  private final RoutingController mRoutingController;

  public NavigationScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
    mNavigationManager = carContext.getCarService(NavigationManager.class);
    mRoutingController = RoutingController.get();
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final NavigationTemplate.Builder builder = new NavigationTemplate.Builder();
    builder.setBackgroundColor(ThemeUtils.isNightMode(getCarContext()) ? Colors.NAVIGATION_TEMPLATE_BACKGROUND_NIGHT : Colors.NAVIGATION_TEMPLATE_BACKGROUND_DAY);
    builder.setActionStrip(createActionStrip());
    builder.setMapActionStrip(UiHelpers.createMapActionStrip(getCarContext(), getSurfaceRenderer()));

    final TravelEstimate destinationTravelEstimate = getDestinationTravelEstimate();
    if (destinationTravelEstimate != null)
      builder.setDestinationTravelEstimate(getDestinationTravelEstimate());

    builder.setNavigationInfo(getNavigationInfo());
    return builder.build();
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    mRoutingController.attach(this);
    mRoutingController.restore();
    if (mRoutingController.isPlanning())
      mRoutingController.start();
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    mRoutingController.detach();
  }

  @Override
  public void onNavigationStarted()
  {
    ThemeSwitcher.INSTANCE.restart(true);
    mNavigationManager.navigationStarted();
  }

  @Override
  public void onNavigationCancelled()
  {
    ThemeSwitcher.INSTANCE.restart(true);
    mNavigationManager.navigationEnded();
    RoutingUtils.resetTrip();
    // TODO (AndrewShkrob): Handle "Stop" click and "Navigation finished" actions separately. Add localized string.
    CarToast.makeText(getCarContext(), "Navigation finished", CarToast.LENGTH_LONG).show();
    getScreenManager().popToRoot();
  }

  public void stopNavigation()
  {
    mRoutingController.cancel();
  }

  @NonNull
  private ActionStrip createActionStrip()
  {
    final Action.Builder stopActionBuilder = new Action.Builder();
    stopActionBuilder.setTitle(getCarContext().getString(R.string.current_location_unknown_stop_button));
    stopActionBuilder.setOnClickListener(this::stopNavigation);

    final ActionStrip.Builder builder = new ActionStrip.Builder();
    builder.addAction(createTtsAction());
    builder.addAction(UiHelpers.createSettingsActionForResult(this, getSurfaceRenderer(), this::onSettingsResult));
    builder.addAction(stopActionBuilder.build());
    return builder.build();
  }

  @Nullable
  private TravelEstimate getDestinationTravelEstimate()
  {
    final Trip trip = RoutingUtils.getLastTrip();

    if (trip.isLoading())
      return null;

    List<TravelEstimate> travelEstimates = trip.getDestinationTravelEstimates();
    if (travelEstimates.size() != 1)
      throw new RuntimeException("TravelEstimates size must be 1");

    return travelEstimates.get(0);
  }

  @NonNull
  private NavigationTemplate.NavigationInfo getNavigationInfo()
  {
    final androidx.car.app.navigation.model.RoutingInfo.Builder builder = new androidx.car.app.navigation.model.RoutingInfo.Builder();
    final Trip trip = RoutingUtils.getLastTrip();

    if (trip.isLoading())
    {
      builder.setLoading(true);
      return builder.build();
    }

    final List<Step> steps = trip.getSteps();
    final List<TravelEstimate> stepsEstimates = trip.getStepTravelEstimates();
    if (steps.isEmpty())
      throw new RuntimeException("Steps size must be at least 1");

    builder.setCurrentStep(steps.get(0), Objects.requireNonNull(stepsEstimates.get(0).getRemainingDistance()));
    if (steps.size() > 1)
      builder.setNextStep(steps.get(1));

    return builder.build();
  }

  private void onSettingsResult(@Nullable Object result)
  {
    if (result == null || result != DrivingOptionsScreen.DRIVING_OPTIONS_RESULT_CHANGED)
      return;

    // TODO (AndrewShkrob): Need to rebuild the route with updated driving options
    Logger.d(TAG, "Driving options changed");
  }

  @NonNull
  private Action createTtsAction()
  {
    final Action.Builder ttsActionBuilder = new Action.Builder();
    @DrawableRes final int imgRes = TtsPlayer.isEnabled() ? R.drawable.ic_voice_on : R.drawable.ic_voice_off;
    ttsActionBuilder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), imgRes)).build());
    ttsActionBuilder.setOnClickListener(() -> {
      TtsPlayer.setEnabled(!TtsPlayer.isEnabled());
      invalidate();
    });

    return ttsActionBuilder.build();
  }
}
