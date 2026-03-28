package app.organicmaps.sdk.editor.data;

import androidx.annotation.IntRange;
import androidx.annotation.Keep;

// Called from JNI.
@Keep
@SuppressWarnings("unused")
public class OpeningHoursInfo
{
  public enum RuleState
  {
    Open,
    Closed,
    Unknown,
  }

  public OpeningHoursInfo(@IntRange(from = 0, to = 2) int state, boolean isTwentyFourSeven, long nextTimeOpen,
                          long nextTimeClosed)
  {
    switch (state)
    {
    case 0: this.state = RuleState.Open; break;
    case 1: this.state = RuleState.Closed; break;
    case 2: this.state = RuleState.Unknown; break;
    default: this.state = RuleState.Unknown; assert false;
    }

    this.nextTimeOpen = nextTimeOpen;
    this.nextTimeClosed = nextTimeClosed;
    this.isTwentyFourSeven = isTwentyFourSeven;
  }

  public final RuleState state;
  public final long nextTimeOpen;
  public final long nextTimeClosed;
  public final boolean isTwentyFourSeven;
}
