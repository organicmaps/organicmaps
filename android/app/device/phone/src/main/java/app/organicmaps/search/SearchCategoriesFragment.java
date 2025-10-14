package app.organicmaps.search;

import android.os.Bundle;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmRecyclerFragment;

public class SearchCategoriesFragment
    extends BaseMwmRecyclerFragment<CategoriesAdapter> implements CategoriesAdapter.CategoriesUiListener
{
  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getAdapter().updateCategories(this);

    ((SearchFragment) getParentFragment()).setRecyclerScrollListener(getRecyclerView());
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

  @Override
  public void onSearchCategorySelected(String category)
  {
    if (!passCategory(getParentFragment(), category))
      passCategory(requireActivity(), category);
  }

  private static boolean passCategory(Object listener, String category)
  {
    if (!(listener instanceof CategoriesAdapter.CategoriesUiListener))
      return false;

    ((CategoriesAdapter.CategoriesUiListener) listener).onSearchCategorySelected(category);
    return true;
  }
}
