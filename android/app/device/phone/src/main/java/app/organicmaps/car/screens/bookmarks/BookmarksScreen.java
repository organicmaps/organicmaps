package app.organicmaps.car.screens.bookmarks;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.R;
import app.organicmaps.car.renderer.Renderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;

public class BookmarksScreen extends BaseMapScreen
{
  @NonNull
  private final BookmarkCategory mBookmarkCategory;

  @NonNull
  private final BookmarksLoader mBookmarksLoader;

  @Nullable
  private ItemList mBookmarksList = null;

  private boolean mIsOnSortingScreen = false;

  public BookmarksScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer,
                         @NonNull BookmarkCategory bookmarkCategory)
  {
    super(carContext, surfaceRenderer);
    mBookmarkCategory = bookmarkCategory;
    mBookmarksLoader = new BookmarksLoader(carContext, mBookmarkCategory, this::onBookmarksLoaded);
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createBookmarksListTemplate());
    return builder.build();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    super.onStop(owner);
    if (!mIsOnSortingScreen)
      mBookmarksLoader.cancel();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(mBookmarkCategory.getName());
    builder.addEndHeaderAction(createSortingAction());
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
      mBookmarksLoader.load();
    }
    else
      builder.setSingleList(mBookmarksList);

    return builder.build();
  }

  @NonNull
  private Action createSortingAction()
  {
    final Action.Builder builder = new Action.Builder();
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_sort)).build());
    builder.setOnClickListener(() -> {
      mIsOnSortingScreen = true;
      getScreenManager().pushForResult(new SortingScreen(getCarContext(), getSurfaceRenderer(), mBookmarkCategory),
                                       this::onSortingResult);
    });
    return builder.build();
  }

  private void onBookmarksLoaded(@NonNull ItemList bookmarksList)
  {
    mBookmarksList = bookmarksList;
    invalidate();
  }

  private void onSortingResult(final Object result)
  {
    mIsOnSortingScreen = false;
    if (Boolean.TRUE.equals(result))
    {
      mBookmarksList = null;
      invalidate();
    }
  }
}
