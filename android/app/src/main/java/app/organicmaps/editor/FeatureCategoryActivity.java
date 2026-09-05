package app.organicmaps.editor;

import android.content.Context;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.data.FeatureCategory;

public class FeatureCategoryActivity
    extends BaseMwmFragmentActivity implements FeatureCategoryFragment.FeatureCategoryListener
{
  public static final String EXTRA_FEATURE_CATEGORY = "FeatureCategory";
  static final String EXTRA_POSITION_LAT = "PositionLat";
  static final String EXTRA_POSITION_LON = "PositionLon";

  public static void start(@NonNull Context context, double lat, double lon)
  {
    context.startActivity(new Intent(context, FeatureCategoryActivity.class)
                              .putExtra(EXTRA_POSITION_LAT, lat)
                              .putExtra(EXTRA_POSITION_LON, lon));
  }

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return FeatureCategoryFragment.class;
  }

  @Override
  public void onFeatureCategorySelected(FeatureCategory category)
  {
    final Intent in = getIntent();
    final double lat = in.getDoubleExtra(EXTRA_POSITION_LAT, Double.NaN);
    final double lon = in.getDoubleExtra(EXTRA_POSITION_LON, Double.NaN);
    if (Double.isNaN(lat) || Double.isNaN(lon))
      throw new IllegalStateException("FeatureCategoryActivity missing position extras");

    if (!Editor.createMapObject(category, lat, lon))
    {
      // The position was checked when the user confirmed it, but the map under it can be updated or
      // deleted while the category is being picked. No other category can succeed at the same
      // point, so return to the map.
      InvalidFeaturePositionDialogFragment.show(getSupportFragmentManager());
      return;
    }

    final Intent intent = new Intent(this, EditorActivity.class);
    intent.putExtra(EXTRA_FEATURE_CATEGORY, category);
    intent.putExtra(EditorActivity.EXTRA_NEW_OBJECT, true);
    startActivity(intent);
    finish();
  }
}
