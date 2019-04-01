package com.mapswithme.maps.settings;

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
import com.mapswithme.maps.bookmarks.data.RoutingOptions;

public class DrivingOptionsFragment extends BaseMwmToolbarFragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_driving_options, container, false);
    initViews(root);
    setHasOptionsMenu(hasBundleOptionsMenu());
    return root;
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
    inflater.inflate(R.menu.menu_tags_done, menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.done)
    {
      requireActivity().finish();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  private void initViews(@NonNull View root)
  {
    Switch avoidTollsBtn = root.findViewById(R.id.avoid_tolls_btn);
    CompoundButton.OnCheckedChangeListener tollBtnListener =
        new ToggleRoutingOptionListener(RoadType.TOLL);
    avoidTollsBtn.setOnCheckedChangeListener(tollBtnListener);

    Switch avoidMotorwaysBtn = root.findViewById(R.id.avoid_motorways_btn);
    CompoundButton.OnCheckedChangeListener motorwayBtnListener =
        new ToggleRoutingOptionListener(RoadType.MOTORWAY);
    avoidMotorwaysBtn.setOnCheckedChangeListener(motorwayBtnListener);

    Switch avoidFerriesBtn = root.findViewById(R.id.avoid_ferries_btn);
    CompoundButton.OnCheckedChangeListener ferryBtnListener =
        new ToggleRoutingOptionListener(RoadType.FERRY);
    avoidFerriesBtn.setOnCheckedChangeListener(ferryBtnListener);

    Switch avoidDirtyRoadsBtn = root.findViewById(R.id.avoid_dirty_roads_btn);
    CompoundButton.OnCheckedChangeListener dirtyBtnListener =
        new ToggleRoutingOptionListener(RoadType.DIRTY);
    avoidDirtyRoadsBtn.setOnCheckedChangeListener(dirtyBtnListener);
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
        RoutingOptions.nativeAddOption(mRoadType.ordinal());
      else
        RoutingOptions.nativeRemoveOption(mRoadType.ordinal());
    }
  }
}
