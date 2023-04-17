package app.organicmaps.car.screens.search;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
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

public class SearchScreen extends BaseMapScreen implements SearchTemplate.SearchCallback, NativeSearchListener
{
  private final int MAX_RESULTS_SIZE;

  @NonNull
  private String mQuery = "";

  @Nullable
  private ItemList mResults = null;

  private SearchScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext, builder.mSurfaceRenderer);
    final ConstraintManager constraintManager = getCarContext().getCarService(ConstraintManager.class);
    MAX_RESULTS_SIZE = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST);

    onSearchSubmitted(builder.mQuery);
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final SearchTemplate.Builder builder = new SearchTemplate.Builder(this);
    builder.setHeaderAction(Action.BACK);
    builder.setShowKeyboardByDefault(false);
    if (mQuery.isEmpty() && mResults == null)
    {
      if (!loadRecents())
        builder.setShowKeyboardByDefault(true);
    }
    if (!mQuery.isEmpty() && mResults != null && !mResults.getItems().isEmpty())
      builder.setActionStrip(createActionStrip());
    if (mResults == null)
      builder.setLoading(true);
    else
      builder.setItemList(mResults);
    builder.setInitialSearchText(mQuery);
    builder.setSearchHint(getCarContext().getString(R.string.search));
    return builder.build();
  }

  @Override
  public void onSearchTextChanged(@NonNull String searchText)
  {
    if (mQuery.equals(searchText))
      return;
    mQuery = searchText;
    mResults = null;

    SearchEngine.INSTANCE.cancel();

    if (mQuery.isEmpty())
    {
      invalidate();
      return;
    }

    final MapObject location = LocationHelper.INSTANCE.getMyPosition();
    final boolean hasLocation = location != null;
    final double lat = hasLocation ? location.getLat() : 0;
    final double lon = hasLocation ? location.getLon() : 0;

    SearchEngine.INSTANCE.search(getCarContext(), mQuery, false, System.nanoTime(), hasLocation, lat, lon);
    invalidate();
  }

  @Override
  public void onSearchSubmitted(@NonNull String searchText)
  {
    onSearchTextChanged(searchText);
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.addListener(this);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.removeListener(this);
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
    builder.setOnClickListener(() ->
        getScreenManager().push(new SearchOnMapScreen.Builder(getCarContext(), getSurfaceRenderer()).setQuery(mQuery).build()));

    return new ActionStrip.Builder().addAction(builder.build()).build();
  }

  /**
   * A builder of {@link SearchScreen}.
   */
  public static final class Builder
  {
    @NonNull
    private final CarContext mCarContext;
    @NonNull
    private final SurfaceRenderer mSurfaceRenderer;

    @NonNull
    private String mQuery = "";

    public Builder(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
    {
      mCarContext = carContext;
      mSurfaceRenderer = surfaceRenderer;
    }

    @NonNull
    public Builder setQuery(@NonNull String query)
    {
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
