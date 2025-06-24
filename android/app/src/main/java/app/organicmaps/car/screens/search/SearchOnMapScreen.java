package app.organicmaps.car.screens.search;

import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.car.app.CarContext;
import androidx.car.app.constraints.ConstraintManager;
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
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.car.SurfaceRenderer;
import app.organicmaps.car.screens.base.BaseMapScreen;
import app.organicmaps.car.util.UiHelpers;
import app.organicmaps.sdk.search.SearchListener;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.search.SearchRecents;
import app.organicmaps.sdk.search.SearchResult;
import app.organicmaps.sdk.util.Language;

public class SearchOnMapScreen extends BaseMapScreen implements SearchListener
{
  private final int MAX_RESULTS_SIZE;

  @NonNull
  private final String mQuery;
  @NonNull
  private final String mLocale;
  private final boolean mIsCategory;

  @Nullable
  private ItemList mResults = null;

  private SearchOnMapScreen(@NonNull Builder builder)
  {
    super(builder.mCarContext, builder.mSurfaceRenderer);
    final ConstraintManager constraintManager = getCarContext().getCarService(ConstraintManager.class);
    MAX_RESULTS_SIZE = constraintManager.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_PLACE_LIST);

    mQuery = builder.mQuery;
    mLocale = builder.mLocale;
    mIsCategory = builder.mIsCategory;
  }

  @NonNull
  @Override
  public Template onGetTemplate()
  {
    final MapWithContentTemplate.Builder builder = new MapWithContentTemplate.Builder();
    builder.setMapController(UiHelpers.createMapController(getCarContext(), getSurfaceRenderer()));
    builder.setContentTemplate(createResultsListTemplate());
    return builder.build();
  }

  @NonNull
  private ListTemplate createResultsListTemplate()
  {
    final ListTemplate.Builder builder = new ListTemplate.Builder();
    builder.setHeader(createHeader());
    if (mResults == null)
      builder.setLoading(true);
    else
      builder.setSingleList(mResults);

    return builder.build();
  }

  @Override
  public void onResultsUpdate(@NonNull SearchResult[] results, long timestamp)
  {
    if (mResults != null)
      return;

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
      builder.addText(result.description.description);

      final CharSequence openingHours = SearchUiHelpers.getOpeningHoursText(getCarContext(), result);
      final CharSequence distance = SearchUiHelpers.getDistanceText(result);
      final CharSequence openingHoursAndDistanceText = SearchUiHelpers.concatenateStrings(openingHours, distance);
      if (!TextUtils.isEmpty(openingHoursAndDistanceText))
        builder.addText(openingHoursAndDistanceText);
      if (distance.length() == 0)
      {
        // All non-browsable rows must have a distance span attached to either its title or texts
        builder.setBrowsable(true);
      }

      builder.setOnClickListener(() -> {
        SearchRecents.add(title, getCarContext());
        SearchEngine.INSTANCE.cancel();
        SearchEngine.INSTANCE.showResult(resultIndex);
      });
    }
    else
    {
      builder.setBrowsable(true);
      builder.setTitle(result.suggestion);
      builder.setImage(new CarIcon.Builder(IconCompat.createWithResource(getCarContext(), R.drawable.ic_search)).build());
      builder.setOnClickListener(() -> getScreenManager().push(new Builder(getCarContext(), getSurfaceRenderer()).setQuery(result.suggestion).build()));
    }
    return builder.build();
  }

  @Override
  public void onStart(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.addListener(this);
  }

  @Override
  public void onResume(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.cancel();

    final MapObject location = MwmApplication.from(getCarContext()).getLocationHelper().getMyPosition();
    final boolean hasLocation = location != null;
    final double lat = hasLocation ? location.getLat() : 0;
    final double lon = hasLocation ? location.getLon() : 0;

    SearchEngine.INSTANCE.searchInteractive(mQuery, mIsCategory, mLocale, System.nanoTime(), true /* isMapAndTable */, hasLocation, lat, lon);
  }

  @Override
  public void onStop(@NonNull LifecycleOwner owner)
  {
    SearchEngine.INSTANCE.removeListener(this);
    SearchEngine.INSTANCE.cancel();
  }

  @NonNull
  private Header createHeader()
  {
    final Header.Builder builder = new Header.Builder();
    builder.setStartHeaderAction(Action.BACK);
    builder.setTitle(mQuery);
    return builder.build();
  }

  /**
   * A builder of {@link SearchOnMapScreen}.
   */
  public static final class Builder
  {
    @NonNull
    private final CarContext mCarContext;
    @NonNull
    private final SurfaceRenderer mSurfaceRenderer;

    @NonNull
    private String mQuery = "";
    @NonNull
    private String mLocale;
    private boolean mIsCategory;

    public Builder(@NonNull CarContext carContext, @NonNull SurfaceRenderer surfaceRenderer)
    {
      mCarContext = carContext;
      mSurfaceRenderer = surfaceRenderer;
      mLocale = Language.getKeyboardLocale(mCarContext);
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

    public Builder setLocale(@NonNull String locale)
    {
      mLocale = locale;
      return this;
    }

    @NonNull
    public SearchOnMapScreen build()
    {
      if (mQuery.isEmpty())
        throw new IllegalStateException("Search query is empty");
      return new SearchOnMapScreen(this);
    }
  }
}
