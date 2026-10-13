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
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  private static final String BUNDLE_ROUTER = "router_type";
  private static final String BUNDLE_ROAD_TYPES = "road_type_ids";
  private static final String BUNDLE_ROUTE_OPTIMIZATION = "route_optimization";
  // Road types of the router the route is planned with, whichever tab is shown.
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();
  private boolean mOptimizationEnabledOnLoad;
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
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                   ? makeRouteTypes(savedInstanceState)
                   : RoutingOptions.getActiveRoadTypes(RoutingController.get().getLastRouterType());
    mOptimizationEnabledOnLoad = savedInstanceState != null ? savedInstanceState.getBoolean(BUNDLE_ROUTE_OPTIMIZATION)
                                                            : RoutingOptions.isRouteOptimizationEnabled();
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
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
      result.add(RoadType.fromNativeValue(each));
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putInt(BUNDLE_ROUTER, mRouter.getType());
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
      savedRoadTypes.add(each.nativeValue);
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
    outState.putBoolean(BUNDLE_ROUTE_OPTIMIZATION, mOptimizationEnabledOnLoad);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    // A configuration change recreates the screen and reports nothing.
    if (requireActivity().isChangingConfigurations())
      return;

    final Set<RoadType> roadTypes = RoutingOptions.getActiveRoadTypes(RoutingController.get().getLastRouterType());
    // A temporary switch to On must not reorder stops if the user switches back before leaving this screen.
    final boolean enabled = RoutingOptions.isRouteOptimizationEnabled();
    final boolean reordered = enabled && !mOptimizationEnabledOnLoad && RoutingController.get().optimizeRoutePoints();
    if (!mRoadTypes.equals(roadTypes) || reordered)
      RoutingController.get().onRoutingOptionsChanged();
    // Re-baseline so leaving this screen again (e.g. after Home) reports only what changed since.
    mRoadTypes = roadTypes;
    mOptimizationEnabledOnLoad = enabled;
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
  }

  private void bindOption(@NonNull View root, @IdRes int buttonId, @NonNull RoadType roadType)
  {
    final Router router = mRouter;
    SwitchCompat button = root.findViewById(buttonId);
    UiUtils.showIf(RoutingOptions.supportsOption(router, roadType), (View) button.getParent());
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
