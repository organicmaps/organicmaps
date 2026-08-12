package app.organicmaps.routing;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.ViewCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.routing.TransitRouteInfo;
import app.organicmaps.util.WindowInsetUtils.PaddingInsetsListener;
import app.organicmaps.widget.ToolbarController;

/**
 * Full-screen per-leg transit route breakdown, reached by tapping the transit summary row. Hosts the
 * same {@link TransitDetailsAdapter} timeline the summary used to expand inline; the route steps are
 * re-read from the cached transit info rather than passed in, so no marshaling is needed.
 */
public class TransitDetailsFragment extends BaseMwmToolbarFragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_transit_details, container, false);

    RecyclerView recycler = root.findViewById(R.id.transit_details_recycler);
    recycler.setLayoutManager(new LinearLayoutManager(requireContext()));
    // A ride row animates its own intermediate-stops height when toggled; the default change animation
    // would fight that, so disable it (mirrors the old inline breakdown setup).
    recycler.setItemAnimator(null);
    TransitDetailsAdapter adapter = new TransitDetailsAdapter();
    recycler.setAdapter(adapter);
    // Pad the scrolling list past the nav bar (bottom) and the display cutout in landscape (left/right);
    // the toolbar above already consumes the top inset. Ctor order is (top, bottom, left, right).
    ViewCompat.setOnApplyWindowInsetsListener(recycler, new PaddingInsetsListener(false, true, true, true));

    final TransitRouteInfo info = RoutingController.get().getCachedTransitInfo();
    if (info == null)
    {
      // The route was cancelled/rebuilt out from under us (e.g. process restore). Nothing to show.
      requireActivity().finish();
      return root;
    }
    adapter.setItems(info.getTransitSteps());

    return root;
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.transit_details_title);
  }

  @NonNull
  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    // The map activity is always directly below, so the up arrow just finishes this screen (a clean
    // slide-back pop) instead of relaunching the parent via the default up-navigation.
    return new ToolbarController(root, requireActivity()) {
      @Override
      public void onUpClick()
      {
        requireActivity().finish();
      }
    };
  }
}
