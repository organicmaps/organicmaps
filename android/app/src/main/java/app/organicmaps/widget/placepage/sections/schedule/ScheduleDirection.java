package app.organicmaps.widget.placepage.sections.schedule;

import androidx.annotation.NonNull;
import java.util.List;

/**
 * Represents a direction within a route, containing multiple arrival times.
 * Example: "To Klusplatz" with arrivals [16 min, 46 min]
 */
public class ScheduleDirection
{
  @NonNull
  public final String destination;
  @NonNull
  public final List<ScheduleArrival> arrivals;

  public ScheduleDirection(@NonNull String destination, @NonNull List<ScheduleArrival> arrivals)
  {
    this.destination = destination;
    this.arrivals = arrivals;
  }

  /**
   * Returns the first arrival (soonest) for this direction, or null if no arrivals.
   */
  @NonNull
  public ScheduleArrival getFirstArrival()
  {
    if (arrivals.isEmpty())
      throw new IllegalStateException("Direction must have at least one arrival");
    return arrivals.get(0);
  }

  /**
   * Returns true if any arrival in this direction has dynamic (real-time) data.
   */
  public boolean hasDynamicData()
  {
    for (ScheduleArrival arrival : arrivals)
    {
      if (arrival.isDynamic)
        return true;
    }
    return false;
  }
}
