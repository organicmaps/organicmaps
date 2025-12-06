package app.organicmaps.sdk.editor.data;

import androidx.annotation.Keep;
import androidx.annotation.NonNull;

/**
 * Information about the current state of opening hours.
 * Called from JNI.
 */
@Keep
@SuppressWarnings("unused")
public class OpeningHoursInfo
{

  public enum State
  {
    OPEN,
    CLOSED,
    UNKNOWN
  }

  @NonNull
  private final State mState;
  private final long mNextTimeOpen;
  private final long mNextTimeClosed;
  private final boolean mIsHoliday;

  public OpeningHoursInfo(@NonNull State state, long nextTimeOpen, long nextTimeClosed, boolean isHoliday)
  {
    mState = state;
    mNextTimeOpen = nextTimeOpen;
    mNextTimeClosed = nextTimeClosed;
    mIsHoliday = isHoliday;
  }

  @NonNull
  public State getState()
  {
    return mState;
  }

  public long getNextTimeOpen()
  {
    return mNextTimeOpen;
  }

  public long getNextTimeClosed()
  {
    return mNextTimeClosed;
  }

  public boolean isHoliday()
  {
    return mIsHoliday;
  }
}

