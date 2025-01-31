package app.organicmaps.routing;

import android.annotation.SuppressLint;
import android.content.Context;
import android.content.res.TypedArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.appcompat.content.res.AppCompatResources;
import androidx.recyclerview.widget.RecyclerView;

import app.organicmaps.R;
import app.organicmaps.util.UiUtils;

public class ManageRouteAdapter extends RecyclerView.Adapter<ManageRouteAdapter.ManageRouteViewHolder>
{
  Context mContext;
  RouteMarkData[] mRouteMarkData;

  public ManageRouteAdapter(Context context, RouteMarkData[] routeMarkData)
  {
    mContext = context;
    mRouteMarkData = routeMarkData;
  }

  @NonNull
  @Override
  public ManageRouteViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.manage_route_list_item,
                                                                 parent, false);

    return new ManageRouteViewHolder(view);
  }

  @Override
  public void onBindViewHolder(@NonNull ManageRouteViewHolder holder, int position)
  {
    // Set icon.
    int iconId;

    if (position == 0)
    {
      // Starting point.
      if (mRouteMarkData[position].mIsMyPosition)
        iconId = R.drawable.ic_my_position_blue;
      else
        iconId = R.drawable.route_point_start;
    }
    else if (position == (mRouteMarkData.length - 1))
    {
      // Destination point.
      iconId = R.drawable.route_point_finish;
    }
    else
    {
      // Intermediate point.
      TypedArray iconArray = mContext.getResources().obtainTypedArray(R.array.route_stop_icons);
      iconId = iconArray.getResourceId(position - 1, R.drawable.route_point_01);
    }

    holder.mImageViewIcon.setImageDrawable(AppCompatResources.getDrawable(mContext, iconId));

    // Set title & subtitle.
    String title, subtitle;
    boolean showSubtitle = true;

    if (mRouteMarkData[position].mIsMyPosition)
    {
      // My position point.
      title = mContext.getString(R.string.p2p_your_location);

      if (position == 0)
      {
        // Hide my position coordinates if it's the starting point of the route.
        subtitle = "";
        showSubtitle = false;
      }
      else
        subtitle = mRouteMarkData[position].mTitle;
    }
    else
    {
      title = mRouteMarkData[position].mTitle;
      subtitle = mRouteMarkData[position].mSubtitle;
    }

    holder.mTextViewTitle.setText(title);
    holder.mTextViewSubtitle.setText(subtitle);
    UiUtils.showIf(showSubtitle, holder.mTextViewSubtitle);
  }

  @Override
  public int getItemCount()
  {
    return mRouteMarkData.length;
  }

  @SuppressLint("NotifyDataSetChanged")
  public void moveRoutePoint(int draggedItemIndex, int targetIndex)
  {
    // Swap route points.
    RouteMarkData tempData = mRouteMarkData[targetIndex];
    mRouteMarkData[targetIndex] = mRouteMarkData[draggedItemIndex];
    mRouteMarkData[draggedItemIndex] = tempData;

    notifyDataSetChanged();
  }

  public RouteMarkData[] getRoutePoints()
  {
    // Update mark data parameters.
    for(int pos = 0; pos < mRouteMarkData.length; pos++)
    {
      if (pos == 0)
      {
        // Set as starting point.
        mRouteMarkData[pos].mPointType = RoutePointInfo.ROUTE_MARK_START;
      }
      else if (pos == mRouteMarkData.length -1)
      {
        // Set as finish point.
        mRouteMarkData[pos].mPointType = RoutePointInfo.ROUTE_MARK_FINISH;
      }
      else
      {
        // Set as intermediate point.
        mRouteMarkData[pos].mPointType = RoutePointInfo.ROUTE_MARK_INTERMEDIATE;
        mRouteMarkData[pos].mIntermediateIndex = pos - 1;
      }
    }

    return mRouteMarkData;
  }

  static class ManageRouteViewHolder extends RecyclerView.ViewHolder
  {
    @NonNull
    public final ImageView mImageViewIcon;

    @NonNull
    public final TextView mTextViewTitle;

    @NonNull
    public final TextView mTextViewSubtitle;

    ManageRouteViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mImageViewIcon = itemView.findViewById(R.id.icon);
      mTextViewTitle = itemView.findViewById(R.id.title);
      mTextViewSubtitle = itemView.findViewById(R.id.subtitle);
    }
  }
}
