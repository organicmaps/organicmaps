package app.organicmaps.routing;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.routing.RouteMarkData;
import app.organicmaps.sdk.routing.RouteMarkType;
import app.organicmaps.sdk.util.Assert;
import app.organicmaps.util.UiUtils;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;

public class ManageRouteAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private final Context mContext;
  private final ArrayList<RouteMarkData> mRoutePoints;
  private final ManageRouteListener mManageRouteListener;
  private static final int TYPE_POINT = 0;
  private static final int TYPE_ADD_BUTTON = 1;
  public interface ManageRouteListener
  {
    void startDrag(RecyclerView.ViewHolder viewHolder);
    void onRoutePointDeleted(RecyclerView.ViewHolder viewHolder);
    void onAddStopButtonClicked();
    void onRoutePointClicked(int position);
  }

  public ManageRouteAdapter(Context context, RouteMarkData[] routeMarkData, ManageRouteListener listener)
  {
    mContext = context;
    mRoutePoints = new ArrayList<>(Arrays.asList(routeMarkData));
    mManageRouteListener = listener;
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    final LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    if (viewType == TYPE_ADD_BUTTON)
      return new AddStopViewHolder(inflater.inflate(R.layout.manage_route_add_stop_item, parent, false));
    return new ManageRouteViewHolder(inflater.inflate(R.layout.manage_route_list_item, parent, false));
  }

  @Override
  public int getItemViewType(int position)
  {
    return (position == mRoutePoints.size()) ? TYPE_ADD_BUTTON : TYPE_POINT;
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    if (holder instanceof AddStopViewHolder)
    {
      holder.itemView.setOnClickListener((v) -> { mManageRouteListener.onAddStopButtonClicked(); });
      return;
    }
    if (!(holder instanceof ManageRouteViewHolder pointHolder))
      return;

    int iconId;
    switch (mRoutePoints.get(position).mPointType)
    {
    case Start:
      if (mRoutePoints.get(position).mIsMyPosition)
        iconId = R.drawable.ic_location_arrow_blue;
      else
        iconId = R.drawable.route_point_start;
      break;
    case Intermediate:
      TypedArray iconArray = mContext.getResources().obtainTypedArray(R.array.route_stop_icons);
      iconId = iconArray.getResourceId(mRoutePoints.get(position).mIntermediateIndex, R.drawable.route_point_20);
      iconArray.recycle();
      break;
    case Finish: iconId = R.drawable.route_point_finish; break;
    default: iconId = R.drawable.warning_icon; break;
    }
    pointHolder.mImageViewIcon.setImageDrawable(AppCompatResources.getDrawable(mContext, iconId));
    String title;
    if (mRoutePoints.get(position).mIsMyPosition)
    {
      title = mContext.getString(app.organicmaps.sdk.R.string.core_my_position);
    }
    else
    {
      title = mRoutePoints.get(position).mTitle;
    }
    pointHolder.mTextViewTitle.setText(title);
    // Show 'Delete' icon button only if we have intermediate stops...
    UiUtils.showIf(mRoutePoints.size() > 2 && mRoutePoints.get(position).mPointType != RouteMarkType.Start
                       && mRoutePoints.get(position).mPointType != RouteMarkType.Finish,
                   pointHolder.mImageViewDelete);
    pointHolder.mImageViewDelete.setOnClickListener(v -> mManageRouteListener.onRoutePointDeleted(pointHolder));
    pointHolder.itemView.setOnClickListener(
        v -> { mManageRouteListener.onRoutePointClicked(pointHolder.getBindingAdapterPosition()); });

    // touch listener on drag handle to initiate drag !
    pointHolder.mImageViewDrag.setOnTouchListener((v, event) -> {
      if (event.getAction() == MotionEvent.ACTION_DOWN)
      {
        mManageRouteListener.startDrag(pointHolder);
        return true;
      }
      return false;
    });
  }

  @Override
  public int getItemCount()
  {
    return mRoutePoints.size() + 1;
  }

  public void moveRoutePoint(@NonNull RecyclerView.ViewHolder draggedItem, @NonNull RecyclerView.ViewHolder targetItem)
  {
    final int draggedItemIndex = draggedItem.getBindingAdapterPosition();
    final int targetIndex = targetItem.getBindingAdapterPosition();
    if (draggedItemIndex == targetIndex) // Dragged to the same spot, nothing to reorder.
      return;
    Collections.swap(mRoutePoints, draggedItemIndex, targetIndex);
    updateRoutePointsData();
    notifyItemMoved(draggedItemIndex, targetIndex);
  }

  public void deleteRoutePoint(RecyclerView.ViewHolder viewHolder)
  {
    final int position = viewHolder.getBindingAdapterPosition();
    mRoutePoints.remove(position);
    updateRoutePointsData();
    notifyItemRemoved(position);
    // Remaining points may need renumbered icons or updated delete-button visibility after the removal.
    notifyItemRangeChanged(0, getItemCount());
  }

  private void updateRoutePointsData()
  {
    Assert.debug(mRoutePoints.size() >= 2, "There must be at least two route points");

    // Set starting point.
    mRoutePoints.get(0).mPointType = RouteMarkType.Start;
    // Set finish point.
    mRoutePoints.get(mRoutePoints.size() - 1).mPointType = RouteMarkType.Finish;
    // Set intermediate point(s).
    for (int pos = 1; pos < mRoutePoints.size() - 1; pos++)
    {
      mRoutePoints.get(pos).mPointType = RouteMarkType.Intermediate;
      mRoutePoints.get(pos).mIntermediateIndex = pos - 1;
    }
  }

  public ArrayList<RouteMarkData> getRoutePoints()
  {
    return mRoutePoints;
  }

  // Resync the existing adapter with the native route points, keeping the same adapter/touch-helper instances.
  public void setRoutePoints(@NonNull RouteMarkData[] routePoints)
  {
    mRoutePoints.clear();
    mRoutePoints.addAll(Arrays.asList(routePoints));
    notifyDataSetChanged();
  }

  static class ManageRouteViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    public final ImageView mImageViewIcon;

    @NonNull
    public final TextView mTextViewTitle;

    @NonNull
    public final ImageView mImageViewDelete;

    @NonNull
    public final ImageView mImageViewDrag;

    ManageRouteViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mImageViewIcon = itemView.findViewById(R.id.type_icon);
      mTextViewTitle = itemView.findViewById(R.id.title);
      mImageViewDelete = itemView.findViewById(R.id.delete_icon);
      mImageViewDrag = itemView.findViewById(R.id.drag_icon);
    }
  }

  static class AddStopViewHolder extends RecyclerView.ViewHolder
  {
    AddStopViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }
  }
}
