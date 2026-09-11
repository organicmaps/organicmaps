package app.organicmaps.routing;

import static androidx.recyclerview.widget.ItemTouchHelper.DOWN;
import static androidx.recyclerview.widget.ItemTouchHelper.UP;

import android.content.Context;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.ConcatAdapter;
import androidx.recyclerview.widget.ItemTouchHelper;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.routing.RoutingController;
import java.util.ArrayList;

public class ManageRouteController implements ManageRouteAdapter.ManageRouteListener
{
  private final View mContainer;
  private final Context mContext;
  private final RecyclerView.Adapter<?> mHeaderAdapter;
  private ManageRouteAdapter mManageRouteAdapter;
  private ItemTouchHelper mTouchHelper;
  private final ManageRouteCallback mCallback;

  public interface ManageRouteCallback
  {
    void onAddStop();
    void onOpenRouteSearch();
  }

  public ManageRouteController(@NonNull View container, @NonNull RecyclerView.Adapter<?> headerAdapter,
                               @NonNull ManageRouteCallback callback)
  {
    mContainer = container;
    mContext = container.getContext();
    mHeaderAdapter = headerAdapter;
    mCallback = callback;
    initViews();
  }

  private void initViews()
  {
    RecyclerView manageRouteList = mContainer.findViewById(R.id.manage_route_list);
    LinearLayoutManager layoutManager = new LinearLayoutManager(mContext);
    manageRouteList.setLayoutManager(layoutManager);
    mManageRouteAdapter = new ManageRouteAdapter(mContext, Framework.nativeGetRoutePoints(), this);
    manageRouteList.addItemDecoration(new RoundedSectionItemDecoration(mContext, mManageRouteAdapter));
    manageRouteList.addItemDecoration(new SectionDividerItemDecoration(mContext, mManageRouteAdapter));
    manageRouteList.setAdapter(new ConcatAdapter(mHeaderAdapter, mManageRouteAdapter));
    mTouchHelper = new ItemTouchHelper(new ManageRouteItemTouchHelperCallback(mManageRouteAdapter, this));
    mTouchHelper.attachToRecyclerView(manageRouteList);
  }

  public void onRouteOrderChanged()
  {
    RoutingController.get().launchPlanning();
  }

  public void refresh()
  {
    // Resync the existing adapter with the native route points. Reusing the same adapter and touch helper
    // preserves the scroll position and never tears down an in-progress drag.
    mManageRouteAdapter.setRoutePoints(Framework.nativeGetRoutePoints());
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
    RouteMarkData point = mManageRouteAdapter.getRoutePoints().get(viewHolder.getBindingAdapterPosition());
    mManageRouteAdapter.deleteRoutePoint(viewHolder);
    Framework.nativeRemoveRoutePoint(point.mPointType, point.mIntermediateIndex);
    RoutingController.get().launchPlanning();
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
    mCallback.onOpenRouteSearch();
  }

  @Override
  public void onPartialSlotClicked(@NonNull RouteMarkType type)
  {
    RoutingController.get().waitForPoiPick(type);
    mCallback.onOpenRouteSearch();
  }

  @Override
  public void onPartialSlotReplaceClicked(@NonNull RouteMarkType realType)
  {
    RoutingController.get().waitForPoiPick(realType);
    RoutingController.get().replaceStopPoiPick(0);
    mCallback.onOpenRouteSearch();
  }

  private static class ManageRouteItemTouchHelperCallback extends ItemTouchHelper.Callback
  {
    private final ManageRouteAdapter mManageRouteAdapter;
    private final ManageRouteController mController;
    private boolean mOrderChanged = false;
    // Snapshot of the route-point reference order captured when the drag starts. Used on drop to skip the
    // native rebuild if the user dragged around but ended at the original order (e.g. picked up an item and
    // dropped it back where it was).
    @Nullable
    private ArrayList<RouteMarkData> mDragStartOrder;

    public ManageRouteItemTouchHelperCallback(ManageRouteAdapter adapter, ManageRouteController controller)
    {
      mController = controller;
      mManageRouteAdapter = adapter;
    }
    @Override
    public int getMovementFlags(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder viewHolder)
    {
      // Only route-point rows (ManageRouteAdapter inside the ConcatAdapter) are draggable; chart header is not.
      // instanceof is robust against transient binding-adapter resolution inside ConcatAdapter.
      if (!(viewHolder instanceof ManageRouteAdapter.ManageRouteViewHolder))
        return 0;
      if (mManageRouteAdapter.getRoutePoints().size() < 2)
        return 0;
      // Enable up & down dragging. No left-right swiping is enabled.
      return makeMovementFlags(UP | DOWN, 0);
    }
    @Override
    public boolean canDropOver(@NonNull RecyclerView recyclerView, @NonNull RecyclerView.ViewHolder current,
                               @NonNull RecyclerView.ViewHolder target)
    {
      if (!(target instanceof ManageRouteAdapter.ManageRouteViewHolder))
        return false;
      final int pos = target.getBindingAdapterPosition();
      return pos >= 0 && pos < mManageRouteAdapter.getItemCount() - 1;
    }
    @Override
    public void onSelectedChanged(@Nullable RecyclerView.ViewHolder viewHolder, int actionState)
    {
      if (viewHolder != null && actionState == ItemTouchHelper.ACTION_STATE_DRAG)
      {
        mDragStartOrder = new ArrayList<>(mManageRouteAdapter.getRoutePoints());
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
      int from = viewHolder.getBindingAdapterPosition();
      int to = target.getBindingAdapterPosition();
      if (from == to)
        return false;
      // Match the adapter's swap using explicit moves, which never trigger optimization.
      Framework.nativeMoveRoutePoint(from, to);
      if (Math.abs(from - to) > 1)
        Framework.nativeMoveRoutePoint(from < to ? to - 1 : to + 1, from);
      mManageRouteAdapter.moveRoutePoint(viewHolder, target);
      mOrderChanged = isOrderDifferentFromDragStart();
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
        mOrderChanged = false;
        // clearView can fire mid-layout, and the rebuild chain ends in notifyDataSetChanged (forbidden during
        // layout) — post it past the layout pass. The core already has the new point order.
        recyclerView.post(mController::onRouteOrderChanged);
      }
      mDragStartOrder = null;
    }

    private boolean isOrderDifferentFromDragStart()
    {
      if (mDragStartOrder == null)
        return true;
      ArrayList<RouteMarkData> current = mManageRouteAdapter.getRoutePoints();
      if (current.size() != mDragStartOrder.size())
        return true;
      for (int i = 0; i < current.size(); i++)
      {
        if (current.get(i) != mDragStartOrder.get(i))
          return true;
      }
      return false;
    }
  }
}
