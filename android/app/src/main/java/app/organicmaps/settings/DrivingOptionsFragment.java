package app.organicmaps.settings;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.SwitchCompat;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.RoutingOptions;
import app.organicmaps.sdk.settings.RoadType;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  private static final String BUNDLE_ROAD_TYPES = "road_types_mask";
  private static final String BUNDLE_ROUTE_OPTIMIZATION = "route_optimization";
  private int mOptionsMask;
  private boolean mOptimizationEnabledOnLoad;
  private View mContent;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    ViewCompat.setOnApplyWindowInsetsListener(mContent, new PaddingInsetsListener(false, true, true, true));
    mOptionsMask = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                     ? savedInstanceState.getInt(BUNDLE_ROAD_TYPES)
                     : RoutingOptions.getOptions();
    mOptimizationEnabledOnLoad = savedInstanceState != null ? savedInstanceState.getBoolean(BUNDLE_ROUTE_OPTIMIZATION)
                                                            : RoutingOptions.isRouteOptimizationEnabled();
    return root;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    outState.putInt(BUNDLE_ROAD_TYPES, mOptionsMask);
    outState.putBoolean(BUNDLE_ROUTE_OPTIMIZATION, mOptimizationEnabledOnLoad);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    // A configuration change recreates the screen and reports nothing.
    if (requireActivity().isChangingConfigurations())
      return;

    final int mask = RoutingOptions.getOptions();
    // A temporary switch to On must not reorder stops if the user switches back before leaving this screen.
    final boolean enabled = RoutingOptions.isRouteOptimizationEnabled();
    final boolean reordered = enabled && !mOptimizationEnabledOnLoad && RoutingController.get().optimizeRoutePoints();
    if (mOptionsMask != mask || reordered)
      RoutingController.get().onRoutingOptionsChanged();
    // Re-baseline so leaving this screen again (e.g. after Home) reports only what changed since.
    mOptionsMask = mask;
    mOptimizationEnabledOnLoad = enabled;
  }

  private void initViews(@NonNull View root)
  {
    mContent = root.findViewById(R.id.content);

    SwitchCompat optimizationBtn = root.findViewById(R.id.route_optimization_btn);
    optimizationBtn.setChecked(RoutingOptions.isRouteOptimizationEnabled());
    optimizationBtn.setOnCheckedChangeListener(
        (buttonView, isChecked) -> RoutingOptions.setRouteOptimizationEnabled(isChecked));

    SwitchCompat tollsBtn = root.findViewById(R.id.avoid_tolls_btn);
    tollsBtn.setChecked(RoutingOptions.hasOption(RoadType.Toll));
    CompoundButton.OnCheckedChangeListener tollBtnListener = new ToggleRoutingOptionListener(RoadType.Toll);
    tollsBtn.setOnCheckedChangeListener(tollBtnListener);

    SwitchCompat motorwaysBtn = root.findViewById(R.id.avoid_motorways_btn);
    motorwaysBtn.setChecked(RoutingOptions.hasOption(RoadType.Motorway));
    CompoundButton.OnCheckedChangeListener motorwayBtnListener = new ToggleRoutingOptionListener(RoadType.Motorway);
    motorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    SwitchCompat ferriesBtn = root.findViewById(R.id.avoid_ferries_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.Ferry));
    CompoundButton.OnCheckedChangeListener ferryBtnListener = new ToggleRoutingOptionListener(RoadType.Ferry);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    SwitchCompat dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.Dirty));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener = new ToggleRoutingOptionListener(RoadType.Dirty);
    dirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);
  }

  private static class ToggleRoutingOptionListener implements CompoundButton.OnCheckedChangeListener
  {
    @NonNull
    private final RoadType mRoadType;

    private ToggleRoutingOptionListener(@NonNull RoadType roadType)
    {
      mRoadType = roadType;
    }

    @Override
    public void onCheckedChanged(CompoundButton buttonView, boolean isChecked)
    {
      if (isChecked)
        RoutingOptions.addOption(mRoadType);
      else
        RoutingOptions.removeOption(mRoadType);
    }
  }
}
