package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.editor.data.FeatureCategory;

public class FeatureCategoryFragment extends BaseMwmRecyclerFragment
{
  private FeatureCategory mSelectedCategory;

  public interface FeatureCategoryListener
  {
    void onFeatureCategorySelected(FeatureCategory category);
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    if (getArguments() != null && getArguments().containsKey(FeatureCategoryActivity.EXTRA_FEATURE_CATEGORY))
      mSelectedCategory = getArguments().getParcelable(FeatureCategoryActivity.EXTRA_FEATURE_CATEGORY);
    return super.onCreateView(inflater, container, savedInstanceState);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    return new FeatureCategoryAdapter(this, Editor.nativeGetNewFeatureCategories(), mSelectedCategory);
  }

  public void selectCategory(FeatureCategory category)
  {
    if (getActivity() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) getActivity()).onFeatureCategorySelected(category);
    else if (getParentFragment() instanceof FeatureCategoryListener)
      ((FeatureCategoryListener) getParentFragment()).onFeatureCategorySelected(category);
  }
}
