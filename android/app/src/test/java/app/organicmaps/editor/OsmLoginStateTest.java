package app.organicmaps.editor;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.doAnswer;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.arch.core.executor.ArchTaskExecutor;
import androidx.arch.core.executor.TaskExecutor;
import androidx.core.view.ViewCompat;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.LifecycleRegistry;
import androidx.lifecycle.Observer;
import app.organicmaps.R;
import app.organicmaps.widget.ToolbarController;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.MockedStatic;

public class OsmLoginStateTest
{
  @Before
  public void runLiveDataOnTestThread()
  {
    ArchTaskExecutor.getInstance().setDelegate(new TaskExecutor() {
      @Override
      public void executeOnDiskIO(Runnable runnable)
      {
        runnable.run();
      }

      @Override
      public void postToMainThread(Runnable runnable)
      {
        runnable.run();
      }

      @Override
      public boolean isMainThread()
      {
        return true;
      }
    });
  }

  @After
  public void restoreExecutor()
  {
    ArchTaskExecutor.getInstance().setDelegate(null);
  }

  @Test
  public void retryClearsTheSavedErrorAndShowsProgress()
  {
    Fixture fixture = new Fixture();
    fixture.show(savedError());
    fixture.fragment.onAuthStarted();
    Bundle saved = mock(Bundle.class);
    fixture.fragment.onSaveInstanceState(saved);

    verify(saved).putString("state_error_text", null);
    verify(fixture.error).setVisibility(View.GONE);
    verify(fixture.progress).setVisibility(View.VISIBLE);
    verify(fixture.login).setEnabled(false);
  }

  @Test
  public void oauthExchangeShowsProgressBeforeSchedulingWork()
  {
    Fixture fixture = new Fixture();
    fixture.withOAuthCode();
    AtomicBoolean progressVisible = new AtomicBoolean();
    doAnswer(call -> {
      progressVisible.set(call.getArgument(0, Integer.class) == View.VISIBLE);
      return null;
    })
        .when(fixture.progress)
        .setVisibility(anyInt());
    doAnswer(call -> {
      assertTrue(progressVisible.get());
      return null;
    })
        .when(fixture.worker)
        .execute(any());
    fixture.show(null);

    verify(fixture.login, atLeastOnce()).setEnabled(false);
    verify(fixture.worker).execute(any());
  }

  @Test
  public void recreationRestoresTheErrorWithoutReplayingTheCode()
  {
    Fixture fixture = new Fixture();
    fixture.withOAuthCode();
    fixture.show(savedError());

    verify(fixture.error).setText("Expired login");
    verify(fixture.error).setVisibility(View.VISIBLE);
    verify(fixture.worker, never()).execute(any());
    verify(fixture.login, never()).setEnabled(false);
  }

  @Test
  public void recreatedScreenKeepsProgressAndClosesWhenTheExchangeSucceeds()
  {
    Fixture original = new Fixture();
    original.withOAuthCode();
    original.show(null);
    original.lifecycle.setCurrentState(Lifecycle.State.DESTROYED);

    Fixture recreated = new Fixture(original);
    recreated.withOAuthCode();
    recreated.show(mock(Bundle.class));
    verify(recreated.login).setEnabled(false);
    verify(recreated.progress).setVisibility(View.VISIBLE);
    verify(recreated.worker, times(1)).execute(any());

    original.completeAuthorization();
    verify(recreated.activity).finish();
    verify(original.activity, never()).finish();
  }

  @Test
  public void handledOAuthFailureDoesNotReplaceANewerPasswordError()
  {
    Fixture original = new Fixture(OsmOAuthViewModel.State.FAILED);
    original.withOAuthCode();
    original.show(null);
    original.completeAuthorization();
    assertNull(original.model.getState().getValue());
    original.fragment.onAuthFail("Incorrect password");
    Bundle saved = mock(Bundle.class);
    original.fragment.onSaveInstanceState(saved);
    ArgumentCaptor<String> error = ArgumentCaptor.forClass(String.class);
    verify(saved).putString(eq("state_error_text"), error.capture());
    when(saved.getString("state_error_text")).thenReturn(error.getValue());
    original.lifecycle.setCurrentState(Lifecycle.State.DESTROYED);

    Fixture recreated = new Fixture(original);
    recreated.withOAuthCode();
    recreated.show(saved);
    verify(recreated.error).setText("Incorrect password");
    verify(recreated.error, never()).setText("Localized login error");
    verify(recreated.worker, times(1)).execute(any());
  }

