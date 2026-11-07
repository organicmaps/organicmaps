package app.organicmaps.search;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.os.Handler;
import androidx.arch.core.executor.ArchTaskExecutor;
import androidx.lifecycle.SavedStateHandle;
import app.organicmaps.sdk.search.SearchRecents;
import app.organicmaps.sdk.search.SearchResult;
import app.organicmaps.sdk.util.Config;
import app.organicmaps.widget.SearchToolbarController;
import java.lang.reflect.Field;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;
import org.junit.Test;
import org.mockito.MockedStatic;

public class SearchQueryProvenanceTest
{
  private static final class StopAtMapSelection extends RuntimeException
  {
  }

  @Test
  public void editedAndRestoredContactSearchDoesNotWriteHistory() throws Exception
  {
    try (MockedStatic<Config> config = mockStatic(Config.class);
         MockedStatic<SearchRecents> history = mockStatic(SearchRecents.class);
         MockedStatic<ArchTaskExecutor> tasks = mockStatic(ArchTaskExecutor.class))
    {
      config.when(Config::isSearchHistoryEnabled).thenReturn(true);
      final ArchTaskExecutor executor = mock(ArchTaskExecutor.class);
      when(executor.isMainThread()).thenReturn(true);
      tasks.when(ArchTaskExecutor::getInstance).thenReturn(executor);
      for (boolean restore : new boolean[] {false, true})
      {
        final SavedStateHandle state = new SavedStateHandle();
        SearchPageViewModel model = new SearchPageViewModel(state);
        model.setCurrentQueryContactDerived(true);
        if (restore)
        {
          model.persistSearchState(true, "123 Main Street", 3, false);
          model = new SearchPageViewModel(state);
          model.setSearchEnabled(
              true, new SearchRequest(model.getPersistedQuery(), null, false, model.getPersistedContactDerived()));
        }
        final SearchFragment fragment = mock(SearchFragment.class, CALLS_REAL_METHODS);
        doReturn(true).when(fragment).isAdded();
        final Class<?> controllerClass = Class.forName(SearchFragment.class.getName() + "$ToolbarController");
        final SearchToolbarController toolbar = (SearchToolbarController) mock(controllerClass, CALLS_REAL_METHODS);
        set(controllerClass, toolbar, "this$0", fragment);
        set(SearchFragment.class, fragment, "mToolbarController", toolbar);
        set(SearchFragment.class, fragment, "mSearchViewModel", model);
        set(SearchFragment.class, fragment, "mHiddenCommands", List.of(mock(HiddenCommand.class)));
        set(SearchFragment.class, fragment, "mSearchDebounceHandler", mock(Handler.class));
        doReturn("123 Main Street ").when(toolbar).getQuery();
        doReturn(false).when(toolbar).isCategory();
        doThrow(new StopAtMapSelection()).when(toolbar).deactivate();
        final Method changed = controllerClass.getDeclaredMethod("onTextChanged", String.class);
        changed.setAccessible(true);
        changed.invoke(toolbar, "123 Main Street ");
        assertTrue(model.isCurrentQueryContactDerived());
        final Method submit = controllerClass.getDeclaredMethod("onStartSearchClick");
        submit.setAccessible(true);
        try
        {
          submit.invoke(toolbar);
          fail("Expected map-selection boundary");
        }
        catch (InvocationTargetException e)
        {
          assertTrue(e.getCause() instanceof StopAtMapSelection);
        }
        assertThrows(StopAtMapSelection.class, () -> fragment.showSingleResultOnMap(mock(SearchResult.class), 0));
        history.verifyNoInteractions();
        model.setSearchEnabled(true, new SearchRequest("cafe", null));
        assertFalse(model.isCurrentQueryContactDerived());
      }
    }
  }

  private static void set(Class<?> type, Object object, String name, Object value) throws Exception
  {
    final Field field = type.getDeclaredField(name);
    field.setAccessible(true);
    field.set(object, value);
  }
  @Test
  public void contactProvenanceSurvivesEditedQueryAndStatePersistence()
  {
    final SavedStateHandle state = new SavedStateHandle();
    final SearchPageViewModel model = new SearchPageViewModel(state);
    model.setCurrentQueryContactDerived(true);
    model.persistSearchState(true, "123 Main Street ", 3, false);
    assertTrue(model.isCurrentQueryContactDerived());
    final SearchPageViewModel restored = new SearchPageViewModel(state);
    assertTrue(restored.getPersistedContactDerived());
    assertTrue(restored.isCurrentQueryContactDerived());
    final SearchRequest request = new SearchRequest(
        restored.getPersistedQuery(), null, restored.getPersistedIsCategory(), restored.getPersistedContactDerived());
    assertTrue(request.isContactDerived);
    assertEquals("123 Main Street ", request.query);
    model.setCurrentQueryContactDerived(false);
    model.persistSearchState(true, "cafe", 3, false);
    assertFalse(new SearchPageViewModel(state).getPersistedContactDerived());
  }
}
