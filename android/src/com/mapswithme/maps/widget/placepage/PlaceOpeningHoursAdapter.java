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
import java.util.Arrays;
import java.util.List;

public class PlaceOpeningHoursAdapter extends RecyclerView.Adapter<PlaceOpeningHoursAdapter.ViewHolder>
{
  private Timetable[] mTimetables = {};
  private int[] closedDays = null;

  public PlaceOpeningHoursAdapter() {}

  public PlaceOpeningHoursAdapter(Timetable[] timetables)
  {
    setTimetables(timetables);
  }

  public void setTimetables(Timetable[] timetables)
  {
    mTimetables = timetables;
    closedDays = findUnhandledDays(timetables);
    notifyDataSetChanged();
  }

  private int[] findUnhandledDays(Timetable[] timetables)
  {
    List<Integer> unhandledDays = new ArrayList<>(Arrays.asList(1, 2, 3, 4, 5, 6, 7));

    for (Timetable tt : timetables) {
      for (int weekDay : tt.weekdays) {
        unhandledDays.remove(Integer.valueOf(weekDay));
      }
    }

    if (unhandledDays.isEmpty())
      return null;

    // Convert List<Integer> to int[].
    int[] days = new int[unhandledDays.size()];
    for (int i = 0; i < days.length; i++)
      days[i] = unhandledDays.get(i);

    return days;
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
    if (mTimetables == null || position > mTimetables.length || position < 0)
      return;

    if (position == mTimetables.length)
    {
      if (closedDays == null)
        return;
      holder.setWeekdays(formatWeekdays(closedDays));
      holder.setOpenTime(holder.itemView.getResources().getString(R.string.day_off));
      holder.hideNonBusinessTime();
      return;
    }

    final Timetable tt = mTimetables[position];

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
    return (mTimetables != null ? mTimetables.length : 0) + (closedDays == null ? 0 : 1);
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
