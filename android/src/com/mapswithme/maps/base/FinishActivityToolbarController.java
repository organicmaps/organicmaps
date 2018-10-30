package com.mapswithme.maps.base;

import android.app.Activity;
import android.support.annotation.NonNull;
import android.view.View;

import com.mapswithme.maps.widget.ToolbarController;

public class FinishActivityToolbarController extends ToolbarController
{
  public FinishActivityToolbarController(@NonNull View root, @NonNull Activity activity)
  {
    super(root, activity);
  }

  @Override
  public void onUpClick()
  {
    getActivity().finish();
  }
}
