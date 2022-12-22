package app.organicmaps.car.screens;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
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

import java.util.Arrays;
import java.util.List;

public class CategoriesScreen extends BaseMapScreen
{
  private static class CategoryData
  {
    @StringRes
    public final int nameResId;

    @DrawableRes
    public final int iconResId;
    @DrawableRes
    public final int iconNightResId;

    public CategoryData(@StringRes int nameResId, @DrawableRes int iconResId, @DrawableRes int iconNightResId)
    {
      this.nameResId = nameResId;
      this.iconResId = iconResId;
      this.iconNightResId = iconNightResId;
    }
  }

  // TODO (AndrewShkrob): discuss categories and their priority for this list
  private static final List<CategoryData> CATEGORIES = Arrays.asList(
      new CategoryData(R.string.fuel, R.drawable.ic_category_fuel, R.drawable.ic_category_fuel_night),
      new CategoryData(R.string.parking, R.drawable.ic_category_parking, R.drawable.ic_category_parking_night),
      new CategoryData(R.string.eat, R.drawable.ic_category_eat, R.drawable.ic_category_eat_night),
      new CategoryData(R.string.food, R.drawable.ic_category_food, R.drawable.ic_category_food_night),
      new CategoryData(R.string.hotel, R.drawable.ic_category_hotel, R.drawable.ic_category_hotel_night),
      new CategoryData(R.string.toilet, R.drawable.ic_category_toilet, R.drawable.ic_category_toilet_night),
      new CategoryData(R.string.rv, R.drawable.ic_category_rv, R.drawable.ic_category_rv_night)
  );

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
    final boolean isNightMode = ThemeUtils.isNightMode(getCarContext());
    final ItemList.Builder builder = new ItemList.Builder();
    final int categoriesSize = Math.min(CATEGORIES.size(), MAX_CATEGORIES_SIZE);
    for (int i = 0; i < categoriesSize; ++i)
    {
      final Row.Builder itemBuilder = new Row.Builder();
      final String title = getCarContext().getString(CATEGORIES.get(i).nameResId);
      @DrawableRes final int iconResId = isNightMode ? CATEGORIES.get(i).iconNightResId : CATEGORIES.get(i).iconResId;

      itemBuilder.setTitle(title);
      itemBuilder.setImage(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), iconResId)).build());
      itemBuilder.setOnClickListener(() -> getScreenManager().push(new SearchOnMapScreen.Builder(getCarContext(), getSurfaceRenderer()).setCategory(title).build()));
      builder.addItem(itemBuilder.build());
    }
    return builder.build();
  }
}
