package com.mapswithme.maps.settings;

import android.app.Activity;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.CompoundButton;
import android.widget.Switch;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.routing.RoutingOptions;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{

  public static final String BUNDLE_ROAD_TYPES = "road_types";
  @NonNull
  private Set<RoadType> mRoadTypes = Collections.emptySet();

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    setHasOptionsMenu(hasBundleOptionsMenu());
    mRoadTypes = savedInstanceState != null && savedInstanceState.containsKey(BUNDLE_ROAD_TYPES)
                 ? makeRouteTypes(savedInstanceState)
                 : RoutingOptions.getActiveRoadTypes();
    return root;
  }

  @NonNull
  private Set<RoadType> makeRouteTypes(@NonNull Bundle bundle)
  {
    Set<RoadType> result = new HashSet<>();
    List<Integer> items = Objects.requireNonNull(bundle.getIntegerArrayList(BUNDLE_ROAD_TYPES));
    for (Integer each : items)
    {
      result.add(RoadType.values()[each]);
    }
    return result;
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    ArrayList<Integer> savedRoadTypes = new ArrayList<>();
    for (RoadType each : mRoadTypes)
    {
      savedRoadTypes.add(each.ordinal());
    }
    outState.putIntegerArrayList(BUNDLE_ROAD_TYPES, savedRoadTypes);
  }

  private boolean isSameSettingsBeforeAndAfter()
  {
    Set<RoadType> lastActiveRoadTypes = RoutingOptions.getActiveRoadTypes();
    return mRoadTypes.containsAll(lastActiveRoadTypes) && lastActiveRoadTypes.containsAll(mRoadTypes);
  }

  private boolean hasBundleOptionsMenu()
  {
    Bundle arguments = getArguments();
    if (arguments == null)
      return false;

    return arguments.getBoolean(DrivingOptionsActivity.BUNDLE_REQUIRE_OPTIONS_MENU, false);
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.menu_done, menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.done)
    {
      requireActivity().setResult(isSameSettingsBeforeAndAfter() ? Activity.RESULT_CANCELED
                                                                 : Activity.RESULT_OK);
      requireActivity().finish();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  private void initViews(@NonNull View root)
  {
    Switch tollsBtn = root.findViewById(R.id.avoid_tolls_btn);
    tollsBtn.setChecked(RoutingOptions.hasOption(RoadType.TOLL));
    CompoundButton.OnCheckedChangeListener tollBtnListener =
        new ToggleRoutingOptionListener(RoadType.TOLL);
    tollsBtn.setOnCheckedChangeListener(tollBtnListener);

    Switch motorwaysBtn = root.findViewById(R.id.avoid_motorways_btn);
    motorwaysBtn.setChecked(RoutingOptions.hasOption(RoadType.MOTORWAY));
    CompoundButton.OnCheckedChangeListener motorwayBtnListener =
        new ToggleRoutingOptionListener(RoadType.MOTORWAY);
    motorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    Switch ferriesBtn = root.findViewById(R.id.avoid_ferries_btn);
    ferriesBtn.setChecked(RoutingOptions.hasOption(RoadType.FERRY));
    CompoundButton.OnCheckedChangeListener ferryBtnListener =
        new ToggleRoutingOptionListener(RoadType.FERRY);
    ferriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    Switch dirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_btn);
    dirtyRoadsBtn.setChecked(RoutingOptions.hasOption(RoadType.DIRTY));
    CompoundButton.OnCheckedChangeListener dirtyBtnListener =
        new ToggleRoutingOptionListener(RoadType.DIRTY);
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
