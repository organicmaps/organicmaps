package app.organicmaps.settings;

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragmentActivity;
import app.organicmaps.sdk.Router;
import app.organicmaps.sdk.routing.RoutingOptions;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import java.util.ArrayList;
import java.util.List;

public class DrivingOptionsActivity extends BaseMwmFragmentActivity
{
  static final String EXTRA_ROUTER = "router_type";

  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return DrivingOptionsFragment.class;
  }

  public static void start(@NonNull Activity activity, @NonNull Router router)
  {
    activity.startActivity(new Intent(activity, DrivingOptionsActivity.class).putExtra(EXTRA_ROUTER, router.getType()));
  }

  @StringRes
  static int title(@NonNull Router router)
  {
    return switch (router)
    {
      case Vehicle -> R.string.routing_options_driving;
      case Bicycle -> R.string.routing_options_cycling;
      case Pedestrian -> R.string.routing_options_walking;
      case Transit -> R.string.driving_options_title;
      default -> throw new IllegalArgumentException("No routing options for " + router);
    };
  }

  public static void chooseProfile(@NonNull Activity activity)
  {
    List<Router> profiles = new ArrayList<>();
    List<CharSequence> titles = new ArrayList<>();
    for (Router router : Router.values())
      if (RoutingOptions.hasSupportedOptions(router))
      {
        profiles.add(router);
        titles.add(activity.getString(title(router)));
      }
    new MaterialAlertDialogBuilder(activity)
        .setTitle(R.string.driving_options_title)
        .setItems(titles.toArray(new CharSequence[0]), (dialog, position) -> start(activity, profiles.get(position)))
        .setNegativeButton(R.string.cancel, null)
        .show();
  }
}
