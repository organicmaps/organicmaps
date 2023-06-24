package app.organicmaps.widget.placepage.sections;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.editor.data.Timespan;
import app.organicmaps.editor.data.Timetable;
import app.organicmaps.util.UiUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.List;

import static app.organicmaps.editor.data.TimeFormatUtils.formatNonBusinessTime;
import static app.organicmaps.editor.data.TimeFormatUtils.formatWeekdaysRange;

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
    final List<Integer> weekDays = buildWeekByFirstDay(firstDayOfWeek);
    final List<WeekScheduleData> scheduleData = new ArrayList<>();

    // Timetables array contains only working days. We need to fill non-working gaps.
    for (int i = 0; i < weekDays.size(); i++)
    {
      final int weekDay = weekDays.get(i);

      final Timetable tt = findScheduleForWeekDay(timetables, weekDay);
      if (tt != null)
      {
        final int startWeekDay = weekDays.get(i);
        while (i < weekDays.size() && tt.containsWeekday(weekDays.get(i)))
          i++;

        i--;
        final int endWeekDay = weekDays.get(i);
        scheduleData.add(new WeekScheduleData(startWeekDay, endWeekDay, tt));
      }
      else
      {
        final int startWeekDay = weekDays.get(i);
        // Search next working day in timetables.
        while (i + 1 < weekDays.size())
        {
          if (findScheduleForWeekDay(timetables, weekDays.get(i + 1)) != null)
            break;
          i++;
        }

        scheduleData.add(new WeekScheduleData(startWeekDay, weekDays.get(i), null));
      }
    }

    mWeekSchedule = scheduleData;

    notifyDataSetChanged();
  }

  public static List<Integer> buildWeekByFirstDay(int firstDayOfWeek)
  {
    if (firstDayOfWeek < 1 || firstDayOfWeek > 7)
      throw new IllegalArgumentException("First day of week "+firstDayOfWeek+" is out of range [1..7]");

    final List<Integer> list = Arrays.asList(Calendar.SUNDAY, Calendar.MONDAY, Calendar.TUESDAY,
                                             Calendar.WEDNESDAY, Calendar.THURSDAY, Calendar.FRIDAY,
                                             Calendar.SATURDAY);
    Collections.rotate(list, 1 - firstDayOfWeek);
    return list;
  }

  public static Timetable findScheduleForWeekDay(Timetable[] tables, int weekDay)
  {
    for(Timetable tt : tables)
      if (tt.containsWeekday(weekDay))
        return tt;

    return null;
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
      holder.setWeekdays(formatWeekdaysRange(schedule.startWeekDay, schedule.endWeekDay));
      holder.setOpenTime(holder.itemView.getResources().getString(R.string.day_off));
      holder.hideNonBusinessTime();
      return;
    }

    final Timetable tt = schedule.timetable;

    String workingTime = tt.isFullday ? holder.itemView.getResources().getString(R.string.editor_time_allday)
                                      : tt.workingTimespan.toWideString();

    holder.setWeekdays(formatWeekdaysRange(schedule.startWeekDay, schedule.endWeekDay));
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

  public static class WeekScheduleData
  {
    public final int startWeekDay;
    public final int endWeekDay;
    public final boolean isClosed;
    public final Timetable timetable;

    public WeekScheduleData(int startWeekDay, int endWeekDay, Timetable timetable)
    {
      this.startWeekDay = startWeekDay;
      this.endWeekDay = endWeekDay;
      this.isClosed = timetable == null;
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
