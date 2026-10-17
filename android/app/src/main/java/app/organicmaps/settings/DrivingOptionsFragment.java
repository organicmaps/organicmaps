package app.organicmaps.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
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
  private static final String BUNDLE_ROAD_TYPES = "road_type_ids";
  private static final String BUNDLE_ROUTE_OPTIMIZATION = "route_optimization";
  private static final String BUNDLE_PREFER_TRACKS = "prefer_saved_tracks";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();
  private boolean mOptimizationEnabledOnLoad;
  private boolean mPreferTracksOnLoad;
  private Router mRouter;
  private View mContent;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    mRouter = Router.valueOf(requireArguments().getInt(DrivingOptionsActivity.EXTRA_ROUTER));
    initViews(root);
    ViewCompat.setOnApplyWindowInsetsListener(mContent, new PaddingInsetsListener(false, true, true, true));
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                   ? makeRouteTypes(savedInstanceState)
                   : RoutingOptions.getActiveRoadTypes(mRouter);
    mOptimizationEnabledOnLoad = savedInstanceState != null ? savedInstanceState.getBoolean(BUNDLE_ROUTE_OPTIMIZATION)
                                                            : RoutingOptions.isRouteOptimizationEnabled();
    mPreferTracksOnLoad = savedInstanceState != null ? savedInstanceState.getBoolean(BUNDLE_PREFER_TRACKS)
                                                     : RoutingOptions.prefersSavedTracks(mRouter);
    return root;
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
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
      savedRoadTypes.add(each.nativeValue);
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
    outState.putBoolean(BUNDLE_ROUTE_OPTIMIZATION, mOptimizationEnabledOnLoad);
    outState.putBoolean(BUNDLE_PREFER_TRACKS, mPreferTracksOnLoad);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    // A configuration change recreates the screen and reports nothing.
    if (requireActivity().isChangingConfigurations())
      return;

    final Set<RoadType> roadTypes = RoutingOptions.getActiveRoadTypes(mRouter);
    // A temporary switch to On must not reorder stops if the user switches back before leaving this screen.
    final boolean enabled = RoutingOptions.isRouteOptimizationEnabled();
    final boolean reordered = enabled && !mOptimizationEnabledOnLoad && RoutingController.get().optimizeRoutePoints();
    final boolean preferTracks = RoutingOptions.prefersSavedTracks(mRouter);
    if ((mRouter == RoutingController.get().getLastRouterType()
         && (!mRoadTypes.equals(roadTypes) || preferTracks != mPreferTracksOnLoad))
        || reordered)
      RoutingController.get().onRoutingOptionsChanged();
    // Re-baseline so leaving this screen again (e.g. after Home) reports only what changed since.
    mRoadTypes = roadTypes;
    mOptimizationEnabledOnLoad = enabled;
    mPreferTracksOnLoad = preferTracks;
  }

  private void initViews(@NonNull View root)
  {
    mContent = root.findViewById(R.id.content);

    UiUtils.showIf(RoutingOptions.supportsSavedTracks(mRouter), root, R.id.saved_tracks_options);
    SwitchCompat tracksBtn = root.findViewById(R.id.prefer_saved_tracks_btn);
    tracksBtn.setChecked(RoutingOptions.prefersSavedTracks(mRouter));
    tracksBtn.setOnCheckedChangeListener(
        (buttonView, isChecked) -> RoutingOptions.setPreferSavedTracks(mRouter, isChecked));

    SwitchCompat optimizationBtn = root.findViewById(R.id.route_optimization_btn);
    optimizationBtn.setChecked(RoutingOptions.isRouteOptimizationEnabled());
    optimizationBtn.setOnCheckedChangeListener(
        (buttonView, isChecked) -> RoutingOptions.setRouteOptimizationEnabled(isChecked));

    UiUtils.showIf(RoutingOptions.hasSupportedOptions(mRouter), root, R.id.avoid_options, R.id.avoid_options_separator);
    initOption(root, R.id.avoid_tolls_btn, RoadType.Toll);
    initOption(root, R.id.avoid_motorways_btn, RoadType.Motorway);
    initOption(root, R.id.avoid_ferries_btn, RoadType.Ferry);
    initOption(root, R.id.avoid_dirty_roads_btn, RoadType.Dirty);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(DrivingOptionsActivity.title(mRouter));
  }

  private void initOption(@NonNull View root, int buttonId, @NonNull RoadType roadType)
  {
    SwitchCompat button = root.findViewById(buttonId);
    UiUtils.showIf(RoutingOptions.supportsOption(mRouter, roadType), (View) button.getParent());
    button.setChecked(RoutingOptions.hasOption(mRouter, roadType));
    button.setOnCheckedChangeListener((unused, isChecked) -> {
      if (isChecked)
        RoutingOptions.addOption(mRouter, roadType);
      else
        RoutingOptions.removeOption(mRouter, roadType);
    });
  }
}
