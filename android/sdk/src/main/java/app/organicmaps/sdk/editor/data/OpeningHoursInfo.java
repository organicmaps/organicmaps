package app.organicmaps.sdk.editor.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class OpeningHoursInfo
{
  // Used for nextTimeOpen and nextTimeClosed when the place will stay open (or closed) forever
  public static final long TIME_NEVER = -1;

  public enum RuleState
  {
    Open,
    Closed,
  }

  public OpeningHoursInfo(@IntRange(from = 0, to = 2) int state, boolean isTwentyFourSeven, long nextTimeOpen,
                          long nextTimeClosed, int utcOffsetNowSeconds, int utcOffsetNextSeconds)
  {
    switch (state)
    {
    case 0: this.state = RuleState.Open; break;
    case 1: this.state = RuleState.Closed; break;
    default: this.state = RuleState.Closed; assert false;
    }

    this.nextTimeOpen = nextTimeOpen;
    this.nextTimeClosed = nextTimeClosed;
    this.isTwentyFourSeven = isTwentyFourSeven;
    this.utcOffsetNowSeconds = utcOffsetNowSeconds;
    this.utcOffsetNextSeconds = utcOffsetNextSeconds;
  }

  public final RuleState state;
  public final long nextTimeOpen; // Is TIME_NEVER if currently closed but never opens
  public final long nextTimeClosed; // Is TIME_NEVER if currently open but never closes
  public final boolean isTwentyFourSeven;
  // The UTC offset may change before the next opening-hours transition.
  public final int utcOffsetNowSeconds;
  public final int utcOffsetNextSeconds;
}
