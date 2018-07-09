package com.mapswithme.maps.editor;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;

public class CuisineFragment extends BaseMwmRecyclerFragment<CuisineAdapter>
{
  private CuisineAdapter mAdapter;

  @NonNull
  @Override
  protected CuisineAdapter createAdapter()
  {
    mAdapter = new CuisineAdapter();
    return mAdapter;
  }

  @NonNull
  public String[] getCuisines()
  {
    return mAdapter.getCuisines();
  }

  public void setFilter(String filter)
  {
    mAdapter.setFilter(filter);
  }
}
