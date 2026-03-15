package app.organicmaps.routing;

import static androidx.recyclerview.widget.ItemTouchHelper.DOWN;
import static androidx.recyclerview.widget.ItemTouchHelper.UP;

import android.app.Activity;
import android.content.Context;
import android.content.res.Resources;
import android.graphics.drawable.GradientDrawable;
import android.view.View;
import android.widget.Button;
import android.widget.ImageView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.core.content.ContextCompat;
import androidx.core.content.res.ResourcesCompat;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.log.Logger;
import app.organicmaps.search.SearchActivity;
import app.organicmaps.util.UiUtils;
import com.google.android.material.divider.MaterialDividerItemDecoration;
import java.util.ArrayList;

public class ManageRouteController implements ManageRouteAdapter.ManageRouteListener
{
  private final View mContainer;
  private final Context mContext;
  private ManageRouteAdapter mManageRouteAdapter;
  private ItemTouchHelper mTouchHelper;
  private final ManageRouteCallback mCallback;

  public interface ManageRouteCallback
  {
    void onAddStop();
    void onReplaceStop();
  }

  public ManageRouteController(@NonNull View container, @NonNull ManageRouteCallback callback)
  {
    mContainer = container;
    mContext = container.getContext();
    mCallback = callback;
    initViews();
  }

  private void initViews()
  {
    RecyclerView manageRouteList = mContainer.findViewById(R.id.manage_route_list);
    LinearLayoutManager layoutManager = new LinearLayoutManager(mContext) {};
    manageRouteList.setLayoutManager(layoutManager);
    mManageRouteAdapter = new ManageRouteAdapter(mContext, Framework.nativeGetRoutePoints(), this);
    MaterialDividerItemDecoration decoration =
        new MaterialDividerItemDecoration(mContext, layoutManager.getOrientation());
    decoration.setLastItemDecorated(false);
    decoration.setDividerInsetStart(mContext.getResources().getDimensionPixelSize(R.dimen.margin_double_and_half));
    GradientDrawable shape = new GradientDrawable();
    shape.setColor(ContextCompat.getColor(mContext, R.color.routing_bottom_manage_route_background));
    shape.setCornerRadius(20);
    manageRouteList.setBackground(shape);
    manageRouteList.setClipToOutline(true);
    manageRouteList.addItemDecoration(decoration);
    manageRouteList.setAdapter(mManageRouteAdapter);
    mTouchHelper = new ItemTouchHelper(new ManageRouteItemTouchHelperCallback(mManageRouteAdapter, this));
    mTouchHelper.attachToRecyclerView(manageRouteList);
  }

  public void onRouteOrderChanged()
  {
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
  }

  public void refresh()
  {
    // Re-create adapter or refresh data?
    // ManageRouteAdapter doesn't have a specific update method for external data, but we can re-create it or add a
    // method. Re-creating is safer for now to ensure sync with nativeGetRoutePoints().
    RecyclerView manageRouteList = mContainer.findViewById(R.id.manage_route_list);
    mManageRouteAdapter = new ManageRouteAdapter(mContext, Framework.nativeGetRoutePoints(), this);
    manageRouteList.setAdapter(mManageRouteAdapter);
    // Re-attach touch helper? It's attached to RecyclerView, not Adapter. Adapter is changed.
    // ManageRouteItemTouchHelperCallback holds reference to Adapter.
    // We need to update callback or re-create touch helper.
    mTouchHelper.attachToRecyclerView(null); // Detach old
    mTouchHelper = new ItemTouchHelper(new ManageRouteItemTouchHelperCallback(mManageRouteAdapter, this));
    mTouchHelper.attachToRecyclerView(manageRouteList);
  }

  @Override
  public void startDrag(RecyclerView.ViewHolder viewHolder)
  {
    // Start dragging.
    mTouchHelper.startDrag(viewHolder);
  }
  @Override
  public void onRoutePointDeleted(RecyclerView.ViewHolder viewHolder)
  {
    mManageRouteAdapter.deleteRoutePoint(viewHolder);
    mManageRouteAdapter.notifyDataSetChanged();
    onRouteOrderChanged();
  }
  @Override
  public void onAddStopButtonClicked()
  {
    mCallback.onAddStop();
  }
  @Override
  public void onRoutePointClicked(int position)
  {
    ArrayList<RouteMarkData> routePoints = mManageRouteAdapter.getRoutePoints();
    if (position < 0 || routePoints == null || position >= routePoints.size())
    {
      return;
    }
    RouteMarkType type = (position == 0)                      ? RouteMarkType.Start
                       : (position == routePoints.size() - 1) ? RouteMarkType.Finish
                                                              : RouteMarkType.Intermediate;

    RouteMarkData point = routePoints.get(position);
    RoutingController.get().waitForPoiPick(type);
    RoutingController.get().replaceStopPoiPick(type == RouteMarkType.Intermediate ? point.mIntermediateIndex : 0);
    mCallback.onReplaceStop();
  }

  private static class ManageRouteItemTouchHelperCallback extends ItemTouchHelper.Callback
  {
    private final ManageRouteAdapter mManageRouteAdapter;
    private final ManageRouteController mController;
    private boolean mOrderChanged = false;

    public ManageRouteItemTouchHelperCallback(ManageRouteAdapter adapter, ManageRouteController controller)
    {
      mController = controller;
      mManageRouteAdapter = adapter;
    }
    @Override
    public int getMovementFlags(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder)
    {
      // Enable up & down dragging. No left-right swiping is enabled.
      return makeMovementFlags(UP | DOWN, 0);
    }
    @Override
    public void onSelectedChanged(@Nullable RecyclerView.ViewHolder viewHolder, int actionState)
    {
      if (viewHolder != null && actionState == ItemTouchHelper.ACTION_STATE_DRAG)
      {
        viewHolder.itemView.setTranslationX(-10f);
        viewHolder.itemView.setTranslationZ(6f);
      }
      super.onSelectedChanged(viewHolder, actionState);
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
      if (target.getBindingAdapterPosition() == mManageRouteAdapter.getItemCount() - 1)
        return false;
      mManageRouteAdapter.moveRoutePoint(viewHolder, target);
      mOrderChanged = true;
      return true;
    }

    @Override
    public void onSwiped(@NonNull RecyclerView.ViewHolder viewHolder, int direction)
    {}

    @Override
    public void clearView(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder)
    {
      super.clearView(recyclerView, viewHolder);
      viewHolder.itemView.setTranslationX(0f);
      viewHolder.itemView.setTranslationZ(0f);
      if (mOrderChanged)
      {
        mController.onRouteOrderChanged();
        mOrderChanged = false;
      }
    }
  }
}
