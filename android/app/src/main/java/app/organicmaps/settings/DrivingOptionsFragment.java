package app.organicmaps.settings;

import android.app.Activity;
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
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  private static final String BUNDLE_ROAD_TYPES = "road_types";
  private static final String BUNDLE_ROUTE_OPTIMIZATION = "route_optimization";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();
  private boolean mRouteOrderChanged;
  private View mContent;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    ViewCompat.setOnApplyWindowInsetsListener(mContent, new PaddingInsetsListener(false, true, true, true));
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                   ? makeRouteTypes(savedInstanceState)
                   : RoutingOptions.getActiveRoadTypes();
    mRouteOrderChanged = savedInstanceState != null && savedInstanceState.getBoolean(BUNDLE_ROUTE_OPTIMIZATION);
    return root;
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
      result.add(RoadType.values()[each]);
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
      savedRoadTypes.add(each.ordinal());
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
    outState.putBoolean(BUNDLE_ROUTE_OPTIMIZATION, mRouteOrderChanged);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    // Reported here rather than in onDestroy() so a process death after this screen is left still applies the
    // change. A configuration change recreates the screen and reports nothing.
    final Activity activity = getActivity();
    if (activity == null || activity.isChangingConfigurations())
      return;

    final Set<RoadType> roadTypes = RoutingOptions.getActiveRoadTypes();
    RoutingController.get().onRoutingOptionsChanged(!mRoadTypes.equals(roadTypes) || mRouteOrderChanged);
    // Re-baseline so returning to this screen and leaving again reports only what changed since.
    mRoadTypes = roadTypes;
    mRouteOrderChanged = false;
  }

  private void initViews(@NonNull View root)
  {
    mContent = root.findViewById(R.id.content);

    SwitchCompat optimizationBtn = root.findViewById(R.id.route_optimization_btn);
    optimizationBtn.setChecked(RoutingOptions.isRouteOptimizationEnabled());
    optimizationBtn.setOnCheckedChangeListener(
        (buttonView, isChecked) -> mRouteOrderChanged |= RoutingOptions.setRouteOptimizationEnabled(isChecked));

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
