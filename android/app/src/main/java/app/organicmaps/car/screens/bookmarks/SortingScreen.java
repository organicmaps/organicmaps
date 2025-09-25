package app.organicmaps.car.screens.bookmarks;

import android.location.Location;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.car.app.CarContext;
import androidx.car.app.model.Action;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.Header;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.ListTemplate;
import androidx.car.app.model.Row;
import androidx.car.app.model.Template;
import androidx.car.app.navigation.model.MapWithContentTemplate;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.car.renderer.Renderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import java.util.Arrays;
import java.util.stream.IntStream;

class SortingScreen extends BaseMapScreen
{
  private static final int DEFAULT_SORTING_TYPE = -1;

  @NonNull
  private final CarIcon mRadioButtonIcon;
  @NonNull
  private final CarIcon mRadioButtonSelectedIcon;

  private final long mBookmarkCategoryId;
  private final @BookmarkManager.SortingType int mLastSortingType;

  private @BookmarkManager.SortingType int mNewSortingType;

  public SortingScreen(@NonNull CarContext carContext, @NonNull Renderer surfaceRenderer,
                       @NonNull BookmarkCategory bookmarkCategory)
  {
    super(carContext, surfaceRenderer);
    mRadioButtonIcon =
        new CarIcon.Builder(IconCompat.createWithResource(carContext, R.drawable.ic_radio_button_unchecked)).build();
    mRadioButtonSelectedIcon =
        new CarIcon.Builder(IconCompat.createWithResource(carContext, R.drawable.ic_radio_button_checked)).build();

    mBookmarkCategoryId = bookmarkCategory.getId();
    mLastSortingType = mNewSortingType = getLastSortingType();
  }

  @NonNull
  @Override
  protected Template onGetTemplateImpl()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createSortingTypesListTemplate());
    return builder.build();
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    super.onStop(owner);
    final boolean sortingTypeChanged = mNewSortingType != mLastSortingType;
    setResult(sortingTypeChanged);
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(getCarContext().getString(R.string.sort_bookmarks));
    return builder.build();
  }

  @NonNull
  private ListTemplate createSortingTypesListTemplate()
  {
    final ListTemplate.Builder builder = new ListTemplate.Builder();
    builder.setHeader(createHeader());

    builder.setSingleList(createSortingTypesList(getAvailableSortingTypes(), getLastAvailableSortingType()));

    return builder.build();
  }

  @NonNull
  private ItemList createSortingTypesList(@NonNull final @BookmarkManager.SortingType int[] availableSortingTypes,
                                          final int lastSortingType)
  {
    final ItemList.Builder builder = new ItemList.Builder();
    for (int type :
         IntStream.concat(IntStream.of(DEFAULT_SORTING_TYPE), Arrays.stream(availableSortingTypes)).toArray())
    {
      final Row.Builder rowBuilder = new Row.Builder();
      rowBuilder.setTitle(getCarContext().getString(sortingTypeToStringRes(type)));
      if (type == lastSortingType)
        rowBuilder.setImage(mRadioButtonSelectedIcon);
      else
      {
        rowBuilder.setImage(mRadioButtonIcon);
        rowBuilder.setOnClickListener(() -> {
          if (type == DEFAULT_SORTING_TYPE)
            BookmarkManager.INSTANCE.resetLastSortingType(mBookmarkCategoryId);
          else
            BookmarkManager.INSTANCE.setLastSortingType(mBookmarkCategoryId, type);
          mNewSortingType = type;
          invalidate();
        });
      }
      builder.addItem(rowBuilder.build());
    }
    return builder.build();
  }

  @StringRes
  private int sortingTypeToStringRes(@BookmarkManager.SortingType int sortingType)
  {
    return switch (sortingType)
    {
      case BookmarkManager.SORT_BY_TYPE -> R.string.by_type;
      case BookmarkManager.SORT_BY_DISTANCE -> R.string.by_distance;
      case BookmarkManager.SORT_BY_TIME -> R.string.by_date;
      case BookmarkManager.SORT_BY_NAME -> R.string.by_name;
      default -> R.string.by_default;
    };
  }

  @NonNull
  @BookmarkManager.SortingType
  private int[] getAvailableSortingTypes()
  {
    final Location loc = MwmApplication.from(getCarContext()).getLocationHelper().getSavedLocation();
    final boolean hasMyPosition = loc != null;
    return BookmarkManager.INSTANCE.getAvailableSortingTypes(mBookmarkCategoryId, hasMyPosition);
  }

  private int getLastSortingType()
  {
    if (BookmarkManager.INSTANCE.hasLastSortingType(mBookmarkCategoryId))
      return BookmarkManager.INSTANCE.getLastSortingType(mBookmarkCategoryId);
    return DEFAULT_SORTING_TYPE;
  }

  private int getLastAvailableSortingType()
  {
    int currentType = getLastSortingType();
    @BookmarkManager.SortingType
    int[] types = getAvailableSortingTypes();
    for (@BookmarkManager.SortingType int type : types)
    {
      if (type == currentType)
        return currentType;
    }
    return DEFAULT_SORTING_TYPE;
  }
}
