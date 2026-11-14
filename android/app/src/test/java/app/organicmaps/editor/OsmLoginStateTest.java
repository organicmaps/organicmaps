package app.organicmaps.editor;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.CALLS_REAL_METHODS;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.mockStatic;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.core.view.ViewCompat;
import app.organicmaps.R;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import app.organicmaps.widget.ToolbarController;
import java.util.concurrent.ExecutorService;
import org.junit.Test;
import org.mockito.InOrder;
import org.mockito.MockedStatic;

public class OsmLoginStateTest
{
  @Test
  public void savedErrorIsShownAfterRecreation()
  {
    Fixture fixture = new Fixture();
    fixture.show(savedError());

    verify(fixture.error).setText("Expired login");
    verify(fixture.error).setVisibility(View.VISIBLE);
    verify(fixture.login).setEnabled(true);
    verify(fixture.progress).setVisibility(View.GONE);
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
    Bundle arguments = mock(Bundle.class);
    when(arguments.getString(OsmLoginActivity.EXTRA_OAUTH2CODE)).thenReturn("oauth-code");
    doReturn(arguments).when(fixture.fragment).getArguments();
    ExecutorService worker = mock(ExecutorService.class);

    try (MockedStatic<ThreadPool> pool = mockStatic(ThreadPool.class))
    {
      pool.when(ThreadPool::getWorker).thenReturn(worker);
      fixture.show(savedError());
    }

    InOrder order = inOrder(fixture.login, fixture.progress, worker);
    order.verify(fixture.login).setEnabled(true);
    order.verify(fixture.progress).setVisibility(View.GONE);
    order.verify(fixture.login).setEnabled(false);
    order.verify(fixture.progress).setVisibility(View.VISIBLE);
    order.verify(worker).execute(any(Runnable.class));
    Bundle saved = mock(Bundle.class);
    fixture.fragment.onSaveInstanceState(saved);
    verify(saved).putString("state_error_text", null);
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

    Fixture()
    {
      fragment.toolbar = mock(ToolbarController.class);
      doReturn(true).when(fragment).isAdded();
      when(root.findViewById(R.id.tv_error)).thenReturn(error);
      when(root.findViewById(R.id.login)).thenReturn(login);
      when(root.findViewById(R.id.osm_login_progress)).thenReturn(progress);
      when(root.findViewById(R.id.register)).thenReturn(mock(Button.class));
      when(root.findViewById(R.id.scrollView)).thenReturn(mock(ScrollView.class));
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

    @NonNull
    @Override
    protected ToolbarController onCreateToolbarController(@NonNull View root)
    {
      return toolbar;
    }

    @Override
    protected void setupLogin(@NonNull View view)
    {}
  }
}
