package app.organicmaps.widget.placepage.sections.schedule;

import android.content.res.ColorStateList;
import android.graphics.drawable.GradientDrawable;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.widget.ImageViewCompat;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import java.util.ArrayList;
import java.util.List;

/**
 * Adapter for grouped schedule display with sections, route headers, and directions.
 * Uses a flattened list approach for RecyclerView with multiple view types.
 */
public class GroupedScheduleAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private static final int TYPE_SECTION_HEADER = 0;
  private static final int TYPE_ROUTE_HEADER = 1;
  private static final int TYPE_DIRECTION = 2;
  private static final int TYPE_DIVIDER = 3;
  private static final int TYPE_SPACER = 4;

  private static final Object DIVIDER_ITEM = new Object();
  private static final Object SPACER_ITEM = new Object();

  private final List<Object> mFlattenedItems = new ArrayList<>();
  private final List<ScheduleSection> mSections = new ArrayList<>();

  @Nullable
  private OnPinClickListener mPinClickListener;
  @Nullable
  private OnRouteDetailsClickListener mDetailsClickListener;

  public interface OnPinClickListener
  {
    void onPinClick(ScheduleRoute route);
  }

  public interface OnRouteDetailsClickListener
  {
    void onDetailsClick(ScheduleRoute route, ScheduleDirection direction);
  }

  public void setOnPinClickListener(@Nullable OnPinClickListener listener)
  {
    mPinClickListener = listener;
  }

  public void setOnRouteDetailsClickListener(@Nullable OnRouteDetailsClickListener listener)
  {
    mDetailsClickListener = listener;
  }

  public void setSections(@NonNull List<ScheduleSection> sections)
  {
    mSections.clear();
    mSections.addAll(sections);
    flattenItems();
    notifyDataSetChanged();
  }

  private void flattenItems()
  {
    mFlattenedItems.clear();
    boolean hasPreviousSection = false;

    for (ScheduleSection section : mSections)
    {
      if (section.routes.isEmpty())
        continue;

      // Add spacer and divider between sections (after pinned section)
      if (hasPreviousSection)
      {
        mFlattenedItems.add(SPACER_ITEM);
        mFlattenedItems.add(DIVIDER_ITEM);
      }

      // Only add section header if it has a title
      if (section.hasTitle())
        mFlattenedItems.add(section);

      for (ScheduleRoute route : section.routes)
      {
        mFlattenedItems.add(route);
        for (ScheduleDirection direction : route.directions)
        {
          mFlattenedItems.add(new DirectionItem(route, direction));
        }
      }

      hasPreviousSection = true;
    }
  }

  @Override
  public int getItemViewType(int position)
  {
    Object item = mFlattenedItems.get(position);
    if (item == SPACER_ITEM)
      return TYPE_SPACER;
    if (item == DIVIDER_ITEM)
      return TYPE_DIVIDER;
    if (item instanceof ScheduleSection)
      return TYPE_SECTION_HEADER;
    if (item instanceof ScheduleRoute)
      return TYPE_ROUTE_HEADER;
    if (item instanceof DirectionItem)
      return TYPE_DIRECTION;
    throw new IllegalArgumentException("Unknown item type at position " + position);
  }

  @NonNull
  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    switch (viewType)
    {
    case TYPE_SECTION_HEADER:
      return new SectionHeaderViewHolder(inflater.inflate(R.layout.item_schedule_section_header, parent, false));
    case TYPE_ROUTE_HEADER:
      return new RouteHeaderViewHolder(inflater.inflate(R.layout.item_schedule_route_header, parent, false));
    case TYPE_DIRECTION:
      return new DirectionViewHolder(inflater.inflate(R.layout.item_schedule_direction, parent, false));
    case TYPE_DIVIDER: return new DividerViewHolder(inflater.inflate(R.layout.item_schedule_divider, parent, false));
    case TYPE_SPACER: return new DividerViewHolder(inflater.inflate(R.layout.item_schedule_spacer, parent, false));
    default: throw new IllegalArgumentException("Unknown view type: " + viewType);
    }
  }

  @Override
  public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position)
  {
    Object item = mFlattenedItems.get(position);
    if (holder instanceof SectionHeaderViewHolder && item instanceof ScheduleSection)
      ((SectionHeaderViewHolder) holder).bind((ScheduleSection) item);
    else if (holder instanceof RouteHeaderViewHolder && item instanceof ScheduleRoute)
      ((RouteHeaderViewHolder) holder).bind((ScheduleRoute) item, mPinClickListener, mDetailsClickListener);
    else if (holder instanceof DirectionViewHolder && item instanceof DirectionItem)
      ((DirectionViewHolder) holder).bind((DirectionItem) item);
  }

  @Override
  public int getItemCount()
  {
    return mFlattenedItems.size();
  }

  /**
   * Wrapper for direction items that keeps reference to parent route.
   */
  private static class DirectionItem
  {
    final ScheduleRoute route;
    final ScheduleDirection direction;

    DirectionItem(ScheduleRoute route, ScheduleDirection direction)
    {
      this.route = route;
      this.direction = direction;
    }
  }

  static class SectionHeaderViewHolder extends RecyclerView.ViewHolder
  {
    private final TextView mTitle;
    private final TextView mWalkTime;

    SectionHeaderViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mTitle = itemView.findViewById(R.id.tv_section_title);
      mWalkTime = itemView.findViewById(R.id.tv_walk_time);
    }

    void bind(ScheduleSection section)
    {
      mTitle.setText(section.title);
      if (section.walkTime != null && !section.walkTime.isEmpty())
      {
        mWalkTime.setText(section.walkTime);
        mWalkTime.setVisibility(View.VISIBLE);
      }
      else
      {
        mWalkTime.setVisibility(View.GONE);
      }
    }
  }

  static class RouteHeaderViewHolder extends RecyclerView.ViewHolder
  {
    private final ImageView mTransportIcon;
    private final TextView mRouteNumber;
    private final ImageView mPinIcon;
    private final ImageView mDetailsIcon;

    RouteHeaderViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mTransportIcon = itemView.findViewById(R.id.iv_transport_icon);
      mRouteNumber = itemView.findViewById(R.id.tv_route_number);
      mPinIcon = itemView.findViewById(R.id.iv_pin);
      mDetailsIcon = itemView.findViewById(R.id.iv_details);
    }

    void bind(ScheduleRoute route, @Nullable OnPinClickListener pinListener,
              @Nullable OnRouteDetailsClickListener detailsListener)
    {
      // Set transport icon based on type
      int iconRes = getTransportIconRes(route.transportType);
      mTransportIcon.setImageResource(iconRes);

      // Apply theme-aware tint
      TypedValue typedValue = new TypedValue();
      itemView.getContext().getTheme().resolveAttribute(android.R.attr.textColorPrimary, typedValue, true);
      int color = ContextCompat.getColor(itemView.getContext(), typedValue.resourceId);
      ImageViewCompat.setImageTintList(mTransportIcon, ColorStateList.valueOf(color));

      // Route number badge
      mRouteNumber.setText(route.routeNumber);
      GradientDrawable background = new GradientDrawable();
      background.setShape(GradientDrawable.RECTANGLE);
      background.setCornerRadius(16f);
      background.setColor(route.color);
      mRouteNumber.setBackground(background);

      // Pin icon
      mPinIcon.setImageResource(route.isPinned ? R.drawable.ic_pin : R.drawable.ic_pin_outline);
      ImageViewCompat.setImageTintList(
          mPinIcon, ColorStateList.valueOf(
                        route.isPinned ? ContextCompat.getColor(itemView.getContext(), R.color.base_accent) : color));

      mPinIcon.setOnClickListener(v -> {
        if (pinListener != null)
          pinListener.onPinClick(route);
      });

      // Details arrow
      ImageViewCompat.setImageTintList(mDetailsIcon, ColorStateList.valueOf(color));
      mDetailsIcon.setOnClickListener(v -> {
        if (detailsListener != null && !route.directions.isEmpty())
          detailsListener.onDetailsClick(route, route.directions.get(0));
      });
    }

    private int getTransportIconRes(TransportType type)
    {
      switch (type)
      {
      case TRAM: return R.drawable.ic_category_tram;
      case SUBWAY:
      case TRAIN:
      case FERRY: return R.drawable.ic_category_transport;
      case TROLLEYBUS:
      case BUS:
      default: return R.drawable.ic_category_bus;
      }
    }
  }

  static class DirectionViewHolder extends RecyclerView.ViewHolder
  {
    private final TextView mDestination;
    private final TextView mArrivalTimes;
    private final ImageView mDynamicIndicator;

    DirectionViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mDestination = itemView.findViewById(R.id.tv_destination);
      mArrivalTimes = itemView.findViewById(R.id.tv_arrival_times);
      mDynamicIndicator = itemView.findViewById(R.id.iv_dynamic_indicator);
    }

    void bind(DirectionItem item)
    {
      ScheduleDirection direction = item.direction;
      mDestination.setText(direction.destination);

      // Format arrival times as "16, 46 min"
      StringBuilder timesBuilder = new StringBuilder();
      for (int i = 0; i < direction.arrivals.size() && i < 3; i++)
      {
        if (i > 0)
          timesBuilder.append(", ");
        ScheduleArrival arrival = direction.arrivals.get(i);
        // Extract just the number for compact display
        String time = arrival.arrivalTime.replaceAll("[^0-9]", "");
        timesBuilder.append(time);
      }
      timesBuilder.append(" min");
      mArrivalTimes.setText(timesBuilder.toString());

      // Show dynamic indicator if any arrival has real-time data
      mDynamicIndicator.setVisibility(direction.hasDynamicData() ? View.VISIBLE : View.INVISIBLE);
    }
  }

  static class DividerViewHolder extends RecyclerView.ViewHolder
  {
    DividerViewHolder(@NonNull View itemView)
    {
      super(itemView);
    }
  }
}
