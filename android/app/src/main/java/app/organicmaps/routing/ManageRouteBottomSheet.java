package app.organicmaps.routing;

import static androidx.recyclerview.widget.ItemTouchHelper.*;

import android.app.Dialog;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.FrameLayout;
import android.widget.ImageView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.util.UiUtils;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialog;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import com.google.android.material.divider.MaterialDividerItemDecoration;
import java.util.ArrayList;

public class ManageRouteBottomSheet
    extends BottomSheetDialogFragment implements View.OnClickListener, ManageRouteAdapter.ManageRouteListener
{
  ManageRouteAdapter mManageRouteAdapter;
  ItemTouchHelper mTouchHelper;
  ImageView mMyLocationImageView;

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View v = inflater.inflate(R.layout.manage_route_bottom_sheet, container, false);

    Button cancelButton = v.findViewById(R.id.btn__cancel);
    cancelButton.setOnClickListener(this);

    Button planButton = v.findViewById(R.id.btn__plan);
    planButton.setOnClickListener(this);

    mMyLocationImageView = v.findViewById(R.id.image_my_location);
    mMyLocationImageView.setOnClickListener(this);

    RecyclerView manageRouteList = v.findViewById(R.id.manage_route_list);
    LinearLayoutManager layoutManager = new LinearLayoutManager(getContext());
    manageRouteList.setLayoutManager(layoutManager);
    RecyclerView.ItemDecoration decoration =
        new MaterialDividerItemDecoration(getContext(), layoutManager.getOrientation());
    manageRouteList.addItemDecoration(decoration);

    mManageRouteAdapter = new ManageRouteAdapter(getContext(), Framework.nativeGetRoutePoints(), this);

    manageRouteList.setAdapter(mManageRouteAdapter);

    // Enable drag & drop in route list.
    mTouchHelper = new ItemTouchHelper(new ManageRouteItemTouchHelperCallback(mManageRouteAdapter, getResources()));
    mTouchHelper.attachToRecyclerView(manageRouteList);

    return v;
  }

  @NonNull
  @Override
  public Dialog onCreateDialog(@Nullable Bundle savedInstanceState)
  {
    Dialog dialog = super.onCreateDialog(savedInstanceState);

    // Expand bottom sheet dialog.
    dialog.setOnShowListener(dialogInterface -> {
      FrameLayout bottomSheet =
          ((BottomSheetDialog) dialogInterface).findViewById(com.google.android.material.R.id.design_bottom_sheet);

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

  @Override
  public void onClick(View v)
  {
    int buttonId = v.getId();

    if (buttonId == R.id.btn__cancel)
    {
      // Close dialog if 'Cancel' button is pressed.
      dismiss();
    }
    else if (buttonId == R.id.image_my_location)
    {
      // Get current location.
      MapObject myLocation = MwmApplication.from(getContext()).getLocationHelper().getMyPosition();

      // Set 'My Location' as starting point of the route.
      if (myLocation != null)
        mManageRouteAdapter.setMyLocationAsStartingPoint(myLocation);
    }
    else if (buttonId == R.id.btn__plan)
    {
      // Get route points from adapter.
      ArrayList<RouteMarkData> newRoutePoints = mManageRouteAdapter.getRoutePoints();

      // Make sure that the new route contains at least 2 points (start and destination).
      assert (newRoutePoints.size() >= 2);

      // Remove all existing route points.
      Framework.nativeRemoveRoutePoints();

      // First, add the destination point.
      Framework.addRoutePoint(newRoutePoints.get(newRoutePoints.size() - 1));

      // Secondly, add the starting point.
      Framework.addRoutePoint(newRoutePoints.get(0));

      // And then, add all intermediate points (with no reordering).
      for (int pos = 1; pos < newRoutePoints.size() - 1; pos++)
        Framework.addRoutePoint(newRoutePoints.get(pos), false);

      // Launch route planning.
      RoutingController.get().launchPlanning();

      // Dismiss (close) manage route bottom sheet.
      dismiss();
    }
  }

  @Override
  public void startDrag(RecyclerView.ViewHolder viewHolder)
  {
    // Start dragging.
    mTouchHelper.startDrag(viewHolder);
  }

  @Override
  public void showMyLocationIcon(boolean showMyLocationIcon)
  {
    // Get current location.
    MapObject myLocation = MwmApplication.from(getContext()).getLocationHelper().getMyPosition();

    UiUtils.showIf(showMyLocationIcon && myLocation != null, mMyLocationImageView);
  }

  @Override
  public void onRoutePointDeleted(RecyclerView.ViewHolder viewHolder)
  {
    mManageRouteAdapter.deleteRoutePoint(viewHolder);

    mManageRouteAdapter.notifyDataSetChanged();
  }

  private static class ManageRouteItemTouchHelperCallback extends ItemTouchHelper.Callback
  {
    private final ManageRouteAdapter mManageRouteAdapter;

    public ManageRouteItemTouchHelperCallback(ManageRouteAdapter adapter, Resources resources)
    {
      mManageRouteAdapter = adapter;
    }

    @Override
    public int getMovementFlags(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder)
    {
      // Enable up & down dragging. No left-right swiping is enabled.
      return makeMovementFlags(UP | DOWN, 0);
    }

    @Override
    public boolean isLongPressDragEnabled()
    {
      return false;
    }

    @Override
    public boolean isItemViewSwipeEnabled()
    {
      return false;
    }

    @Override
    public boolean onMove(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder,
                          @NonNull RecyclerView.ViewHolder target)
    {
      mManageRouteAdapter.moveRoutePoint(viewHolder.getAbsoluteAdapterPosition(), target.getAbsoluteAdapterPosition());
      return true;
    }

    @Override
    public void onSwiped(@NonNull RecyclerView.ViewHolder viewHolder, int direction)
    {}

    @Override
    public void clearView(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder)
    {
      super.clearView(recyclerView, viewHolder);

      // Called when dragging action has finished.
      mManageRouteAdapter.notifyDataSetChanged();
    }
  }
}
