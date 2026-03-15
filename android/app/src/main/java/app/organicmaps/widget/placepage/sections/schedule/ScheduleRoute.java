package app.organicmaps.widget.placepage.sections.schedule;

import androidx.annotation.NonNull;
import java.util.List;

/**
 * Represents a transport route with multiple directions.
 * Example: Route "704" (Bus) with directions to Klusplatz and Volketswil.
 */
public class ScheduleRoute
{
  @NonNull
  public final String routeId;
  @NonNull
  public final String routeNumber;
  @NonNull
  public final String routeName;
  public final int color;
  @NonNull
  public final TransportType transportType;
  @NonNull
  public final List<ScheduleDirection> directions;
  public boolean isPinned;

  public ScheduleRoute(@NonNull String routeId, @NonNull String routeNumber, @NonNull String routeName, int color,
                       @NonNull TransportType transportType, @NonNull List<ScheduleDirection> directions)
  {
    this.routeId = routeId;
    this.routeNumber = routeNumber;
    this.routeName = routeName;
    this.color = color;
    this.transportType = transportType;
    this.directions = directions;
    this.isPinned = false;
  }

  /**
   * Returns the earliest arrival time across all directions.
   */
  public int getEarliestArrivalMinutes()
  {
    int earliest = Integer.MAX_VALUE;
    for (ScheduleDirection direction : directions)
    {
      if (!direction.arrivals.isEmpty())
      {
        String time = direction.getFirstArrival().arrivalTime;
        int minutes = parseMinutes(time);
        if (minutes < earliest)
          earliest = minutes;
      }
    }
    return earliest == Integer.MAX_VALUE ? 0 : earliest;
  }

  private int parseMinutes(String timeStr)
  {
    try
    {
      return Integer.parseInt(timeStr.replaceAll("[^0-9]", ""));
    }
    catch (NumberFormatException e)
    {
      return 0;
    }
  }
}
