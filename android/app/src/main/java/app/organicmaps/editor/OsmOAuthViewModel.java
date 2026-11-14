package app.organicmaps.editor;

import androidx.annotation.MainThread;
import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.ViewModel;
import app.organicmaps.sdk.editor.OsmOAuth;
import app.organicmaps.sdk.util.concurrency.ThreadPool;
import java.util.concurrent.Executor;
import java.util.function.Function;

public class OsmOAuthViewModel extends ViewModel
{
  enum State
  {
    AUTHORIZING,
    SUCCEEDED,
    FAILED
  }

  private final MutableLiveData<State> mState = new MutableLiveData<>();
  private final Function<String, State> mAuthorize;
  private final Executor mWorker;
  private boolean mStarted;

  public OsmOAuthViewModel()
  {
    this(OsmOAuthViewModel::exchangeCode, ThreadPool.getWorker());
  }

  @VisibleForTesting
  OsmOAuthViewModel(@NonNull Function<String, State> authorize, @NonNull Executor worker)
  {
    mAuthorize = authorize;
    mWorker = worker;
  }

  @NonNull
  LiveData<State> getState()
  {
    return mState;
  }

  @MainThread
  void authorize(@NonNull String code)
  {
    // OAuth codes are single-use; a recreated view observes the existing request.
    if (mStarted)
      return;

    mStarted = true;
    mState.setValue(State.AUTHORIZING);
    mWorker.execute(() -> mState.postValue(mAuthorize.apply(code)));
  }

  @MainThread
  void consumeResult()
  {
    State state = mState.getValue();
    if (state == State.SUCCEEDED || state == State.FAILED)
      mState.setValue(null);
  }

  @NonNull
  private static State exchangeCode(@NonNull String code)
  {
    final String token = OsmOAuth.nativeAuthWithOAuth2Code(code);
    if (token == null)
      return State.FAILED;

    final String username = OsmOAuth.nativeGetOsmUsername(token);
    OsmOAuth.setAuthorization(token, username);
    return State.SUCCEEDED;
  }
}
