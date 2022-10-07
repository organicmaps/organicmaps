package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.CallSuper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.editor.data.FeatureCategory;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Language;
import com.mapswithme.util.Utils;

import java.util.Arrays;

public class FeatureCategoryFragment extends BaseMwmRecyclerFragment<FeatureCategoryAdapter>
{
  private FeatureCategory mSelectedCategory;
  protected ToolbarController mToolbarController;

  public interface FeatureCategoryListener
  {
    void onFeatureCategorySelected(FeatureCategory category);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_categories, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    if (getArguments() != null && getArguments().containsKey(FeatureCategoryActivity.EXTRA_FEATURE_CATEGORY))
      mSelectedCategory = getArguments().getParcelable(FeatureCategoryActivity.EXTRA_FEATURE_CATEGORY);
    mToolbarController = new SearchToolbarController(view, requireActivity())
    {
      @Override
      protected void onTextChanged(String query)
      {
        setFilter(query);
      }
    };
  }

  private void setFilter(String query)
  {
    String locale = Language.getDefaultLocale();
    String[] creatableTypes = query.isEmpty()
                              ? Editor.nativeGetAllCreatableFeatureTypes(locale)
                              : Editor.nativeSearchCreatableFeatureTypes(query, locale);

    FeatureCategory[] categories = makeFeatureCategoriesFromTypes(creatableTypes);

    getAdapter().setCategories(categories);
  }

  @NonNull
  @Override
  protected FeatureCategoryAdapter createAdapter()
  {
    String locale = Language.getDefaultLocale();
    String[] creatableTypes = Editor.nativeGetAllCreatableFeatureTypes(locale);

    FeatureCategory[] categories = makeFeatureCategoriesFromTypes(creatableTypes);

    return new FeatureCategoryAdapter(this, categories, mSelectedCategory);
  }

  @NonNull
  private FeatureCategory[] makeFeatureCategoriesFromTypes(@NonNull String[] creatableTypes)
  {
    FeatureCategory[] categories = new FeatureCategory[creatableTypes.length];

    for (int i = 0; i < creatableTypes.length; ++i)
    {
      String localizedType = Utils.getLocalizedFeatureType(requireContext(), creatableTypes[i]);
      categories[i] = new FeatureCategory(creatableTypes[i], localizedType);
    }

    Arrays.sort(categories, (lhs, rhs) ->
      lhs.getLocalizedTypeName().compareTo(rhs.getLocalizedTypeName()));

    return categories;
  }

  public void selectCategory(FeatureCategory category)
  {
    if (requireActivity() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) requireActivity()).onFeatureCategorySelected(category);
    else if (getParentFragment() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) getParentFragment()).onFeatureCategorySelected(category);
  }
}
