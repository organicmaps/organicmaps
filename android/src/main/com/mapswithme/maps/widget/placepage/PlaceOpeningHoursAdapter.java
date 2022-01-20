package com.mapswithme.maps.widget.placepage;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.R;
import static com.mapswithme.maps.editor.data.TimeFormatUtils.formatWeekdays;
import static com.mapswithme.maps.editor.data.TimeFormatUtils.formatNonBusinessTime;

import com.mapswithme.maps.editor.data.Timespan;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collections;
import java.util.List;

public class PlaceOpeningHoursAdapter extends RecyclerView.Adapter<PlaceOpeningHoursAdapter.ViewHolder>
{
  private List<WeekScheduleData> mWeekSchedule = Collections.emptyList();

  public PlaceOpeningHoursAdapter() {}

  public PlaceOpeningHoursAdapter(Timetable[] timetables, int firstDayOfWeek)
  {
    setTimetables(timetables, firstDayOfWeek);
  }

  public void setTimetables(Timetable[] timetables, int firstDayOfWeek)
  {
    int[] weekDays = null;
    if (firstDayOfWeek == Calendar.SUNDAY)
      weekDays = new int[]{1, 2, 3, 4, 5, 6, 7};
    else
      weekDays = new int[]{2, 3, 4, 5, 6, 7, 1};

    final List<WeekScheduleData> scheduleData = new ArrayList<>();

    // timetables array contains only working days. We need to fill non working gaps
    for (int idx=0; idx < weekDays.length; idx++)
    {
      int weekDay = weekDays[idx];
      Timetable tt = findScheduleForWeekDay(timetables, weekDay);
      if (tt != null)
      {
        scheduleData.add(new WeekScheduleData(tt.weekdays, false, tt));
        while(idx < weekDays.length && tt.containsWeekday(weekDays[idx]))
          idx++;

        idx--;
      }
      else
      {
        int endDate = weekDays[idx];
        idx ++;
        while (idx < weekDays.length)
        {
          endDate = weekDays[idx];
          Timetable tt2 = findScheduleForWeekDay(timetables, endDate);
          if (tt2 != null)
          {
            idx --;
            endDate = weekDays[idx];
            break;
          }
          idx ++;
        }

        int[] closedDatesRange = createRange(weekDay, endDate);
        scheduleData.add(new WeekScheduleData(closedDatesRange, true, null));
      }
    }

    mWeekSchedule = scheduleData;

    //mTimetables = timetables;
    //findUnhandledDays(timetables);
    notifyDataSetChanged();
  }

  public static Timetable findScheduleForWeekDay(Timetable[] tables, int weekDay)
  {
    for(Timetable tt : tables)
      if (tt.containsWeekday(weekDay))
        return tt;

    return null;
  }

  public static int[] createRange(int start, int end)
  {
    int[] result = new int[end-start+1];
    for (int i=start; i <= end; i++)
      result[i-start] = i;
    return result;
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return new ViewHolder(LayoutInflater.from(parent.getContext())
                                        .inflate(R.layout.place_page_opening_hours_item, parent, false));
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position)
  {
    if (mWeekSchedule == null || position >= mWeekSchedule.size() || position < 0)
      return;

    final WeekScheduleData schedule = mWeekSchedule.get(position);

    if (schedule.isClosed)
    {
      holder.setWeekdays(formatWeekdays(schedule.weekDays));
      holder.setOpenTime(holder.itemView.getResources().getString(R.string.day_off));
      holder.hideNonBusinessTime();
      return;
    }

    final Timetable tt = schedule.timetable;

    final String weekdays = formatWeekdays(tt);
    String workingTime = tt.isFullday ? holder.itemView.getResources().getString(R.string.editor_time_allday)
                                      : tt.workingTimespan.toWideString();

    holder.setWeekdays(weekdays);
    holder.setOpenTime(workingTime);

    final Timespan[] closedTime = tt.closedTimespans;
    if (closedTime == null || closedTime.length == 0)
    {
      holder.hideNonBusinessTime();
    }
    else
    {
      final String hoursNonBusinessLabel = holder.itemView.getResources().getString(R.string.editor_hours_closed);
      holder.setNonBusinessTime(formatNonBusinessTime(closedTime, hoursNonBusinessLabel));
    }
  }

  @Override
  public int getItemCount()
  {
    return (mWeekSchedule != null ? mWeekSchedule.size() : 0);
  }

  private static class WeekScheduleData
  {
    public final int[] weekDays;
    public final boolean isClosed;
    public final Timetable timetable;

    public WeekScheduleData(int[] weekDays, boolean isClosed, Timetable timetable)
    {
      if(!isClosed && timetable == null)
        throw new IllegalArgumentException("timetable parameter is null while isClosed = false");

      this.weekDays = weekDays;
      this.isClosed = isClosed;
      this.timetable = timetable;
    }
  }

  public static class ViewHolder extends RecyclerView.ViewHolder
  {
    private final TextView mWeekdays;
    private final TextView mOpenTime;
    private final TextView mNonBusinessTime;

    public ViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mWeekdays = itemView.findViewById(R.id.tv__opening_hours_weekdays);
      mOpenTime = itemView.findViewById(R.id.tv__opening_hours_time);
      mNonBusinessTime = itemView.findViewById(R.id.tv__opening_hours_nonbusiness_time);
      itemView.setVisibility(View.VISIBLE);
    }

    public void setWeekdays(String weekdays)
    {
      mWeekdays.setText(weekdays);
    }

    public void setOpenTime(String openTime)
    {
      mOpenTime.setText(openTime);
    }

    public void setNonBusinessTime(String nonBusinessTime)
    {
      UiUtils.setTextAndShow(mNonBusinessTime, nonBusinessTime);
    }

    public void hideNonBusinessTime()
    {
      UiUtils.clearTextAndHide(mNonBusinessTime);
    }
  }
}
