package app.organicmaps;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.res.Resources;
import java.util.Locale;
public final class AccessibilityTestContext
{
  private AccessibilityTestContext() {}
  public static Context wrap(Context context)
  {
    Resources original = context.getResources();
    Resources resources =
        new Resources(original.getAssets(), original.getDisplayMetrics(), original.getConfiguration()) {
          @Override
          public String getString(int id)
          {
            if (id == R.string.navigation_controls)
              return "Navigation controls";
            if (id == R.string.navigation_controls_expand)
              return "Expand navigation controls";
            if (id == R.string.navigation_controls_collapse)
              return "Collapse navigation controls";
            if (id == R.string.navigation_controls_expanded)
              return "Expanded";
            if (id == R.string.navigation_controls_collapsed)
              return "Collapsed";
            if (id == R.string.navigation_current_speed)
              return "Current speed: %1$s %2$s";
            if (id == R.string.navigation_speed_limit)
              return "Speed limit: %1$s %2$s";
            if (id == R.string.navigation_speed_warning)
              return "Speed warning";
            if (id == R.string.navigation_speed_alert)
              return "Speed alert";
            if (id == R.string.navigation_time_remaining)
              return "Time remaining: %s";
            if (id == R.string.navigation_arrival_time)
              return "Arrival: %s";
            if (id == R.string.navigation_distance_remaining)
              return "Distance remaining: %s";
            if (id == R.string.bookmark_actions_named)
              return "Actions for %s";
            if (id == R.string.bookmark_change_color_named)
              return "Change color of %s";
            if (id == R.string.bookmark_hide_list_named)
              return "Hide list %s";
            if (id == R.string.bookmark_show_list_named)
              return "Show list %s";
            if (id == R.string.bookmark_hide_track_named)
              return "Hide track %s";
            if (id == R.string.bookmark_show_track_named)
              return "Show track %s";
            return super.getString(id);
          }
          @Override
          public String getString(int id, Object... args)
          {
            return String.format(Locale.US, getString(id), args);
          }
        };
    return new ContextWrapper(context) {
      @Override
      public Resources getResources()
      {
        return resources;
      }
    };
  }
}