  @Test
  public void resultSurvivesUntilConsumedWithoutReusingTheCode()
  {
    for (OsmOAuthViewModel.State result : List.of(OsmOAuthViewModel.State.SUCCEEDED, OsmOAuthViewModel.State.FAILED))
    {
      ExecutorService worker = mock(ExecutorService.class);
      AtomicReference<String> request = new AtomicReference<>();
      OsmOAuthViewModel model = new OsmOAuthViewModel(code -> {
        request.set(code);
        return result;
      }, worker);
      model.authorize("oauth-code");
      model.consumeResult();
      assertEquals(OsmOAuthViewModel.State.AUTHORIZING, model.getState().getValue());
      model.authorize("oauth-code");
      ArgumentCaptor<Runnable> task = ArgumentCaptor.forClass(Runnable.class);
      verify(worker, times(1)).execute(task.capture());
      task.getValue().run();
      assertEquals("oauth-code", request.get());

      AtomicReference<OsmOAuthViewModel.State> restored = new AtomicReference<>();
      Observer<OsmOAuthViewModel.State> observer = restored::set;
      model.getState().observeForever(observer);
      try
      {
        assertEquals(result, restored.get());
        model.consumeResult();
        assertNull(restored.get());
        model.authorize("oauth-code");
        verify(worker, times(1)).execute(any());
      }
      finally
      {
        model.getState().removeObserver(observer);
      }
    }
  }

  private static Bundle savedError()
  {
    Bundle saved = mock(Bundle.class);
    when(saved.getString("state_error_text")).thenReturn("Expired login");
    return saved;
  }

  private static class Fixture
  {
    final TestLoginFragment fragment = mock(TestLoginFragment.class, CALLS_REAL_METHODS);
    final View root = mock(View.class);
    final TextView error = mock(TextView.class);
    final Button login = mock(Button.class);
    final ProgressBar progress = mock(ProgressBar.class);
    final FragmentActivity activity = mock(FragmentActivity.class);
    final OsmOAuthViewModel model;
    final ExecutorService worker;
    final LifecycleOwner owner = mock(LifecycleOwner.class);
    final LifecycleRegistry lifecycle = new LifecycleRegistry(owner);

    Fixture()
    {
      this(OsmOAuthViewModel.State.SUCCEEDED);
    }

    Fixture(OsmOAuthViewModel.State result)
    {
      worker = mock(ExecutorService.class);
      model = new OsmOAuthViewModel(code -> result, worker);
      bindViews();
    }

    Fixture(Fixture original)
    {
      worker = original.worker;
      model = original.model;
      bindViews();
    }

    private void bindViews()
    {
      fragment.model = model;
      doReturn("Localized login error").when(fragment).getString(R.string.osm_login_error);
      fragment.toolbar = mock(ToolbarController.class);
      doReturn(true).when(fragment).isAdded();
      doReturn(activity).when(fragment).requireActivity();
      when(activity.getIntent()).thenReturn(mock(Intent.class));
      when(owner.getLifecycle()).thenReturn(lifecycle);
      lifecycle.setCurrentState(Lifecycle.State.STARTED);
      doReturn(owner).when(fragment).getViewLifecycleOwner();
      when(root.findViewById(R.id.tv_error)).thenReturn(error);
      when(root.findViewById(R.id.login)).thenReturn(login);
      when(root.findViewById(R.id.osm_login_progress)).thenReturn(progress);
      when(root.findViewById(R.id.register)).thenReturn(mock(Button.class));
      when(root.findViewById(R.id.scrollView)).thenReturn(mock(ScrollView.class));
    }

    void completeAuthorization()
    {
      ArgumentCaptor<Runnable> task = ArgumentCaptor.forClass(Runnable.class);
      verify(worker).execute(task.capture());
      task.getValue().run();
    }

    void withOAuthCode()
    {
      Bundle arguments = mock(Bundle.class);
      when(arguments.getString(OsmLoginActivity.EXTRA_OAUTH2CODE)).thenReturn("oauth-code");
      doReturn(arguments).when(fragment).getArguments();
    }

    void show(Bundle saved)
    {
      try (MockedStatic<ViewCompat> insets = mockStatic(ViewCompat.class))
      {
        fragment.onViewCreated(root, saved);
      }
    }
  }

  private static class TestLoginFragment extends BaseOsmLoginFragment
  {
    ToolbarController toolbar;
    OsmOAuthViewModel model;

    @NonNull
    @Override
    protected ToolbarController onCreateToolbarController(@NonNull View root)
    {
      return toolbar;
    }

    @NonNull
    @Override
    protected OsmOAuthViewModel getOAuthViewModel()
    {
      return model;
    }

    @Override
    protected void setupLogin(@NonNull View view)
    {}
  }
}
