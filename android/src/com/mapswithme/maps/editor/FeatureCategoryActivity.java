package com.mapswithme.maps.editor;

import android.content.Intent;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.editor.data.FeatureCategory;

public class FeatureCategoryActivity extends BaseToolbarActivity implements FeatureCategoryFragment.FeatureCategoryListener
{
  public static final String EXTRA_FEATURE_CATEGORY = "FeatureCategory";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return FeatureCategoryFragment.class;
  }

  @Override
  @StringRes
  protected int getToolbarTitle()
  {
    return R.string.editor_add_select_category;
  }

  @Override
  public void onFeatureCategorySelected(FeatureCategory category)
  {
    Editor.createMapObject(category);
    final Intent intent = new Intent(this, EditorActivity.class);
    intent.putExtra(EXTRA_FEATURE_CATEGORY, category);
    intent.putExtra(EditorActivity.EXTRA_NEW_OBJECT, true);
    startActivity(intent);
  }
}
