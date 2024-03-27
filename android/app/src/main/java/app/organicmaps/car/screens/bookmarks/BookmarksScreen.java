package app.organicmaps.car.screens.bookmarks;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Pane;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapTemplate;

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
    final MapTemplate.Builder builder = new MapTemplate.Builder();

    builder.setHeader(createHeader());
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    if (mBookmarksList == null)
    {
      builder.setPane(new Pane.Builder().setLoading(true).build());
      BookmarksLoader.load(getCarContext(), mBookmarkCategory, (bookmarksList) -> {
        mBookmarksList = bookmarksList;
        invalidate();
      });
    }
    else
      builder.setItemList(mBookmarksList);
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
}
