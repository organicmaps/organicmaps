package com.mapswithme.maps.base;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.widget.ToolbarController;

public class BaseMwmToolbarFragment extends BaseMwmFragment
{
  protected ToolbarController mToolbarController;

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mToolbarController = onCreateToolbarController(view);
  }

  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, getActivity());
  }
}
