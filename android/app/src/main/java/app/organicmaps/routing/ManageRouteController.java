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
import app.organicmaps.sdk.util.Assert;
import java.util.ArrayList;

public class ManageRouteController implements ManageRouteAdapter.ManageRouteListener
{
  private final View mContainer;
  private final Context mContext;
  private final RecyclerView.Adapter<?> mHeaderAdapter;
  private ManageRouteAdapter mManageRouteAdapter;
  private ItemTouchHelper mTouchHelper;
  // Point order when the staged reorder began, so an unchanged drop skips the rebuild. Non-null until that drop is
  // committed or abandoned; panel edits are ignored meanwhile.
  @Nullable
  private ArrayList<RouteMarkData> mDragStartOrder;
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

  public void onRouteOrderChanged(@NonNull ArrayList<RouteMarkData> newRoutePoints)
  {
    // Make sure that the new route contains at least 2 points (start and destination).
    Assert.debug(newRoutePoints.size() >= 2, "There must be at least two route points");

    // Remove all existing route points.
    Framework.nativeRemoveRoutePoints();

    for (RouteMarkData point : newRoutePoints)
      Framework.addRoutePoint(point, false /* allowOptimization */);

    RoutingController.get().launchPlanning();
  }

  private boolean isReordering()
  {
    return mDragStartOrder != null;
  }

  public void refresh()
  {
    if (isReordering())
      return;
    // Keep the adapter and touch helper attached so route changes retain the list's scroll position.
    mManageRouteAdapter.setRoutePoints(Framework.nativeGetRoutePoints());
  }

  @Override
  public void startDrag(RecyclerView.ViewHolder viewHolder)
  {
    if (!isReordering() && viewHolder.getBindingAdapterPosition() != RecyclerView.NO_POSITION)
      mTouchHelper.startDrag(viewHolder);
  }
  @Override
  public void onRoutePointDeleted(RecyclerView.ViewHolder viewHolder)
  {
    final int position = viewHolder.getBindingAdapterPosition();
    // Positions are invalid until the layout following a complete adapter refresh.
    if (position == RecyclerView.NO_POSITION || isReordering())
      return;
    final RouteMarkData point = mManageRouteAdapter.getRoutePointAt(position);
    RoutingController.get().removeStop(point.mPointType, point.mIntermediateIndex);
  }
  @Override
  public void onAddStopButtonClicked()
  {
    if (!isReordering())
      mCallback.onAddStop();
  }
  @Override
  public void onRoutePointClicked(int position)
  {
    if (isReordering())
      return;
    ArrayList<RouteMarkData> routePoints = mManageRouteAdapter.getRoutePoints();
    if (position < 0 || position >= routePoints.size())
    {
      return;
    }
    RouteMarkType type = (position == 0)                      ? RouteMarkType.Start
                       : (position == routePoints.size() - 1) ? RouteMarkType.Finish
                                                              : RouteMarkType.Intermediate;

    RouteMarkData point = routePoints.get(position);
    RoutingController.get().waitForPoiReplacement(type,
                                                  type == RouteMarkType.Intermediate ? point.mIntermediateIndex : 0);
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
    RoutingController.get().waitForPoiReplacement(realType, 0);
    mCallback.onOpenRouteSearch();
  }

  private static class ManageRouteItemTouchHelperCallback extends ItemTouchHelper.Callback
  {
    private final ManageRouteAdapter mManageRouteAdapter;
    private final ManageRouteController mController;
    private int mDragGeneration;

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
        ++mDragGeneration;
        // Re-grabbing a recovering row continues the staged edit until its latest drop commits.
        if (!mController.isReordering())
          mController.mDragStartOrder = new ArrayList<>(mManageRouteAdapter.getRoutePoints());
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
      mManageRouteAdapter.moveRoutePoint(viewHolder, target);
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
      if (!mController.isReordering())
        return;
      // Both committing and refreshing can notify the adapter; defer them past a layout callback.
      final ArrayList<RouteMarkData> newOrder =
          isOrderDifferentFromDragStart() ? new ArrayList<>(mManageRouteAdapter.getRoutePoints()) : null;
      final int generation = mDragGeneration;
      recyclerView.post(() -> {
        if (generation != mDragGeneration || !mController.isReordering())
          return;
        mController.mDragStartOrder = null;
        // Cancellation or destruction of this planner abandons its pending edit.
        if (!recyclerView.isAttachedToWindow() || !RoutingController.get().isPlanning())
          return;
        if (newOrder != null)
          mController.onRouteOrderChanged(newOrder);
        else
          mController.refresh();
      });
    }

    private boolean isOrderDifferentFromDragStart()
    {
      final ArrayList<RouteMarkData> start = mController.mDragStartOrder;
      Assert.debug(start != null, "A drag must have its original point order");
      ArrayList<RouteMarkData> current = mManageRouteAdapter.getRoutePoints();
      if (current.size() != start.size())
        return true;
      for (int i = 0; i < current.size(); i++)
      {
        if (current.get(i) != start.get(i))
          return true;
      }
      return false;
    }
  }
}
