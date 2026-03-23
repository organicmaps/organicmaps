package app.organicmaps.widget.placepage.sections.schedule;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import java.util.List;

/**
 * Represents a section in the schedule list (e.g., "Pinned Bus Lines").
 * If title is null, no header is shown for this section.
 */
public class ScheduleSection
{
  @Nullable
  public final String title;
  @Nullable
  public final String walkTime;
  @NonNull
  public final List<ScheduleRoute> routes;

  public ScheduleSection(@NonNull List<ScheduleRoute> routes)
  {
    this(null, null, routes);
  }

  public ScheduleSection(@Nullable String title, @NonNull List<ScheduleRoute> routes)
  {
    this(title, null, routes);
  }

  public ScheduleSection(@Nullable String title, @Nullable String walkTime, @NonNull List<ScheduleRoute> routes)
  {
    this.title = title;
    this.walkTime = walkTime;
    this.routes = routes;
  }

  public boolean hasTitle()
  {
    return title != null && !title.isEmpty();
  }
}
