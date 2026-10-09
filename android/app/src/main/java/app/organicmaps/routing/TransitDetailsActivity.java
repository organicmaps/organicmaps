package app.organicmaps.routing;

import android.app.Activity;
import android.content.Intent;
import androidx.annotation.NonNull;
import androidx.fragment.app.Fragment;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragmentActivity;

/**
 * Full-screen host for the transit route details ({@link TransitDetailsFragment}). Pushed on top of the
 * routing panel with a slide-in-from-the-right transition when the transit summary row is tapped.
 */
public class TransitDetailsActivity extends BaseMwmFragmentActivity
{
  @Override
  protected Class<? extends Fragment> getFragmentClass()
  {
    return TransitDetailsFragment.class;
  }

  public static void start(@NonNull Activity activity)
  {
    activity.startActivity(new Intent(activity, TransitDetailsActivity.class));
    applyOpenTransition(activity);
  }

  @Override
  public void finish()
  {
    super.finish();
    applyCloseTransition(this);
  }

  @SuppressWarnings("deprecation") // overrideActivityTransition requires API 34; keep parity down to minSdk.
  private static void applyOpenTransition(@NonNull Activity activity)
  {
    activity.overridePendingTransition(R.anim.slide_in_right, R.anim.slide_out_left);
  }

  @SuppressWarnings("deprecation")
  private static void applyCloseTransition(@NonNull Activity activity)
  {
    activity.overridePendingTransition(R.anim.slide_in_left, R.anim.slide_out_right);
  }
}
