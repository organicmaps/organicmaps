package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;

public class CuisineFragment extends BaseMwmRecyclerFragment
{
  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new CuisineAdapter(Editor.nativeGetSelectedCuisines(), Editor.nativeGetCuisines(), Editor.nativeGetCuisinesTranslations());
  }

  @NonNull
  public String[] getCuisines()
  {
    return ((CuisineAdapter) getAdapter()).getCuisines();
  }
}
