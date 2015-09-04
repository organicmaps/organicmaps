package com.mapswithme.maps.search;

import android.os.Bundle;
import android.view.View;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;

public class SearchCategoriesFragment extends BaseMwmRecyclerFragment implements CategoriesAdapter.OnCategorySelectedListener
{
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getRecyclerView().setAdapter(new CategoriesAdapter(this));
  }

  @Override
  public void onCategorySelected(String category)
  {
    if (!passCategory(getParentFragment(), category))
      passCategory(getActivity(), category);
  }

  private boolean passCategory(Object listener, String category)
  {
    if (!(listener instanceof CategoriesAdapter.OnCategorySelectedListener))
      return false;

    ((CategoriesAdapter.OnCategorySelectedListener)listener).onCategorySelected(category);
    return true;
  }
}
