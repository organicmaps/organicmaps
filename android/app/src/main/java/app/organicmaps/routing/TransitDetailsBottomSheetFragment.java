package app.organicmaps.routing;

import android.os.Bundle;
import android.transition.ChangeBounds;
import android.transition.TransitionManager;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewParent;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;

/**
 * Bottom sheet that slides up to reveal the per-leg breakdown of a public transport route.
 * Triggered by tapping the inline transit summary strip in the routing plan panel.
 *
 * Data is sourced from {@link RoutingController#getCachedTransitInfo()} on each open so the sheet
 * stays in sync with the latest planned route without needing to be (re)constructed with arguments.
 */
public class TransitDetailsBottomSheetFragment extends BottomSheetDialogFragment
{
  public static final String TAG = TransitDetailsBottomSheetFragment.class.getSimpleName();

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_transit_details_sheet, container, false);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BottomSheetBehavior<View> behavior = BottomSheetBehavior.from((View) requireView().getParent());
    behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    behavior.setSkipCollapsed(true);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
    if (info == null)
    {
      dismiss();
      return;
    }

    RecyclerView recycler = view.findViewById(R.id.transit_details_recycler);
    recycler.setLayoutManager(new LinearLayoutManager(requireContext()));
    // No item animator: the sheet's ChangeBounds owns all motion. Leaving it on lets the rows below
    // run their own move animation, which fights the transition and makes them bounce.
    recycler.setItemAnimator(null);
    TransitDetailsAdapter adapter = new TransitDetailsAdapter();
    recycler.setAdapter(adapter);
    adapter.setOnBeforeToggleListener(() -> beginSheetTransition(view));
    adapter.setItems(info.getTransitSteps());
  }

  /**
   * Runs a {@link ChangeBounds} on the sheet's coordinator so the {@link BottomSheetBehavior}
   * reposition tweens smoothly on the next layout. ChangeBounds (rather than AutoTransition) animates
   * the height and row positions without fading the revealed stops in.
   */
  private static void beginSheetTransition(@NonNull View content)
  {
    ViewParent sheet = content.getParent();
    ViewParent coordinator = sheet == null ? null : sheet.getParent();
    if (coordinator instanceof ViewGroup group)
    {
      ChangeBounds transition = new ChangeBounds();
      transition.setDuration(200);
      TransitionManager.beginDelayedTransition(group, transition);
    }
  }
}
