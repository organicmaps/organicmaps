package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;

public class CuisineFragment extends BaseMwmRecyclerFragment
{
  public static final String EXTRA_CURRENT_CUISINE = "Cuisine";

  private String mCurrentCuisine;

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    mCurrentCuisine = getArguments().getString(EXTRA_CURRENT_CUISINE, "");
    return super.onCreateView(inflater, container, savedInstanceState);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new CuisineAdapter(mCurrentCuisine);
  }

  @NonNull
  public String getCuisine()
  {
    return ((CuisineAdapter) getAdapter()).getCuisine();
  }
}
