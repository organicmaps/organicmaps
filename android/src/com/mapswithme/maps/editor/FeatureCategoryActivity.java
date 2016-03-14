package com.mapswithme.maps.editor;

import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.Size;
import android.support.v4.app.Fragment;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.base.BaseToolbarActivity;
import com.mapswithme.maps.editor.data.FeatureCategory;

public class FeatureCategoryActivity extends BaseToolbarActivity implements FeatureCategoryFragment.FeatureCategoryListener
{
  public static final String EXTRA_FEATURE_CATEGORY = "FeatureCategory";
  public static final String EXTRA_LAT_LON = "LatLon";

  private double[] mLatLon;

  public static void pickFeatureCategory(MwmActivity parent, @Size(2) double[] rect)
  {
    final Intent intent = new Intent(parent, FeatureCategoryActivity.class);
    intent.putExtra(EXTRA_LAT_LON, rect);
    parent.startActivity(intent);
  }

  @Override
  protected void onCreate(Bundle state)
  {
    super.onCreate(state);
    mLatLon = getIntent().getDoubleArrayExtra(EXTRA_LAT_LON);
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return FeatureCategoryFragment.class;
  }

  @Override
  protected int getToolbarTitle()
  {
    // TODO set correct text
    return super.getToolbarTitle();
  }

  @Override
  public void onFeatureCategorySelected(FeatureCategory category)
  {
    Editor.createMapObject(category, mLatLon);
    final Intent intent = new Intent(this, EditorActivity.class);
    intent.putExtra(EXTRA_FEATURE_CATEGORY, category);
    intent.putExtra(EditorActivity.EXTRA_NEW_OBJECT, true);
    startActivity(intent);
  }
}
