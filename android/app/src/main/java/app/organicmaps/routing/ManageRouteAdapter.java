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

public class ManageRouteAdapter extends RecyclerView.Adapter<ManageRouteAdapter.ManageRouteViewHolder>
{
  Context mContext;
  ArrayList<RouteMarkData> mRoutePoints;
  ManageRouteListener mManageRouteListener;
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
  public ManageRouteViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.manage_route_list_item, parent, false);

    return new ManageRouteViewHolder(view);
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onBindViewHolder(@NonNull ManageRouteViewHolder holder, int position)
  {
    int iconId;
    if (position == mRoutePoints.size())
    {
      holder.mTextViewTitle.setText("Add Stop");
      holder.mImageViewIcon.setImageDrawable(AppCompatResources.getDrawable(mContext, R.drawable.ic_plus_blue));
      holder.mImageViewDelete.setVisibility(View.GONE);
      holder.mImageViewDrag.setVisibility(View.GONE);
      holder.mTextViewTitle.setTextColor(AppCompatResources.getColorStateList(mContext, R.color.base_accent));
      holder.itemView.setOnClickListener((v) -> { mManageRouteListener.onAddStopButtonClicked(); });
      return;
    }

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
    holder.mImageViewIcon.setImageDrawable(AppCompatResources.getDrawable(mContext, iconId));
    String title;
    if (mRoutePoints.get(position).mIsMyPosition)
    {
      title = mContext.getString(app.organicmaps.sdk.R.string.core_my_position);
    }
    else
    {
      title = mRoutePoints.get(position).mTitle;
    }
    holder.mTextViewTitle.setText(title);
    // Show 'Delete' icon button only if we have intermediate stops...
    UiUtils.showIf(mRoutePoints.size() > 2 && mRoutePoints.get(position).mPointType != RouteMarkType.Start
                       && mRoutePoints.get(position).mPointType != RouteMarkType.Finish,
                   holder.mImageViewDelete);
    holder.mImageViewDelete.setOnClickListener(v -> mManageRouteListener.onRoutePointDeleted(holder));
    holder.mTextViewTitle.setOnClickListener(
        v -> { mManageRouteListener.onRoutePointClicked(holder.getAbsoluteAdapterPosition()); });

    // touch listener on drag handle to initiate drag !
    holder.mImageViewDrag.setOnTouchListener((v, event) -> {
      if (event.getAction() == MotionEvent.ACTION_DOWN)
      {
        mManageRouteListener.startDrag(holder);
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
    final int draggedItemIndex = draggedItem.getAbsoluteAdapterPosition();
    final int targetIndex = targetItem.getAbsoluteAdapterPosition();
    if (draggedItemIndex == targetIndex && targetIndex == mRoutePoints.size()) // Dragged to same spot. Do nothing.
      return;
    Collections.swap(mRoutePoints, draggedItemIndex, targetIndex);
    updateRoutePointsData();
    notifyItemMoved(draggedItemIndex, targetIndex);
    // Rebinding  view holders to update their content, draggedItem is now at targetIndex and targetItem is now at
    // draggedItemIndex.
    onBindViewHolder((ManageRouteViewHolder) draggedItem, targetIndex);
    onBindViewHolder((ManageRouteViewHolder) targetItem, draggedItemIndex);
  }

  public void deleteRoutePoint(RecyclerView.ViewHolder viewHolder)
  {
    mRoutePoints.remove(viewHolder.getAbsoluteAdapterPosition());
    updateRoutePointsData();
    notifyItemRemoved(viewHolder.getAbsoluteAdapterPosition());
  }
/// TODO for blue my location icon
//  public void setMyLocationAsStartingPoint(MapObject myLocation)
//  {
//    String latLonString = StringUtils.formatUsingUsLocale("%.6f, %.6f", myLocation.getLat(), myLocation.getLon());
//
//    // Replace route point in first position with 'My Position".
//    mRoutePoints.set(0, new RouteMarkData(latLonString, "", RouteMarkType.Start, 0, true, true, false,
//                                          myLocation.getLat(), myLocation.getLon()));
//
//    // Update data.
//    updateRoutePointsData();
//
//    // Update adapter.
//    notifyItemChanged(0);
//
//    // Show 'My location' crosshair button.
//    if (mManageRouteListener != null)
//      mManageRouteListener.showMyLocationIcon(true);
//  }
//
//  private void updateMyLocationIcon()
//  {
//    boolean containsMyLocationPoint = false;
//
//    for (RouteMarkData routePoint : mRoutePoints)
//    {
//      if (routePoint.mIsMyPosition)
//      {
//        containsMyLocationPoint = true;
//        break;
//      }
//    }
//
//    if (mManageRouteListener != null)
//      mManageRouteListener.showMyLocationIcon(!containsMyLocationPoint);
//  }

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

  static class ManageRouteViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    public final View mItemView;

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
      mItemView = itemView;
      mImageViewIcon = itemView.findViewById(R.id.type_icon);
      mTextViewTitle = itemView.findViewById(R.id.title);
      mImageViewDelete = itemView.findViewById(R.id.delete_icon);
      mImageViewDrag = itemView.findViewById(R.id.drag_icon);
    }
  }
}
