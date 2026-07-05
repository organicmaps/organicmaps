package app.organicmaps.widget.placepage.sections;

import static app.organicmaps.editor.data.TimeFormatUtils.formatWeekdaysRange;

import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.editor.data.Timetable;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.List;
public class PlaceOpeningHoursAdapter extends RecyclerView.Adapter<PlaceOpeningHoursAdapter.ViewHolder>
{
  private List<WeekScheduleData> mWeekSchedule = Collections.emptyList();

  public PlaceOpeningHoursAdapter() {}

  public void setTimetables(Timetable[] timetables, int currentDayOfWeek)
  {
    final List<Integer> weekDays = buildWeekByFirstDay(currentDayOfWeek);
    final List<WeekScheduleData> scheduleData = new ArrayList<>();

    // Timetables array contains only working days. We need to fill non-working gaps.
    for (int i = 0; i < weekDays.size(); i++)
    {
      final int weekDay = weekDays.get(i);

      final Timetable tt = findScheduleForWeekDay(timetables, weekDay);
      final int startWeekDay = weekDays.get(i);
      if (tt != null)
      {
        while (i < weekDays.size() && tt.containsWeekday(weekDays.get(i)))
          i++;

        i--;
      }
      else
      {
        // Search next working day in timetables.
        while (i + 1 < weekDays.size())
        {
          if (findScheduleForWeekDay(timetables, weekDays.get(i + 1)) != null)
            break;
          i++;
        }
      }

      final int endWeekDay = weekDays.get(i);
      final boolean isBold = (startWeekDay <= endWeekDay)
                               ? (startWeekDay <= currentDayOfWeek && currentDayOfWeek <= endWeekDay)
                               : (currentDayOfWeek >= startWeekDay || currentDayOfWeek <= endWeekDay);
      scheduleData.add(new WeekScheduleData(startWeekDay, endWeekDay, tt, isBold));
    }

    mWeekSchedule = scheduleData;

    notifyDataSetChanged();
  }

  public static List<Integer> buildWeekByFirstDay(int firstDayOfWeek)
  {
    if (firstDayOfWeek < 1 || firstDayOfWeek > 7)
      throw new IllegalArgumentException("First day of week " + firstDayOfWeek + " is out of range [1..7]");

    final List<Integer> list = Arrays.asList(Calendar.SUNDAY, Calendar.MONDAY, Calendar.TUESDAY, Calendar.WEDNESDAY,
                                             Calendar.THURSDAY, Calendar.FRIDAY, Calendar.SATURDAY);
    Collections.rotate(list, 1 - firstDayOfWeek);
    return list;
  }

  public static Timetable findScheduleForWeekDay(Timetable[] tables, int weekDay)
  {
    for (Timetable tt : tables)
      if (tt.containsWeekday(weekDay))
        return tt;

    return null;
  }

  @NonNull
  @Override
  public ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType)
  {
    return new ViewHolder(
        LayoutInflater.from(parent.getContext()).inflate(R.layout.place_page_opening_hours_item, parent, false));
  }

  @Override
  public void onBindViewHolder(@NonNull ViewHolder holder, int position)
  {
    if (mWeekSchedule == null || position >= mWeekSchedule.size() || position < 0)
      return;

    final WeekScheduleData schedule = mWeekSchedule.get(position);

    holder.setBoldStyle(schedule.isBold);
    holder.setWeekdays(formatWeekdaysRange(schedule.startWeekDay, schedule.endWeekDay));

    final String openTime;
    if (schedule.isClosed)
      openTime = holder.itemView.getResources().getString(R.string.day_off);
    else if (schedule.timetable.isFullday)
      openTime = holder.itemView.getResources().getString(R.string.editor_time_allday);
    else
    {
      final String shifts = schedule.timetable.formatOpenShifts("\n");
      // A working day fully covered by breaks has no open shift; show it as closed.
      openTime = shifts.isEmpty() ? holder.itemView.getResources().getString(R.string.day_off) : shifts;
    }

    holder.setOpenTime(openTime);
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
    public final boolean isBold;

    public WeekScheduleData(int startWeekDay, int endWeekDay, Timetable timetable, boolean isBold)
    {
      this.startWeekDay = startWeekDay;
      this.endWeekDay = endWeekDay;
      this.isClosed = timetable == null;
      this.timetable = timetable;
      this.isBold = isBold;
    }
  }

  public static class ViewHolder extends RecyclerView.ViewHolder
  {
    private final TextView mWeekdays;
    private final TextView mOpenTime;

    public ViewHolder(@NonNull View itemView)
    {
      super(itemView);
      mWeekdays = itemView.findViewById(R.id.tv__opening_hours_weekdays);
      mOpenTime = itemView.findViewById(R.id.tv__opening_hours_time);
      itemView.setVisibility(View.VISIBLE);
    }

    public void setBoldStyle(boolean isBold)
    {
      final int style = isBold ? R.style.MwmTextAppearance_PlacePage : R.style.MwmTextAppearance_Body3;
      mWeekdays.setTextAppearance(itemView.getContext(), style);
      mOpenTime.setTextAppearance(itemView.getContext(), style);
    }

    public void setWeekdays(String weekdays)
    {
      mWeekdays.setText(weekdays);
    }

    public void setOpenTime(String openTime)
    {
      mOpenTime.setText(openTime);
    }
  }
}
