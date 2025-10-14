package app.organicmaps.car.screens;

import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.GridItem;
import androidx.car.app.model.GridTemplate;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;
import app.organicmaps.car.renderer.Renderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.search.SearchOnMapScreen;
import app.organicmaps.car.util.UiHelpers;
import java.util.Arrays;
import java.util.List;

public class CategoriesScreen extends BaseMapScreen
{
  private record CategoryData(@StringRes int nameResId, @DrawableRes int iconResId) {}

  private static final List<CategoryData> CATEGORIES =
      Arrays.asList(new CategoryData(R.string.category_fuel, R.drawable.ic_category_fuel),
                    new CategoryData(R.string.category_parking, R.drawable.ic_category_parking),
                    new CategoryData(R.string.category_eat, R.drawable.ic_category_eat),
                    new CategoryData(R.string.category_food, R.drawable.ic_category_food),
                    new CategoryData(R.string.category_hotel, R.drawable.ic_category_hotel),
                    new CategoryData(R.string.category_toilet, R.drawable.ic_category_toilet),
                    new CategoryData(R.string.category_rv, R.drawable.ic_category_rv));

  private final int MAX_CATEGORIES_SIZE;

  public CategoriesScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
    final ConstraintManager constraintManager = getCarContext().getCarService(ConstraintManager.class);
    MAX_CATEGORIES_SIZE = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createCategoriesListTemplate());
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
  private GridTemplate createCategoriesListTemplate()
  {
    final ItemList.Builder builder = new ItemList.Builder();
    final int categoriesSize = Math.min(CATEGORIES.size(), MAX_CATEGORIES_SIZE);
    for (int i = 0; i < categoriesSize; ++i)
    {
      final GridItem.Builder itemBuilder = new GridItem.Builder();
      final String title = getCarContext().getString(CATEGORIES.get(i).nameResId);
      @DrawableRes
      final int iconResId = CATEGORIES.get(i).iconResId;

      itemBuilder.setTitle(title);
      itemBuilder.setImage(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), iconResId)).build());
      itemBuilder.setOnClickListener(
          ()
              -> getScreenManager().push(
                  new SearchOnMapScreen.Builder(getCarContext(), getSurfaceRenderer()).setCategory(title).build()));
      builder.addItem(itemBuilder.build());
    }
    return new GridTemplate.Builder().setHeader(createHeader()).setSingleList(builder.build()).build();
  }
}
