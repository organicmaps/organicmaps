package com.mapswithme.maps.search;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.purchase.AdsRemovalActivationCallback;
import com.mapswithme.maps.purchase.AdsRemovalPurchaseDialog;

public class SearchCategoriesFragment extends BaseMwmRecyclerFragment<CategoriesAdapter>
    implements CategoriesAdapter.CategoriesUiListener, AdsRemovalActivationCallback
{
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getAdapter().updateCategories(this);
  }

  @NonNull
  @Override
  protected CategoriesAdapter createAdapter()
  {
    return new CategoriesAdapter(this);
  }

  @Override
  protected int getLayoutRes()
  {
    return R.layout.fragment_search_categories;
  }


  protected void safeOnActivityCreated(@Nullable Bundle savedInstanceState)
  {
    ((SearchFragment) getParentFragment()).setRecyclerScrollListener(getRecyclerView());
  }

  @Override
  public void onSearchCategorySelected(String category)
  {
    if (!passCategory(getParentFragment(), category))
      passCategory(getActivity(), category);
  }

  @Override
  public void onPromoCategorySelected(@NonNull PromoCategory promo)
  {
    PromoCategoryProcessor processor = promo.createProcessor(getActivity());
    processor.process();
  }

  @Override
  public void onAdsRemovalSelected()
  {
    AdsRemovalPurchaseDialog.show(this);
  }

  private static boolean passCategory(Object listener, String category)
  {
    if (!(listener instanceof CategoriesAdapter.CategoriesUiListener))
      return false;

    ((CategoriesAdapter.CategoriesUiListener)listener).onSearchCategorySelected(category);
    return true;
  }

  @Override
  public void onAdsRemovalActivation()
  {
    getAdapter().updateCategories(this);
    getAdapter().notifyDataSetChanged();
  }
}
