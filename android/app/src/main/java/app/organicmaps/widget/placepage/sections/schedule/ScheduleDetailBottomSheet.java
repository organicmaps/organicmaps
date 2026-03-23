package app.organicmaps.widget.placepage.sections.schedule;

import android.app.DatePickerDialog;
import android.content.res.ColorStateList;
import android.graphics.Paint;
import android.graphics.drawable.GradientDrawable;
import android.os.Bundle;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.GridLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.widget.ImageViewCompat;
import app.organicmaps.R;
import com.google.android.material.bottomsheet.BottomSheetBehavior;
import com.google.android.material.bottomsheet.BottomSheetDialogFragment;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;

/**
 * Bottom sheet dialog showing full schedule for a route direction.
 */
public class ScheduleDetailBottomSheet extends BottomSheetDialogFragment
{
  private static final String ARG_ROUTE_NUMBER = "route_number";
  private static final String ARG_ROUTE_COLOR = "route_color";
  private static final String ARG_TRANSPORT_TYPE = "transport_type";
  private static final String ARG_DIRECTION = "direction";

  private GridLayout mGridScheduleTimes;
  private TextView mSelectedDateText;
  private ImageView mPrevDayButton;
  private ImageView mNextDayButton;
  private Calendar mSelectedDate = Calendar.getInstance();
  private Calendar mToday = Calendar.getInstance();

  public static ScheduleDetailBottomSheet newInstance(@NonNull ScheduleRoute route,
                                                      @NonNull ScheduleDirection direction)
  {
    ScheduleDetailBottomSheet fragment = new ScheduleDetailBottomSheet();
    Bundle args = new Bundle();
    args.putString(ARG_ROUTE_NUMBER, route.routeNumber);
    args.putInt(ARG_ROUTE_COLOR, route.color);
    args.putString(ARG_TRANSPORT_TYPE, route.transportType.name());
    args.putString(ARG_DIRECTION, direction.destination);
    fragment.setArguments(args);
    return fragment;
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_schedule_detail, container, false);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    View view = getView();
    if (view == null)
      return;

    View parent = (View) view.getParent();
    BottomSheetBehavior<View> behavior = BottomSheetBehavior.from(parent);
    behavior.setState(BottomSheetBehavior.STATE_EXPANDED);
    behavior.setSkipCollapsed(true);

    ViewGroup.LayoutParams layoutParams = parent.getLayoutParams();
    layoutParams.height = ViewGroup.LayoutParams.MATCH_PARENT;
    parent.setLayoutParams(layoutParams);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    Bundle args = getArguments();
    if (args == null)
    {
      dismiss();
      return;
    }

    String routeNumber = args.getString(ARG_ROUTE_NUMBER, "");
    int routeColor = args.getInt(ARG_ROUTE_COLOR);
    String transportTypeName = args.getString(ARG_TRANSPORT_TYPE, TransportType.BUS.name());
    String direction = args.getString(ARG_DIRECTION, "");
    TransportType transportType = TransportType.valueOf(transportTypeName);

    mGridScheduleTimes = view.findViewById(R.id.grid_schedule_times);
    mSelectedDateText = view.findViewById(R.id.tv_selected_date);
    mPrevDayButton = view.findViewById(R.id.iv_prev_day);
    mNextDayButton = view.findViewById(R.id.iv_next_day);

