package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.CallSuper;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.editor.data.FeatureCategory;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.Language;

public class FeatureCategoryFragment extends BaseMwmRecyclerFragment
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
    mToolbarController = new SearchToolbarController(view, getActivity())
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
    ((FeatureCategoryAdapter) getAdapter()).setCategories(query.isEmpty() ? Editor.nativeGetAllFeatureCategories(Language.getKeyboardLocale())
                                                                          : Editor.nativeSearchFeatureCategories(query, Language.getKeyboardLocale()));
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new FeatureCategoryAdapter(this, Editor.nativeGetAllFeatureCategories(Language.getKeyboardLocale()), mSelectedCategory);
  }

  public void selectCategory(FeatureCategory category)
  {
    if (getActivity() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) getActivity()).onFeatureCategorySelected(category);
    else if (getParentFragment() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) getParentFragment()).onFeatureCategorySelected(category);
  }
}
