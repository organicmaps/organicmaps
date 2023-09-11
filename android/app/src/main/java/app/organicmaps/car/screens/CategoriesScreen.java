package app.organicmaps.car.screens;

import android.content.res.Resources;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;
import androidx.core.graphics.drawable.IconCompat;

import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.search.SearchOnMapScreen;
import app.organicmaps.car.util.ThemeUtils;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.search.DisplayedCategories;

public class CategoriesScreen extends BaseMapScreen
{
  private final int MAX_CATEGORIES_SIZE;

  public CategoriesScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
    final ConstraintManager constraintManager = getCarContext().getCarService(ConstraintManager.class);
    MAX_CATEGORIES_SIZE = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MapTemplate.Builder builder = new MapTemplate.Builder();
    builder.setHeader(createHeader());
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setItemList(createCategoriesList());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.categories));
    return builder.build();
  }

  @NonNull
  private ItemList createCategoriesList()
  {
    final Resources resources = getCarContext().getResources();
    final String packageName = getCarContext().getPackageName();

    final boolean isNightMode = ThemeUtils.isNightMode(getCarContext());
    final String[] categoriesKeys = DisplayedCategories.getKeys();
    final ItemList.Builder builder = new ItemList.Builder();
    final int categoriesSize = Math.min(categoriesKeys.length, MAX_CATEGORIES_SIZE);
    for (int i = 0; i < categoriesSize; ++i)
    {
      final Row.Builder itemBuilder = new Row.Builder();
      final String title = getCarContext().getString(DisplayedCategories.getTitleResId(resources, packageName, categoriesKeys[i]));
      @DrawableRes final int iconResId = DisplayedCategories.getDrawableResId(resources, packageName, isNightMode, categoriesKeys[i]);

      itemBuilder.setTitle(title);
      itemBuilder.setImage(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), iconResId)).build());
      itemBuilder.setOnClickListener(() -> getScreenManager().push(new SearchOnMapScreen.Builder(getCarContext(), getSurfaceRenderer()).setCategory(title).build()));
      builder.addItem(itemBuilder.build());
    }
    return builder.build();
  }
}