    setupHeader(view, routeNumber, routeColor, transportType, direction);
    setupDaySelector();
    updateScheduleTimes();
  }

  private void setupHeader(View view, String routeNumber, int routeColor, TransportType transportType, String direction)
  {
    ImageView transportIcon = view.findViewById(R.id.iv_transport_icon);
    TextView routeNumberView = view.findViewById(R.id.tv_route_number);
    TextView directionView = view.findViewById(R.id.tv_direction);

    // Transport icon
    int iconRes = getTransportIconRes(transportType);
    transportIcon.setImageResource(iconRes);

    TypedValue typedValue = new TypedValue();
    requireContext().getTheme().resolveAttribute(android.R.attr.textColorPrimary, typedValue, true);
    int color = ContextCompat.getColor(requireContext(), typedValue.resourceId);
    ImageViewCompat.setImageTintList(transportIcon, ColorStateList.valueOf(color));

    // Route number badge
    routeNumberView.setText(routeNumber);
    GradientDrawable background = new GradientDrawable();
    background.setShape(GradientDrawable.RECTANGLE);
    background.setCornerRadius(16f);
    background.setColor(routeColor);
    routeNumberView.setBackground(background);

    // Direction
    directionView.setText(direction);
  }

  private void setupDaySelector()
  {
    mPrevDayButton.setOnClickListener(v -> navigateDay(-1));
    mNextDayButton.setOnClickListener(v -> navigateDay(1));
    mSelectedDateText.setOnClickListener(v -> showDatePicker());

    updateDateDisplay();
  }

  private void navigateDay(int delta)
  {
    Calendar newDate = (Calendar) mSelectedDate.clone();
    newDate.add(Calendar.DAY_OF_YEAR, delta);

    // Don't allow going before today
    if (newDate.before(mToday) && !isSameDay(newDate, mToday))
      return;

    mSelectedDate = newDate;
    updateDateDisplay();
    updateScheduleTimes();
  }

  private void showDatePicker()
  {
    DatePickerDialog dialog = new DatePickerDialog(requireContext(), (view, year, month, dayOfMonth) -> {
      Calendar selected = Calendar.getInstance();
      selected.set(year, month, dayOfMonth);
      mSelectedDate = selected;
      updateDateDisplay();
      updateScheduleTimes();
    }, mSelectedDate.get(Calendar.YEAR), mSelectedDate.get(Calendar.MONTH), mSelectedDate.get(Calendar.DAY_OF_MONTH));

    // Set min date to today
    dialog.getDatePicker().setMinDate(mToday.getTimeInMillis() - 1000);

    dialog.show();
  }

  private void updateDateDisplay()
  {
    String dateLabel = formatDateLabel(mSelectedDate);
    mSelectedDateText.setText(dateLabel);

    // Disable prev button if on today
    boolean isToday = isSameDay(mSelectedDate, mToday);
    mPrevDayButton.setAlpha(isToday ? 0.3f : 1.0f);
    mPrevDayButton.setEnabled(!isToday);
  }

  private String formatDateLabel(Calendar date)
  {
    Calendar tomorrow = (Calendar) mToday.clone();
    tomorrow.add(Calendar.DAY_OF_YEAR, 1);

    if (isSameDay(date, mToday))
    {
      SimpleDateFormat format = new SimpleDateFormat("MMM d", Locale.getDefault());
      return getString(R.string.schedule_today) + ", " + format.format(date.getTime());
    }
    else if (isSameDay(date, tomorrow))
    {
      SimpleDateFormat format = new SimpleDateFormat("MMM d", Locale.getDefault());
      return getString(R.string.schedule_tomorrow) + ", " + format.format(date.getTime());
    }
    else
    {
      SimpleDateFormat format = new SimpleDateFormat("EEE, MMM d", Locale.getDefault());
      return format.format(date.getTime());
    }
  }

  private boolean isSameDay(Calendar cal1, Calendar cal2)
  {
    return cal1.get(Calendar.YEAR) == cal2.get(Calendar.YEAR)
 && cal1.get(Calendar.DAY_OF_YEAR) == cal2.get(Calendar.DAY_OF_YEAR);
  }

  private int getDayOffset()
  {
    long diffMillis = mSelectedDate.getTimeInMillis() - mToday.getTimeInMillis();
    return (int) (diffMillis / (24 * 60 * 60 * 1000));
  }

  private void updateScheduleTimes()
  {
    mGridScheduleTimes.removeAllViews();

    int dayOffset = getDayOffset();
    List<ScheduleTimeEntry> entries = generateMockScheduleEntries(dayOffset);

    int paddingHorizontalDp = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 4,
                                                              requireContext().getResources().getDisplayMetrics());

    int paddingVerticalDp = (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 8,
                                                            requireContext().getResources().getDisplayMetrics());

    TypedValue typedValue = new TypedValue();
    requireContext().getTheme().resolveAttribute(android.R.attr.textColorPrimary, typedValue, true);
    int defaultTextColor = ContextCompat.getColor(requireContext(), typedValue.resourceId);

    requireContext().getTheme().resolveAttribute(android.R.attr.textColorSecondary, typedValue, true);
    int secondaryTextColor = ContextCompat.getColor(requireContext(), typedValue.resourceId);

    for (int i = 0; i < entries.size(); i++)
    {
      ScheduleTimeEntry entry = entries.get(i);

      // Container for scheduled + actual time
      LinearLayout container = new LinearLayout(requireContext());
      container.setOrientation(LinearLayout.VERTICAL);
      container.setGravity(Gravity.CENTER);
      container.setPadding(paddingHorizontalDp, paddingVerticalDp, paddingHorizontalDp, paddingVerticalDp);

      if (entry.actualTime != null)
      {
        // Has delay or early - show both times
        // Scheduled time (strikethrough)
        TextView scheduledView = new TextView(requireContext());
        scheduledView.setText(entry.scheduledTime);
        scheduledView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11);
        scheduledView.setGravity(Gravity.CENTER);
        scheduledView.setTextColor(secondaryTextColor);
        scheduledView.setPaintFlags(scheduledView.getPaintFlags() | Paint.STRIKE_THRU_TEXT_FLAG);
        container.addView(scheduledView);

        // Actual time
        TextView actualView = new TextView(requireContext());
        actualView.setText(entry.actualTime);
        actualView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        actualView.setGravity(Gravity.CENTER);
        actualView.setTextColor(entry.status == ArrivalStatus.DELAYED
                                    ? ContextCompat.getColor(requireContext(), R.color.base_red)
                                    : ContextCompat.getColor(requireContext(), R.color.light_green));
        container.addView(actualView);
      }
      else
      {
        // On time - show only scheduled
        TextView timeView = new TextView(requireContext());
        timeView.setText(entry.scheduledTime);
        timeView.setTextSize(TypedValue.COMPLEX_UNIT_SP, 14);
        timeView.setGravity(Gravity.CENTER);
        timeView.setTextColor(defaultTextColor);
        container.addView(timeView);
      }

      GridLayout.LayoutParams params = new GridLayout.LayoutParams();
      params.columnSpec = GridLayout.spec(i % 4, 1f);
      params.rowSpec = GridLayout.spec(i / 4);
      params.width = 0;
      container.setLayoutParams(params);

      mGridScheduleTimes.addView(container);
    }
  }

  private List<ScheduleTimeEntry> generateMockScheduleEntries(int dayOffset)
  {
    List<ScheduleTimeEntry> entries = new ArrayList<>();

    String[] baseTimes = {"06:15", "06:45", "07:15", "07:45", "08:15", "08:45", "09:15", "09:45", "10:15",
                          "10:45", "11:15", "11:45", "12:15", "12:45", "13:15", "13:45", "14:15", "14:45",
                          "15:15", "15:45", "16:15", "16:45", "17:15", "17:45", "18:15", "18:45", "19:15",
                          "19:45", "20:15", "20:45", "21:15", "21:45", "22:15", "22:45", "23:15", "23:45"};

    // Only today (dayOffset == 0) has real-time status variations
    if (dayOffset == 0)
    {
      // Hardcoded examples for demo
      for (int i = 0; i < baseTimes.length; i++)
      {
        String time = baseTimes[i];
        ArrivalStatus status = ArrivalStatus.ON_TIME;
        String actualTime = null;

        // Delayed examples: indices 2, 7, 15, 22
        if (i == 2)
        {
          status = ArrivalStatus.DELAYED;
          actualTime = addMinutes(time, 8); // +8 min
        }
        else if (i == 7)
        {
          status = ArrivalStatus.DELAYED;
          actualTime = addMinutes(time, 5); // +5 min
        }
        else if (i == 15)
        {
          status = ArrivalStatus.DELAYED;
          actualTime = addMinutes(time, 12); // +12 min
        }
        else if (i == 22)
        {
          status = ArrivalStatus.DELAYED;
          actualTime = addMinutes(time, 4); // +4 min
        }
        // Early examples: indices 4, 10, 18, 25, 30
        else if (i == 4)
        {
          status = ArrivalStatus.EARLY;
          actualTime = addMinutes(time, -2); // -2 min
        }
        else if (i == 10)
        {
          status = ArrivalStatus.EARLY;
          actualTime = addMinutes(time, -3); // -3 min
        }
        else if (i == 18)
        {
          status = ArrivalStatus.EARLY;
          actualTime = addMinutes(time, -1); // -1 min
        }
        else if (i == 25)
        {
          status = ArrivalStatus.EARLY;
          actualTime = addMinutes(time, -4); // -4 min
        }
        else if (i == 30)
        {
          status = ArrivalStatus.EARLY;
          actualTime = addMinutes(time, -2); // -2 min
        }

        entries.add(new ScheduleTimeEntry(time, actualTime, status));
      }
    }
    else
    {
      // Future days - all on time (no real-time data)
      for (String time : baseTimes)
      {
        entries.add(new ScheduleTimeEntry(time, null, ArrivalStatus.ON_TIME));
      }
    }

    return entries;
  }

  private String addMinutes(String time, int minutes)
  {
    try
    {
      String[] parts = time.split(":");
      int hours = Integer.parseInt(parts[0]);
      int mins = Integer.parseInt(parts[1]);

      mins += minutes;
      while (mins >= 60)
      {
        mins -= 60;
        hours++;
      }
      while (mins < 0)
      {
        mins += 60;
        hours--;
      }
      hours = (hours + 24) % 24;

      return String.format(Locale.US, "%02d:%02d", hours, mins);
    }
    catch (Exception e)
    {
      return time;
    }
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

  /**
   * Represents a single time entry with scheduled and actual times.
   */
  private static class ScheduleTimeEntry
  {
    final String scheduledTime;
    @Nullable
    final String actualTime;
    final ArrivalStatus status;

    ScheduleTimeEntry(String scheduledTime, @Nullable String actualTime, ArrivalStatus status)
    {
      this.scheduledTime = scheduledTime;
      this.actualTime = actualTime;
      this.status = status;
    }
  }
}
