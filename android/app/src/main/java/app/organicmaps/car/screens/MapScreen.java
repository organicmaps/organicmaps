package app.organicmaps.car.screens;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.GridItem;
import androidx.car.app.model.GridTemplate;
import androidx.car.app.model.Header;
import androidx.car.app.model.Item;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.core.graphics.drawable.IconCompat;
import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.screens.bookmarks.BookmarkCategoriesScreen;
import app.organicmaps.car.screens.search.SearchScreen;
import app.organicmaps.car.screens.settings.SettingsScreen;
import app.organicmaps.car.util.SuggestionsHelpers;
import app.organicmaps.car.util.UiHelpers;

public class MapScreen extends BaseMapScreen
{
  public MapScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    SuggestionsHelpers.updateSuggestions(getCarContext());

    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setActionStrip(createActionStrip());
    builder.setContentTemplate(createGridTemplate());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(new Action.Builder(Action.APP_ICON).build());
    builder.setTitle(getCarContext().getString(R.string.app_name));
    return builder.build();
  }

  @NonNull
  private ActionStrip createActionStrip()
  {
    final Action.Builder freeDriveScreenBuilder = new Action.Builder();
    freeDriveScreenBuilder.setIcon(
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_steering_wheel)).build());
    freeDriveScreenBuilder.setOnClickListener(
        () -> getScreenManager().push(new FreeDriveScreen(getCarContext(), getSurfaceRenderer())));

    final ActionStrip.Builder builder = new ActionStrip.Builder();
    builder.addAction(freeDriveScreenBuilder.build());
    return builder.build();
  }

  @NonNull
  private GridTemplate createGridTemplate()
  {
    final GridTemplate.Builder builder = new GridTemplate.Builder();

    final ItemList.Builder itemsBuilder = new ItemList.Builder();
    itemsBuilder.addItem(createSearchItem());
    itemsBuilder.addItem(createCategoriesItem());
    itemsBuilder.addItem(createBookmarksItem());
    itemsBuilder.addItem(createSettingsItem());

    builder.setHeader(createHeader());
    builder.setSingleList(itemsBuilder.build());
    return builder.build();
  }

  @NonNull
  private Item createSearchItem()
  {
    final CarIcon iconSearch =
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_search)).build();

    final GridItem.Builder builder = new GridItem.Builder();
    builder.setTitle(getCarContext().getString(R.string.search));
    builder.setImage(iconSearch);
    builder.setOnClickListener(this::openSearch);
    return builder.build();
  }

  @NonNull
  private Item createCategoriesItem()
  {
    final CarIcon iconCategories =
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_address)).build();

    final GridItem.Builder builder = new GridItem.Builder();
    builder.setImage(iconCategories);
    builder.setTitle(getCarContext().getString(R.string.categories));
    builder.setOnClickListener(this::openCategories);
    return builder.build();
  }

  @NonNull
  private Item createBookmarksItem()
  {
    final CarIcon iconBookmarks =
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_bookmarks)).build();

    final GridItem.Builder builder = new GridItem.Builder();
    builder.setImage(iconBookmarks);
    builder.setTitle(getCarContext().getString(R.string.bookmarks));
    builder.setOnClickListener(this::openBookmarks);
    return builder.build();
  }

  @NonNull
  private Item createSettingsItem()
  {
    final CarIcon iconSettings =
        new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_settings)).build();

    final GridItem.Builder builder = new GridItem.Builder();
    builder.setImage(iconSettings);
    builder.setTitle(getCarContext().getString(R.string.settings));
    builder.setOnClickListener(this::openSettings);
    return builder.build();
  }

  private void openSearch()
  {
    // Details in UiHelpers.createSettingsAction()
    if (getScreenManager().getTop() != this)
      return;
    getScreenManager().push(new SearchScreen.Builder(getCarContext(), getSurfaceRenderer()).build());
  }

  private void openCategories()
  {
    // Details in UiHelpers.createSettingsAction()
    if (getScreenManager().getTop() != this)
      return;
    getScreenManager().push(new CategoriesScreen(getCarContext(), getSurfaceRenderer()));
  }

  private void openBookmarks()
  {
    // Details in UiHelpers.createSettingsAction()
    if (getScreenManager().getTop() != this)
      return;
    getScreenManager().push(new BookmarkCategoriesScreen(getCarContext(), getSurfaceRenderer()));
  }

  private void openSettings()
  {
    // Details in UiHelpers.createSettingsAction()
    if (getScreenManager().getTop() != this)
      return;
    getScreenManager().push(new SettingsScreen(getCarContext(), getSurfaceRenderer()));
  }
}
