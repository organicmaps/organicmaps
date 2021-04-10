package com.mapswithme.maps.base;

import android.app.Activity;
import android.view.View;

import androidx.annotation.NonNull;

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
    requireActivity().finish();
  }
}
