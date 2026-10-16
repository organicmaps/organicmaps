package app.organicmaps.widget.placepage;

import androidx.annotation.Nullable;
import app.organicmaps.sdk.bookmarks.data.ElevationInfo;
import java.util.concurrent.Executor;
import java.util.concurrent.FutureTask;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.function.BiFunction;
import java.util.function.BooleanSupplier;
import java.util.function.Consumer;

/** Owns visible recording-chart requests. All state transitions run on the UI executor. */
public final class RecordingElevationProfileLoader
{
  private final Executor mWorker;
  private final Executor mUi;
  private final BooleanSupplier mIsVisible;
  private final BiFunction<ElevationInfo, AtomicBoolean, ElevationInfo> mLoad;
  private final Consumer<ElevationInfo> mDisplay;
  @Nullable
  private ElevationInfo mData;
  @Nullable
  private FutureTask<?> mTask;
  @Nullable
  private AtomicBoolean mCancellation;
  private boolean mVisible;
  private boolean mDirty = true;

  public RecordingElevationProfileLoader(Executor worker, Executor ui, BooleanSupplier isVisible,
                                         BiFunction<ElevationInfo, AtomicBoolean, ElevationInfo> load,
                                         Consumer<ElevationInfo> display)
  {
    mWorker = worker;
    mUi = ui;
    mIsVisible = isVisible;
    mLoad = load;
    mDisplay = display;
  }

  public void invalidate()
  {
    mDirty = true;
  }

  public void setVisible(boolean visible)
  {
    mVisible = visible && mIsVisible.getAsBoolean();
    if (!mVisible)
    {
      if (mCancellation != null)
        mCancellation.set(true);
      if (mTask != null)
      {
        mTask.cancel(false);
        mTask = null;
        mDirty = true;
      }
      mCancellation = null;
      return;
    }
    requestIfNeeded();
  }

  private void requestIfNeeded()
  {
    if (!mVisible || !mIsVisible.getAsBoolean() || !mDirty || mTask != null)
      return;
    mDirty = false;
    AtomicBoolean cancellation = new AtomicBoolean();
    mCancellation = cancellation;
    ElevationInfo previous = mData;
    mTask = new FutureTask<>(() -> {
      ElevationInfo data = mLoad.apply(previous, cancellation);
      mUi.execute(() -> {
        if (mCancellation != cancellation || cancellation.get())
          return;
        mTask = null;
        mCancellation = null;
        if (!mVisible || !mIsVisible.getAsBoolean())
        {
          mDirty = true;
          return;
        }
        if (data != mData)
        {
          mData = data;
          mDisplay.accept(data);
        }
        requestIfNeeded();
      });
    }, null);
    mWorker.execute(mTask);
  }
}
