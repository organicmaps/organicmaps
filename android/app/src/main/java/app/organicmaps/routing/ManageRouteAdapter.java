package app.organicmaps.routing;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.graphics.RectF;
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
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.util.StringUtils;
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
    void showMyLocationIcon(boolean showMyLocationIcon);
    void onRoutePointDeleted(RecyclerView.ViewHolder viewHolder);
  }

  public ManageRouteAdapter(Context context, RouteMarkData[] routeMarkData, ManageRouteListener listener)
  {
    mContext = context;
    mRoutePoints = new ArrayList<>(Arrays.asList(routeMarkData));
    mManageRouteListener = listener;

    updateMyLocationIcon();
  }

  @NonNull
  @Override
  public ManageRouteViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.manage_route_list_item,
                                                                 parent, false);

    return new ManageRouteViewHolder(view);
  }

  @SuppressLint("ClickableViewAccessibility")
  @Override
  public void onBindViewHolder(@NonNull ManageRouteViewHolder holder, int position)
  {
    // Set route point icon.
    int iconId;

    switch(mRoutePoints.get(position).mPointType)
    {
      case RoutePointInfo.ROUTE_MARK_START: // Starting point.
        if (mRoutePoints.get(position).mIsMyPosition)
          iconId = R.drawable.ic_my_position_blue;
        else
          iconId = R.drawable.route_point_start;
        break;

      case RoutePointInfo.ROUTE_MARK_INTERMEDIATE: // Intermediate stop.
        TypedArray iconArray = mContext.getResources().obtainTypedArray(R.array.route_stop_icons);
        iconId = iconArray.getResourceId(mRoutePoints.get(position).mIntermediateIndex,
                                         R.drawable.route_point_01);
        iconArray.recycle();
        break;

      case RoutePointInfo.ROUTE_MARK_FINISH: // Destination point.
        iconId = R.drawable.route_point_finish;
        break;

      default: // Unknown route type.
        iconId = R.drawable.warning_icon;
        break;
    }

    // Set icon widget.
    holder.mImageViewIcon.setImageDrawable(AppCompatResources.getDrawable(mContext, iconId));

    // Set title & subtitle.
    String title, subtitle;
    boolean showSubtitle;

    if (mRoutePoints.get(position).mIsMyPosition)
    {
      // My position point.
      title = mContext.getString(R.string.p2p_your_location);

      if (mRoutePoints.get(position).mPointType == RoutePointInfo.ROUTE_MARK_START)
      {
        // Hide my position coordinates if it's the starting point of the route.
        subtitle = "";
        showSubtitle = false;
      }
      else
      {
        subtitle = mRoutePoints.get(position).mTitle;
        showSubtitle = true;
      }
    }
    else
    {
      title = mRoutePoints.get(position).mTitle;
      subtitle = mRoutePoints.get(position).mSubtitle;
      showSubtitle = true;
    }

    holder.mTextViewTitle.setText(title);
    holder.mTextViewSubtitle.setText(subtitle);
    UiUtils.showIf(showSubtitle, holder.mTextViewSubtitle);

    // Show 'Delete' icon button only if we have intermediate stops.
    UiUtils.showIf(mRoutePoints.size() > 2, holder.mImageViewDelete);

    // Detection of touch events on holder view.
    holder.mItemView.setOnTouchListener((v, event) -> {

      if (event.getAction() == MotionEvent.ACTION_DOWN)
      {
        RectF deleteButtonRect = new RectF(holder.mImageViewDelete.getLeft(),
                                           holder.mImageViewDelete.getTop(),
                                           holder.mImageViewDelete.getRight(),
                                           holder.mImageViewDelete.getBottom());

        if (holder.mImageViewDelete.isShown() && deleteButtonRect.contains(event.getX(), event.getY()))
        {
          // User has clicked on the 'Delete' icon button.
          mManageRouteListener.onRoutePointDeleted(holder);
        }
        else
        {
          // Call start drag listener on touch.
          mManageRouteListener.startDrag(holder);
        }
      }

      return false;
    });
  }

  @Override
  public int getItemCount()
  {
    return mRoutePoints.size();
  }

  public void moveRoutePoint(int draggedItemIndex, int targetIndex)
  {
    if (draggedItemIndex == targetIndex) // Dragged to same spot. Do nothing.
      return;

    Collections.swap(mRoutePoints, draggedItemIndex, targetIndex);

    updateRoutePointsData();

    notifyItemMoved(draggedItemIndex, targetIndex);
  }

  public void deleteRoutePoint(RecyclerView.ViewHolder viewHolder)
  {
    mRoutePoints.remove(viewHolder.getAbsoluteAdapterPosition());

    updateRoutePointsData();

    notifyItemRemoved(viewHolder.getAbsoluteAdapterPosition());
  }

  public void setMyLocationAsStartingPoint(MapObject myLocation)
  {
    String latLonString = StringUtils.formatUsingUsLocale("%.6f, %.6f",
                                                          myLocation.getLat(),
                                                          myLocation.getLon());

    // Replace route point in first position with 'My Position"
    mRoutePoints.set(0, new RouteMarkData(latLonString, // Title.
                                          "", // Subtitle.
                                          RoutePointInfo.ROUTE_MARK_START, // Point type.
                                          0, // Intermediate index.
                                          true, // Visible.
                                          true, // Is my position.
                                          false, // No passed.
                                          myLocation.getLat(), // Latitude.
                                          myLocation.getLon()));  // Longitude.

    // Update data.
    updateRoutePointsData();

    // Update adapter.
    notifyItemChanged(0);

    // Update 'My position' image button.
    updateMyLocationIcon();
  }

  private void updateMyLocationIcon()
  {
    boolean containsMyLocationPoint = false;

    for (RouteMarkData routePoint : mRoutePoints)
    {
      if (routePoint.mIsMyPosition)
      {
        containsMyLocationPoint = true;
        break;
      }
    }

    if (mManageRouteListener != null)
      mManageRouteListener.showMyLocationIcon(!containsMyLocationPoint);
  }

  private void updateRoutePointsData()
  {
    for (int pos = 0; pos < mRoutePoints.size(); pos++)
    {
      if (pos == 0)
      {
        // Set as starting point.
        mRoutePoints.get(pos).mPointType = RoutePointInfo.ROUTE_MARK_START;
      }
      else if (pos == mRoutePoints.size() - 1)
      {
        // Set as finish point.
        mRoutePoints.get(pos).mPointType = RoutePointInfo.ROUTE_MARK_FINISH;
      }
      else
      {
        // Set as intermediate point.
        mRoutePoints.get(pos).mPointType = RoutePointInfo.ROUTE_MARK_INTERMEDIATE;
        mRoutePoints.get(pos).mIntermediateIndex = pos - 1;
      }
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
    public final TextView mTextViewSubtitle;

    @NonNull
    public final ImageView mImageViewDelete;

    ManageRouteViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mItemView = itemView;
      mImageViewIcon = itemView.findViewById(R.id.type_icon);
      mTextViewTitle = itemView.findViewById(R.id.title);
      mTextViewSubtitle = itemView.findViewById(R.id.subtitle);
      mImageViewDelete = itemView.findViewById(R.id.delete_icon);
    }
  }
}
