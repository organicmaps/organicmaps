package app.organicmaps.car.screens.search;

import androidx.annotation.NonNull;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Action;
import androidx.car.app.model.ActionStrip;
import androidx.car.app.model.CarIcon;
import androidx.car.app.model.ItemList;
import androidx.car.app.model.Row;
import androidx.car.app.model.SearchTemplate;
import androidx.car.app.model.Template;
import androidx.core.graphics.drawable.IconCompat;
import androidx.lifecycle.LifecycleOwner;

import app.organicmaps.R;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.search.NativeSearchListener;
import app.organicmaps.search.SearchEngine;
import app.organicmaps.search.SearchRecents;
import app.organicmaps.search.SearchResult;
import app.organicmaps.util.log.Logger;

public class SearchScreen extends BaseMapScreen implements SearchTemplate.SearchCallback, NativeSearchListener
{
  private static final String TAG = SearchScreen.class.getSimpleName();

  private final int MAX_RESULTS_SIZE;
  private ItemList mResults;
  private String mQuery = "";
  private boolean mIsCategory;

  private SearchScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext, builder.mSurfaceRenderer);
    final ConstraintManager constraintManager = getCarContext().getCarService(ConstraintManager.class);
    MAX_RESULTS_SIZE = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);

    mIsCategory = builder.mIsCategory;
    onSearchSubmitted(builder.mQuery);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    SearchTemplate.Builder builder = new SearchTemplate.Builder(this);
    builder.setHeaderAction(Action.BACK);
    builder.setShowKeyboardByDefault(false);
    if (mQuery.isEmpty() && mResults == null)
    {
      if (!loadRecents())
        builder.setShowKeyboardByDefault(true);
    }
    if (!mQuery.isEmpty() && mResults != null)
      builder.setActionStrip(createActionStrip());
    if (mResults == null)
      builder.setLoading(true);
    else
      builder.setItemList(mResults);
    Logger.d(TAG, mQuery);
    builder.setInitialSearchText(mQuery);
    builder.setSearchHint(getCarContext().getString(R.string.search));
    return builder.build();
  }

  @Override
  public void onSearchTextChanged(@NonNull String searchText)
  {
    Logger.d(TAG, searchText);
    if (mQuery.equals(searchText))
      return;
    if (!mQuery.isEmpty())
      mIsCategory = false;
    mQuery = searchText;

    SearchEngine.INSTANCE.cancel();

    final MapObject location = LocationHelper.INSTANCE.getMyPosition();
    if (location != null)
      SearchEngine.INSTANCE.search(getCarContext(), mQuery, mIsCategory, System.nanoTime(), true, location.getLat(), location.getLon());
    else
      SearchEngine.INSTANCE.search(getCarContext(), mQuery, mIsCategory, System.nanoTime(), false, 0, 0);

    mResults = null;
    invalidate();
  }

  @Override
  public void onSearchSubmitted(@NonNull String searchText)
  {
    Logger.d(TAG, searchText);
    onSearchTextChanged(searchText);
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.addListener(this);
  }

  @Override
  public void onPause(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.removeListener(this);
  }

  @Override
  public void onDestroy(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.cancel();
  }

  @Override
  public void onResultsUpdate(@NonNull SearchResult[] results, long timestamp)
  {
    final ItemList.Builder builder = new ItemList.Builder();
    builder.setNoItemsMessage(getCarContext().getString(R.string.search_not_found));
    final int resultsSize = Math.min(results.length, MAX_RESULTS_SIZE);
    for (int i = 0; i < resultsSize; i++)
      builder.addItem(createResultItem(results[i], i));
    mResults = builder.build();
    invalidate();
  }

  @NonNull
  private Row createResultItem(@NonNull SearchResult result, int resultIndex)
  {
    final Row.Builder builder = new Row.Builder();
    if (result.type == SearchResult.TYPE_RESULT)
    {
      if (result.description != null)
        Logger.d(TAG, result.description.featureType);
      final String title = result.getTitle(getCarContext());
      builder.setTitle(title);
      builder.addText(result.getFormattedDescription(getCarContext()));
      final CharSequence openingHoursAndDistance = SearchUiHelpers.getOpeningHoursAndDistanceText(getCarContext(), result);
      if (openingHoursAndDistance.length() != 0)
        builder.addText(openingHoursAndDistance);
      builder.setOnClickListener(() -> {
        SearchRecents.add(title, getCarContext());
        SearchEngine.INSTANCE.cancel();
        SearchEngine.INSTANCE.showResult(resultIndex);
        getScreenManager().popToRoot();
      });
    }
    else
    {
      builder.setBrowsable(true);
      builder.setTitle(result.suggestion);
      builder.setImage(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_search)).build());
      builder.setOnClickListener(() -> onSearchSubmitted(result.suggestion));
    }
    return builder.build();
  }

  private boolean loadRecents()
  {
    final CarIcon iconRecent = new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_search_recent)).build();

    final ItemList.Builder builder = new ItemList.Builder();
    builder.setNoItemsMessage(getCarContext().getString(R.string.search_history_text));
    SearchRecents.refresh();
    final int recentsSize = Math.min(SearchRecents.getSize(), MAX_RESULTS_SIZE);
    for (int i = 0; i < recentsSize; ++i)
    {
      final Row.Builder itemBuilder = new Row.Builder();
      final String title = SearchRecents.get(i);
      itemBuilder.setTitle(title);
      itemBuilder.setImage(iconRecent);
      itemBuilder.setOnClickListener(() -> onSearchSubmitted(title));
      builder.addItem(itemBuilder.build());
    }
    mResults = builder.build();

    return recentsSize != 0;
  }

  @NonNull
  private ActionStrip createActionStrip()
  {
    final Action.Builder builder = new Action.Builder();
    builder.setIcon(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_show_on_map)).build());
    // TODO: uncomment when implementation of {@link SearchOnMapScreen} will be finished
    // builder.setOnClickListener(() ->
    //     getScreenManager().push(new SearchOnMapScreen.Builder(getCarContext(), getSurfaceRenderer()).setQuery(mQuery).build()));

    return new ActionStrip.Builder().addAction(builder.build()).build();
  }

  /**
   * A builder of {@link SearchScreen}.
   */
  public static final class Builder
  {
    private final CarContext mCarContext;
    private final SurfaceRenderer mSurfaceRenderer;

    private boolean mIsCategory;
    private String mQuery = "";

    public Builder(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
    {
      mCarContext = carContext;
      mSurfaceRenderer = surfaceRenderer;
    }

    public Builder setCategory(@NonNull String category)
    {
      mIsCategory = true;
      mQuery = category;
      return this;
    }

    public Builder setQuery(@NonNull String query)
    {
      mIsCategory = false;
      mQuery = query;
      return this;
    }

    @NonNull
    public SearchScreen build()
    {
      return new SearchScreen(this);
    }
  }
}
