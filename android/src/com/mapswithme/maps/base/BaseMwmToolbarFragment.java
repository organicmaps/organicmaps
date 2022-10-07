package com.mapswithme.maps.base;

import android.os.Bundle;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.widget.ToolbarController;

public class BaseMwmToolbarFragment extends BaseAsyncOperationFragment
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private ToolbarController mToolbarController;

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mToolbarController = onCreateToolbarController(view);
  }

  @NonNull
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new ToolbarController(root, requireActivity());
  }

  @NonNull
  protected ToolbarController getToolbarController()
  {
    return mToolbarController;
  }
}
