package app.organicmaps.car.screens.bookmarks;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import app.organicmaps.R;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import java.util.ArrayList;
import java.util.List;

public class BookmarkCategoriesScreen extends BaseMapScreen
{
  private final int MAX_CATEGORIES_SIZE;

  public BookmarkCategoriesScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
  {
    super(carContext, surfaceRenderer);
    final ConstraintManager constraintManager = getCarContext().getCarService(ConstraintManager.class);
    MAX_CATEGORIES_SIZE = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createBookmarkCategoriesListTemplate());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.bookmarks));
    return builder.build();
  }

  @NonNull
  private ListTemplate createBookmarkCategoriesListTemplate()
  {
    final List<BookmarkCategory> bookmarkCategories = getBookmarks();
    final int categoriesSize = Math.min(bookmarkCategories.size(), MAX_CATEGORIES_SIZE);

    final ItemList.Builder builder = new ItemList.Builder();
    for (int i = 0; i < categoriesSize; ++i)
    {
      final BookmarkCategory bookmarkCategory = bookmarkCategories.get(i);

      Row.Builder itemBuilder = new Row.Builder();
      itemBuilder.setTitle(bookmarkCategory.getName());
      itemBuilder.addText(bookmarkCategory.getDescription());
      itemBuilder.setOnClickListener(
          () -> getScreenManager().push(new BookmarksScreen(getCarContext(), getSurfaceRenderer(), bookmarkCategory)));
      itemBuilder.setBrowsable(true);
      builder.addItem(itemBuilder.build());
    }
    return new ListTemplate.Builder().setHeader(createHeader()).setSingleList(builder.build()).build();
  }

  @NonNull
  private static List<BookmarkCategory> getBookmarks()
  {
    final List<BookmarkCategory> bookmarkCategories = new ArrayList<>(BookmarkManager.INSTANCE.getCategories());

    final List<BookmarkCategory> toRemove = new ArrayList<>();
    for (final BookmarkCategory bookmarkCategory : bookmarkCategories)
    {
      if (bookmarkCategory.getBookmarksCount() == 0 || !bookmarkCategory.isVisible())
        toRemove.add(bookmarkCategory);
    }
    bookmarkCategories.removeAll(toRemove);

    return bookmarkCategories;
  }
}
