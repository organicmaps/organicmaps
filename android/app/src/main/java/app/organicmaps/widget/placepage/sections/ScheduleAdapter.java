package app.organicmaps.widget.placepage.sections;

import android.content.res.ColorStateList;
import android.graphics.Paint;
import android.graphics.drawable.GradientDrawable;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.core.content.ContextCompat;
import androidx.core.widget.ImageViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import java.util.ArrayList;
import java.util.List;

public class ScheduleAdapter extends RecyclerView.Adapter<ScheduleAdapter.ViewHolder>
{
  private final List<ScheduleItem> mItems = new ArrayList<>();

  public enum ArrivalStatus
  {
    ON_TIME,
    DELAYED,
    EARLY
  }

  public enum TransportType
  {
    BUS,
    TRAM
  }

  public static class ScheduleItem
  {
    public final String routeNumber;
    public final String destination;
    public final String arrivalTime; // e.g. "3 min"
    public final String scheduledTime; // original scheduled time, shown if delayed/early
    public final ArrivalStatus status;
    public final int color;
    public final TransportType transportType;
    public final boolean isDynamic; // true = real-time data

    public ScheduleItem(String routeNumber, String destination, String arrivalTime, int color)
    {
      this(routeNumber, destination, arrivalTime, null, ArrivalStatus.ON_TIME, color, TransportType.BUS, false);
    }

    public ScheduleItem(String routeNumber, String destination, String arrivalTime, int color,
                        TransportType transportType)
    {
      this(routeNumber, destination, arrivalTime, null, ArrivalStatus.ON_TIME, color, transportType, false);
    }

    public ScheduleItem(String routeNumber, String destination, String arrivalTime, int color,
                        TransportType transportType, boolean isDynamic)
    {
      this(routeNumber, destination, arrivalTime, null, ArrivalStatus.ON_TIME, color, transportType, isDynamic);
    }

    public ScheduleItem(String routeNumber, String destination, String arrivalTime, String scheduledTime,
                        ArrivalStatus status, int color)
    {
      this(routeNumber, destination, arrivalTime, scheduledTime, status, color, TransportType.BUS, false);
    }

    public ScheduleItem(String routeNumber, String destination, String arrivalTime, String scheduledTime,
                        ArrivalStatus status, int color, TransportType transportType)
    {
      this(routeNumber, destination, arrivalTime, scheduledTime, status, color, transportType, false);
    }

    public ScheduleItem(String routeNumber, String destination, String arrivalTime, String scheduledTime,
                        ArrivalStatus status, int color, TransportType transportType, boolean isDynamic)
    {
      this.routeNumber = routeNumber;
      this.destination = destination;
      this.arrivalTime = arrivalTime;
      this.scheduledTime = scheduledTime;
      this.status = status;
      this.color = color;
      this.transportType = transportType;
      this.isDynamic = isDynamic;
    }
  }

  public void setItems(List<ScheduleItem> items)
  {
    mItems.clear();
    mItems.addAll(items);
    notifyDataSetChanged();
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    View view = LayoutInflater.from(parent.getContext()).inflate(R.layout.item_schedule_route, parent, false);
    return new ViewHolder(view);
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  static class ViewHolder extends RecyclerView.ViewHolder
  {
    private final ImageView mTransportIcon;
    private final TextView mRouteNumber;
    private final TextView mDestination;
    private final TextView mArrivalTime;
    private final TextView mScheduledTime;
    private final TextView mStatusLabel;
    private final ImageView mDynamicIndicator;

    ViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mTransportIcon = itemView.findViewById(R.id.iv_transport_icon);
      mRouteNumber = itemView.findViewById(R.id.tv_route_number);
      mDestination = itemView.findViewById(R.id.tv_destination);
      mArrivalTime = itemView.findViewById(R.id.tv_arrival_time);
      mScheduledTime = itemView.findViewById(R.id.tv_scheduled_time);
      mStatusLabel = itemView.findViewById(R.id.tv_status_label);
      mDynamicIndicator = itemView.findViewById(R.id.iv_dynamic_indicator);
    }

    void bind(ScheduleItem item)
    {
      // Set transport type icon
      if (item.transportType == TransportType.TRAM)
        mTransportIcon.setImageResource(R.drawable.ic_category_tram);
      else
        mTransportIcon.setImageResource(R.drawable.ic_category_bus);

      // Apply theme-aware tint (white in dark mode, dark in light mode)
      TypedValue typedValue = new TypedValue();
      itemView.getContext().getTheme().resolveAttribute(android.R.attr.textColorPrimary, typedValue, true);
      int color = ContextCompat.getColor(itemView.getContext(), typedValue.resourceId);
      ImageViewCompat.setImageTintList(mTransportIcon, ColorStateList.valueOf(color));

      mRouteNumber.setText(item.routeNumber);
      mDestination.setText(item.destination);
      mArrivalTime.setText(item.arrivalTime);

      // Set route badge color
      GradientDrawable background = new GradientDrawable();
      background.setShape(GradientDrawable.RECTANGLE);
      background.setCornerRadius(16f);
      background.setColor(item.color);
      mRouteNumber.setBackground(background);

      // Handle delay/early status
      if (item.status == ArrivalStatus.ON_TIME || item.scheduledTime == null)
      {
        mScheduledTime.setVisibility(View.GONE);
        mStatusLabel.setVisibility(View.GONE);
        // Reset to default text appearance color
        mArrivalTime.setTextAppearance(R.style.MwmTextAppearance_Body2_Primary);
      }
      else
      {
        mScheduledTime.setVisibility(View.VISIBLE);
        mScheduledTime.setText(item.scheduledTime);
        mScheduledTime.setPaintFlags(mScheduledTime.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);

        mStatusLabel.setVisibility(View.VISIBLE);
        if (item.status == ArrivalStatus.DELAYED)
        {
          mStatusLabel.setText(R.string.schedule_delayed);
          mStatusLabel.setTextColor(ContextCompat.getColor(itemView.getContext(), R.color.base_red));
          mArrivalTime.setTextColor(ContextCompat.getColor(itemView.getContext(), R.color.base_red));
        }
        else // EARLY
        {
          mStatusLabel.setText(R.string.schedule_early);
          mStatusLabel.setTextColor(ContextCompat.getColor(itemView.getContext(), R.color.base_green));
          mArrivalTime.setTextColor(ContextCompat.getColor(itemView.getContext(), R.color.base_green));
        }
      }

      // Show dynamic indicator for real-time data (INVISIBLE to reserve space)
      mDynamicIndicator.setVisibility(item.isDynamic ? View.VISIBLE : View.INVISIBLE);
    }
  }
}
