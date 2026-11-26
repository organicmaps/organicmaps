package app.organicmaps.car.screens.search;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.when;

import android.content.Context;
import androidx.car.app.AppManager;
import androidx.car.app.OnDoneCallback;
import androidx.car.app.ScreenManager;
import androidx.car.app.constraints.ConstraintManager;
import androidx.car.app.model.Row;
import androidx.car.app.model.SearchTemplate;
import androidx.car.app.testing.TestCarContext;
import androidx.lifecycle.Lifecycle;
import app.organicmaps.sdk.OrganicMaps;
import app.organicmaps.sdk.car.renderer.Renderer;
import app.organicmaps.sdk.location.LocationHelper;
import app.organicmaps.sdk.search.Popularity;
import app.organicmaps.sdk.search.SearchEngine;
import app.organicmaps.sdk.search.SearchRecents;
import app.organicmaps.sdk.search.SearchResult;
import app.organicmaps.sdk.util.Distance;
import app.organicmaps.sdk.util.Language;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockedStatic;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;
import org.robolectric.annotation.Implementation;
import org.robolectric.annotation.Implements;

@RunWith(RobolectricTestRunner.class)
@Config(sdk = 34, application = android.app.Application.class,
        shadows = {SearchScreenHandoffTest.Sdk.class, SearchScreenHandoffTest.Search.class,
                   SearchScreenHandoffTest.Locale.class})
public class SearchScreenHandoffTest
{
  @Test
  public void returningSearchReplacesThePhonesResultsWithItsOwnQuery()
  {
    final SearchScreen screen = openCoffeeSearch();
    final TestCarContext context = (TestCarContext) screen.getCarContext();
    final ScreenManager stack = context.getCarService(ScreenManager.class);
    final long firstRequest = Search.timestamp;
    final SearchResult coffee = new SearchResult("Cafe", "Cafe", 0, 0, new int[0], new int[0]);
    SearchEngine.INSTANCE.onResultsUpdate(new SearchResult[] {coffee}, firstRequest);
    assertEquals("Cafe", firstTitle(screen));

    stack.push(new SearchOnMapScreen.Builder(context, screen.getOrganicMapsContext(), mock(Renderer.class))
                   .setQuery("hotel")
                   .build());
    final SearchResult hotel = new SearchResult("Hotel", "Hotel", 0, 0, new int[0], new int[0]);
    SearchEngine.INSTANCE.onResultsUpdate(new SearchResult[] {hotel}, firstRequest + 1);
    stack.pop();
    assertTrue(Search.timestamp > firstRequest);
    assertEquals("coffee", Search.query);
    SearchEngine.INSTANCE.onResultsUpdate(new SearchResult[] {hotel}, firstRequest);
    assertTrue(((SearchTemplate) screen.onGetTemplate()).isLoading());
    SearchEngine.INSTANCE.onResultsUpdate(new SearchResult[] {coffee}, Search.timestamp);
    assertEquals("Cafe", firstTitle(screen));
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.DESTROYED);
  }

  private static SearchScreen openCoffeeSearch()
  {
    final TestCarContext context = TestCarContext.createCarContext(RuntimeEnvironment.getApplication());
    context.overrideCarService(AppManager.class, mock(AppManager.class));
    final ConstraintManager limits = mock(ConstraintManager.class);
    when(limits.getContentLimit(ConstraintManager.CONTENT_LIMIT_TYPE_LIST)).thenReturn(5);
    context.overrideCarService(ConstraintManager.class, limits);
    final OrganicMaps sdk = mock(OrganicMaps.class);
    when(sdk.getLocationHelper()).thenReturn(mock(LocationHelper.class));
    final SearchScreen screen = new SearchScreen.Builder(context, sdk, mock(Renderer.class)).setQuery("coffee").build();
    final ScreenManager stack = context.getCarService(ScreenManager.class);
    context.getLifecycleOwner().getRegistry().setCurrentState(Lifecycle.State.RESUMED);
    stack.push(screen);
    return screen;
  }

  @Test
  public void obsoleteTapRefreshesTheQueryAndOnlyTheCurrentRowIsSelected()
  {
    final SearchScreen screen = openCoffeeSearch();
    final SearchResult coffee =
        new SearchResult("Cafe", new SearchResult.Description("", "", Distance.EMPTY, "", 0, 0, 0, false), 1, 2,
                         new int[0], new int[0], Popularity.defaultInstance());
    final SearchResult[] first = {coffee};
    Search.selected = -1;
    SearchEngine.INSTANCE.onResultsUpdate(first, Search.timestamp);
    final Row old = (Row) ((SearchTemplate) screen.onGetTemplate()).getItemList().getItems().get(0);
    final long previous = Search.timestamp;
    SearchEngine.INSTANCE.onResultsUpdate(new SearchResult[0], previous + 1);
    try (MockedStatic<SearchRecents> ignored = mockStatic(SearchRecents.class))
    {
      old.getOnClickDelegate().sendClick(mock(OnDoneCallback.class));
      assertEquals(-1, Search.selected);
      assertTrue(Search.timestamp > previous);
      assertTrue(((SearchTemplate) screen.onGetTemplate()).isLoading());
      SearchEngine.INSTANCE.onResultsUpdate(new SearchResult[] {coffee}, Search.timestamp);
      final Row current = (Row) ((SearchTemplate) screen.onGetTemplate()).getItemList().getItems().get(0);
      current.getOnClickDelegate().sendClick(mock(OnDoneCallback.class));
      assertEquals(0, Search.selected);
    }
    ((TestCarContext) screen.getCarContext())
        .getLifecycleOwner()
        .getRegistry()
        .setCurrentState(Lifecycle.State.DESTROYED);
  }

  private static String firstTitle(SearchScreen screen)
  {
    final SearchTemplate template = (SearchTemplate) screen.onGetTemplate();
    return ((Row) template.getItemList().getItems().get(0)).getTitle().toString();
  }

  @Implements(SearchEngine.class)
  public static class Search
  {
    static int selected;
    @Implementation
    protected static void nativeSelectResult(int index)
    {
      selected = index;
    }
    static long timestamp;
    static String query;

    @Implementation
    protected boolean search(Context context, String text, boolean category, long request, boolean hasPosition,
                             double lat, double lon)
    {
      timestamp = request;
      query = text;
      return true;
    }
    @Implementation
    protected void cancel()
    {
      query = null;
    }

    @Implementation
    protected boolean searchInteractive(String text, boolean category, String locale, long request, boolean mapAndTable,
                                        boolean hasPosition, double lat, double lon)
    {
      query = text;
      return true;
    }
  }

  @Implements(OrganicMaps.class)
  public static class Sdk
  {
    @Implementation
    protected static void __staticInitializer__()
    {}
  }

  @Implements(Language.class)
  public static class Locale
  {
    @Implementation
    protected static String getDefaultLocale()
    {
      return "en";
    }
  }
}
