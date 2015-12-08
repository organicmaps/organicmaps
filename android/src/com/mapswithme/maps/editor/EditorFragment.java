package com.mapswithme.maps.editor;

import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.base.OnBackPressListener;

public class EditorFragment extends BaseMwmFragment
                         implements OnBackPressListener
{
  public static final String EXTRA_POINT = "point";

  @Override
  public boolean onBackPressed()
  {
    return false;
  }
}
