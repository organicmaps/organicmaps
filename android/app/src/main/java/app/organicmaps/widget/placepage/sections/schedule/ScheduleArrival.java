package app.organicmaps.widget.placepage.sections.schedule;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

/**
 * Represents a single arrival time for a transport route.
 */
public class ScheduleArrival
{
  @NonNull
  public final String arrivalTime;
  @Nullable
  public final String scheduledTime;
  @NonNull
  public final ArrivalStatus status;
  public final boolean isDynamic;

  public ScheduleArrival(@NonNull String arrivalTime, boolean isDynamic)
  {
    this(arrivalTime, null, ArrivalStatus.ON_TIME, isDynamic);
  }

  public ScheduleArrival(@NonNull String arrivalTime, @Nullable String scheduledTime, @NonNull ArrivalStatus status,
                         boolean isDynamic)
  {
    this.arrivalTime = arrivalTime;
    this.scheduledTime = scheduledTime;
    this.status = status;
    this.isDynamic = isDynamic;
  }
}
