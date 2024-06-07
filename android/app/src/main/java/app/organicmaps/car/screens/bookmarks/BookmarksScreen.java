package app.organicmaps.car.screens.bookmarks;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;

import app.organicmaps.bookmarks.data.BookmarkCategory;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.UiHelpers;

public class BookmarksScreen extends BaseMapScreen
{
  @NonNull
  private final BookmarkCategory mBookmarkCategory;

  @Nullable
  private ItemList mBookmarksList = null;

  public BookmarksScreen(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer, @NonNull BookmarkCategory bookmarkCategory)
  {
    super(carContext, surfaceRenderer);
    mBookmarkCategory = bookmarkCategory;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createBookmarksListTemplate());
    return builder.build();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(mBookmarkCategory.getName());
    return builder.build();
  }

  @NonNull
  private ListTemplate createBookmarksListTemplate()
  {
    final ListTemplate.Builder builder = new ListTemplate.Builder();
    builder.setHeader(createHeader());

    if (mBookmarksList == null)
    {
      builder.setLoading(true);
      BookmarksLoader.load(getCarContext(), mBookmarkCategory, (bookmarksList) -> {
        mBookmarksList = bookmarksList;
        invalidate();
      });
    }
    else
      builder.setSingleList(mBookmarksList);

    return builder.build();
  }
}
