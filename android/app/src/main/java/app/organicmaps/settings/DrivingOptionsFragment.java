package app.organicmaps.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.RadioButton;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.routing.RoutingOptions.BicycleSurfacePreference;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import java.util.ArrayList;
import java.util.EnumMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  private static final String BUNDLE_ROUTER = "router_type";
  private static final String BUNDLE_ROAD_TYPES = "road_type_ids";
  private static final String BUNDLE_ROUTE_OPTIMIZATION = "route_optimization";
  private static final String BUNDLE_BICYCLE_SURFACE = "bicycle_surface";
  // Road types of every mode, as the route mode can change while this screen is open (e.g. by Android Auto).
  @NonNull
  private final Map<Router, Set<RoadType>> mRoadTypes = new EnumMap<>(Router.class);
  private boolean mOptimizationEnabledOnLoad;
  private BicycleSurfacePreference mBicycleSurfaceOnLoad;
  @NonNull
  private Router mRouter = Router.Vehicle;
  private View mContent;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    mRouter = savedInstanceState != null ? Router.valueOf(savedInstanceState.getInt(BUNDLE_ROUTER)) : initialRouter();
    initViews(root);
    ViewCompat.setOnApplyWindowInsetsListener(mContent, new PaddingInsetsListener(false, true, true, true));
    for (Router router : Router.values())
    {
      final String key = roadTypesKey(router);
      mRoadTypes.put(router, savedInstanceState != null && savedInstanceState.containsKey(key)
                                 ? makeRouteTypes(savedInstanceState, key)
                                 : RoutingOptions.getActiveRoadTypes(router));
    }
    mOptimizationEnabledOnLoad = savedInstanceState != null ? savedInstanceState.getBoolean(BUNDLE_ROUTE_OPTIMIZATION)
                                                            : RoutingOptions.isRouteOptimizationEnabled();
    // A restored bundle may lack the surface choice. Use the current preference as its baseline.
    mBicycleSurfaceOnLoad =
        savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_BICYCLE_SURFACE)
            ? BicycleSurfacePreference.fromNativeValue(savedInstanceState.getInt(BUNDLE_BICYCLE_SURFACE))
            : RoutingOptions.getBicycleSurfacePreference();
    return root;
  }

  @NonNull
  private static Router initialRouter()
  {
    final Router router = RoutingController.get().getLastRouterType();
    // Only modes with avoid options have a tab.
    return router == Router.Pedestrian || router == Router.Bicycle ? router : Router.Vehicle;
  }

  @NonNull
  private static String roadTypesKey(@NonNull Router router)
  {
    return BUNDLE_ROAD_TYPES + "_" + router.name();
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle, @NonNull String key)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(key));
    for (Integer each : items)
      result.add(RoadType.fromNativeValue(each));
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putInt(BUNDLE_ROUTER, mRouter.getType());
    for (Map.Entry<Router, Set<RoadType>> entry : mRoadTypes.entrySet())
    {
      ArrayList<Integer> savedRoadTypes = new ArrayList<>();
      for (RoadType each : entry.getValue())
        savedRoadTypes.add(each.nativeValue);
      outState.putIntegerArrayList(roadTypesKey(entry.getKey()), savedRoadTypes);
    }
    outState.putBoolean(BUNDLE_ROUTE_OPTIMIZATION, mOptimizationEnabledOnLoad);
    outState.putInt(BUNDLE_BICYCLE_SURFACE, mBicycleSurfaceOnLoad.nativeValue);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    // A configuration change recreates the screen and reports nothing.
    if (requireActivity().isChangingConfigurations())
      return;

    final RoutingController controller = RoutingController.get();
    final Router router = controller.getLastRouterType();
    final BicycleSurfacePreference bicycleSurface = RoutingOptions.getBicycleSurfacePreference();
    final boolean changed = !RoutingOptions.getActiveRoadTypes(router).equals(mRoadTypes.get(router))
                         || (router == Router.Bicycle && bicycleSurface != mBicycleSurfaceOnLoad);
    // A temporary switch to On must not reorder stops if the user switches back before leaving this screen.
    final boolean enabled = RoutingOptions.isRouteOptimizationEnabled();
    final boolean reordered = enabled && !mOptimizationEnabledOnLoad && controller.optimizeRoutePoints();
    if (changed || reordered)
      controller.onRoutingOptionsChanged();
    // Re-baseline so leaving this screen again (e.g. after Home) reports only what changed since.
    for (Router each : Router.values())
      mRoadTypes.put(each, RoutingOptions.getActiveRoadTypes(each));
    mOptimizationEnabledOnLoad = enabled;
    mBicycleSurfaceOnLoad = bicycleSurface;
  }

  private void initViews(@NonNull View root)
  {
    mContent = root.findViewById(R.id.content);

    SwitchCompat optimizationBtn = root.findViewById(R.id.route_optimization_btn);
    optimizationBtn.setChecked(RoutingOptions.isRouteOptimizationEnabled());
    optimizationBtn.setOnCheckedChangeListener(
        (buttonView, isChecked) -> RoutingOptions.setRouteOptimizationEnabled(isChecked));

    initRouterTab(root, R.id.vehicle, Router.Vehicle);
    initRouterTab(root, R.id.pedestrian, Router.Pedestrian);
    initRouterTab(root, R.id.bicycle, Router.Bicycle);
    bindOptions(root);
  }

  private void initRouterTab(@NonNull View root, @IdRes int buttonId, @NonNull Router router)
  {
    RadioButton button = root.findViewById(buttonId);
    button.setChecked(router == mRouter);
    button.setOnClickListener(v -> {
      mRouter = router;
      bindOptions(root);
    });
  }

  private void bindOptions(@NonNull View root)
  {
    bindOption(root, R.id.avoid_tolls_btn, RoadType.Toll);
    bindOption(root, R.id.avoid_motorways_btn, RoadType.Motorway);
    bindOption(root, R.id.avoid_ferries_btn, RoadType.Ferry);
    bindOption(root, R.id.avoid_dirty_roads_btn, RoadType.Dirty);
    bindOption(root, R.id.avoid_steps_btn, RoadType.Steps);
    bindOption(root, R.id.avoid_difficult_trails_btn, RoadType.DifficultTrail);
    bindBicycleSurfaceOptions(root);
  }

  private void bindBicycleSurfaceOptions(@NonNull View root)
  {
    final boolean cycling = mRouter == Router.Bicycle;
    UiUtils.showIf(cycling, root.findViewById(R.id.bicycle_surface_options));
    if (!cycling)
      return;

    final BicycleSurfacePreference selected = RoutingOptions.getBicycleSurfacePreference();
    bindSurfaceOption(root, R.id.bicycle_surface_any, BicycleSurfacePreference.Any, selected);
    bindSurfaceOption(root, R.id.bicycle_surface_paved, BicycleSurfacePreference.PreferPaved, selected);
    bindSurfaceOption(root, R.id.bicycle_surface_unpaved, BicycleSurfacePreference.PreferUnpaved, selected);
    bindSurfaceOption(root, R.id.bicycle_surface_avoid_unpaved, BicycleSurfacePreference.AvoidUnpaved, selected);
  }

  private void bindSurfaceOption(@NonNull View root, @IdRes int buttonId, BicycleSurfacePreference option,
                                 BicycleSurfacePreference selected)
  {
    RadioButton button = root.findViewById(buttonId);
    button.setChecked(option == selected);
    button.setOnClickListener(v -> RoutingOptions.setBicycleSurfacePreference(option));
  }

  private void bindOption(@NonNull View root, @IdRes int buttonId, @NonNull RoadType roadType)
  {
    final Router router = mRouter;
    SwitchCompat button = root.findViewById(buttonId);
    UiUtils.showIf(
        (router != Router.Bicycle || roadType != RoadType.Dirty) && RoutingOptions.supportsOption(router, roadType),
        button);
    button.setOnCheckedChangeListener(null);
    button.setChecked(RoutingOptions.hasOption(router, roadType));
    // Another mode's settings are shown as they are, without animating the switches.
    button.jumpDrawablesToCurrentState();
    button.setOnCheckedChangeListener((unused, isChecked) -> {
      if (isChecked)
        RoutingOptions.addOption(router, roadType);
      else
        RoutingOptions.removeOption(router, roadType);
    });
  }
}
