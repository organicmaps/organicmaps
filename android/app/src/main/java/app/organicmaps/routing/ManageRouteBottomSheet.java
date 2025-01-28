package app.organicmaps.routing;

import android.app.Dialog;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.Framework;
import app.organicmaps.R;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.divider.MaterialDividerItemDecoration;

public class ManageRouteBottomSheet extends BottomSheetDialogFragment
    implements View.OnClickListener
{
  ManageRouteAdapter mManageRouteAdapter;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View v = inflater.inflate(R.layout.manage_route_bottom_sheet, container, false);

    Button cancelButton = v.findViewById(R.id.btn__cancel);
    cancelButton.setOnClickListener(this);

    Button planButton = v.findViewById(R.id.btn__plan);
    planButton.setOnClickListener(this);

    RecyclerView manageRouteList = v.findViewById(R.id.manage_route_list);
    LinearLayoutManager layoutManager = new LinearLayoutManager(getContext());
    manageRouteList.setLayoutManager(layoutManager);
    RecyclerView.ItemDecoration decoration = new MaterialDividerItemDecoration(getContext(),
                                                                               layoutManager.getOrientation());
    manageRouteList.addItemDecoration(decoration);

    mManageRouteAdapter = new ManageRouteAdapter(getContext(), Framework.nativeGetRoutePoints());
    manageRouteList.setAdapter(mManageRouteAdapter);

    // Enable drag & drop in route list.
    ItemTouchHelper touchHelper = new ItemTouchHelper(
      new ItemTouchHelper.Callback()
      {
        @Override
        public int getMovementFlags(@NonNull RecyclerView recyclerView,
                                    @NonNull RecyclerView.ViewHolder viewHolder)
        {
          return makeMovementFlags(ItemTouchHelper.UP | ItemTouchHelper.DOWN, 0);
        }

        @Override
        public boolean onMove(@NonNull RecyclerView recyclerView,
                              @NonNull RecyclerView.ViewHolder viewHolder,
                              @NonNull RecyclerView.ViewHolder target)
        {
          mManageRouteAdapter.moveRoutePoint(viewHolder.getAbsoluteAdapterPosition(),
                                             target.getAbsoluteAdapterPosition());

          return true;
        }

        @Override
        public void onSwiped(@NonNull RecyclerView.ViewHolder viewHolder, int direction)
        {

        }
      });

    touchHelper.attachToRecyclerView(manageRouteList);

    return v;
  }

  @Override
  public void onClick(View v)
  {
    int buttonId = v.getId();

    if (buttonId == R.id.btn__cancel)
      dismiss();
    else if (buttonId == R.id.btn__plan)
    {
      // Remove all existing route points.
      Framework.nativeRemoveRoutePoints();

      // Get route points from adapter.
      RouteMarkData[] routePoints = mManageRouteAdapter.getRoutePoints();
      
      // Add all updated route points.
      for (RouteMarkData point :routePoints)
      {
        // Add each new route point.
        Framework.nativeAddRoutePoint(point.mTitle, point.mSubtitle, point.mPointType,
                                      point.mIntermediateIndex, point.mIsMyPosition,
                                      point.mLat, point.mLon);

        RoutingController.get().launchPlanning();

        dismiss();
      }
    }
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    Dialog dialog = super.onCreateDialog(savedInstanceState);

    // Expand bottom sheet dialog.
    dialog.setOnShowListener(dialogInterface -> {

      FrameLayout bottomSheet = ((BottomSheetDialog) dialogInterface).findViewById(
          com.google.android.material.R.id.design_bottom_sheet);
      if (bottomSheet != null)
        BottomSheetBehavior.from(bottomSheet).setState(BottomSheetBehavior.STATE_EXPANDED);
    });

    // Set key listener to detect back button pressed.
    dialog.setOnKeyListener((dialog1, keyCode, event) -> {
      if (keyCode == KeyEvent.KEYCODE_BACK && event.getAction() == KeyEvent.ACTION_UP)
      {
        // Dismiss the fragment
        dismiss();
        return true;
      }
      // Otherwise, do nothing else.
      return false;
    });

    return dialog;
  }
}
