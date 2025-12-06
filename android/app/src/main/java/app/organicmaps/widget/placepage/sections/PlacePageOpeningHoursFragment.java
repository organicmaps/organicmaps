package app.organicmaps.widget.placepage.sections;

import android.content.res.Resources;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.editor.data.TimeFormatUtils;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.editor.OpeningHours;
import app.organicmaps.sdk.editor.data.OpeningHoursInfo;
import app.organicmaps.sdk.editor.data.Timespan;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.util.Calendar;
import java.util.Locale;
import java.util.TimeZone;

public class PlacePageOpeningHoursFragment extends Fragment implements Observer<MapObject>
{
  private View mFrame;
  private TextView mTodayLabel;
  private TextView mTodayOpenTime;
  private TextView mTodayNonBusinessTime;
  private RecyclerView mFullWeekOpeningHours;
  private PlaceOpeningHoursAdapter mOpeningHoursAdapter;

  private PlacePageViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_opening_hours_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mFrame = view;
    mTodayLabel = view.findViewById(R.id.oh_today_label);
    mTodayOpenTime = view.findViewById(R.id.oh_today_open_time);
    mTodayNonBusinessTime = view.findViewById(R.id.oh_nonbusiness_time);
    mFullWeekOpeningHours = view.findViewById(R.id.rw__full_opening_hours);
    mOpeningHoursAdapter = new PlaceOpeningHoursAdapter();
    mFullWeekOpeningHours.setAdapter(mOpeningHoursAdapter);
  }

  private void refreshTodayNonBusinessTime(Timespan[] closedTimespans)
  {
    final String hoursClosedLabel = getResources().getString(R.string.editor_hours_closed);
    if (closedTimespans == null || closedTimespans.length == 0)
      UiUtils.clearTextAndHide(mTodayNonBusinessTime);
    else
      UiUtils.setTextAndShow(mTodayNonBusinessTime,
                             TimeFormatUtils.formatNonBusinessTime(closedTimespans, hoursClosedLabel));
  }

  private void refreshTodayOpeningHours(String label, String openTime, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayLabel, label);
    UiUtils.setTextAndShow(mTodayOpenTime, openTime);
    mTodayLabel.setTextColor(color);
    mTodayOpenTime.setTextColor(color);
  }

  private void refreshTodayOpeningHours(String label, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayLabel, label);
    UiUtils.hide(mTodayOpenTime);
    mTodayLabel.setTextColor(color);
    mTodayOpenTime.setTextColor(color);
  }

  private void refreshOpeningHours(MapObject mapObject)
  {
    final String ohStr = mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
    mFrame.setOnLongClickListener((v) -> {
      PlacePageUtils.copyToClipboard(requireContext(), mFrame,
          TimeFormatUtils.formatTimetables(getResources(), ohStr, timetables));
      return true;
    });

    // Public holiday handling
    long[] holidays = mapObject.getPublicHolidays();
    java.util.Map<Long, String> holidayNames = mapObject.getPublicHolidayNames();
    final long currentTime = System.currentTimeMillis() / 1000;
    final OpeningHoursInfo ohInfo =
        (ohStr != null && !ohStr.isEmpty()) ? OpeningHours.nativeGetInfo(ohStr, holidays, currentTime) : null;
    final boolean isHoliday = (ohInfo != null && ohInfo.isHoliday());
    final boolean hasPHRules = ohStr != null && ohStr.contains("PH");
    String holidayName = null;
    if (isHoliday && hasPHRules && holidayNames != null && !holidayNames.isEmpty())
    {
      Calendar today = Calendar.getInstance(TimeZone.getTimeZone("UTC"));
      today.set(Calendar.HOUR_OF_DAY, 0);
      today.set(Calendar.MINUTE, 0);
      today.set(Calendar.SECOND, 0);
      today.set(Calendar.MILLISECOND, 0);
      long todayTs = today.getTimeInMillis() / 1000;
      holidayName = holidayNames.getOrDefault(todayTs, null);
      if (holidayName == null)
      {
        long todayDays = todayTs / 86400L;
        for (java.util.Map.Entry<Long, String> e : holidayNames.entrySet())
        {
          if (e.getKey() / 86400L == todayDays)
          {
            holidayName = e.getValue();
            break;
          }
        }
      }
    }

    final boolean isEmptyTT = (timetables == null || timetables.length == 0);
    final int color = ThemeUtils.getColor(requireContext(), android.R.attr.textColorPrimary);
    final Resources resources = getResources();

    if (isEmptyTT)
    {
     // 'opening_hours' tag wasn't parsed either because it's empty or wrong format.
      if (!ohStr.isEmpty())
      {
        UiUtils.show(mFrame);
        refreshTodayOpeningHours(ohStr, color);
        UiUtils.hide(mTodayNonBusinessTime);
        UiUtils.hide(mFullWeekOpeningHours);
      }
      else
      {
        UiUtils.hide(mFrame);
      }
      return;
    }

    UiUtils.show(mFrame);

    if (timetables[0].isFullWeek())
    {
      Timetable tt = timetables[0];
      if (hasPHRules && isHoliday && ohInfo != null)
      {
        if (ohInfo.getState() == OpeningHoursInfo.State.CLOSED)
        {
          String label = resources.getString(R.string.day_off_today);
          if (holidayName != null) label += " (" + holidayName + ")";
          refreshTodayOpeningHours(label, ContextCompat.getColor(requireContext(), R.color.base_red));
          UiUtils.hide(mTodayNonBusinessTime);
        }
        else if (ohInfo.getState() == OpeningHoursInfo.State.OPEN)
        {
          String label = resources.getString(R.string.daily);
          if (holidayName != null) label += " (" + holidayName + ")";
          refreshTodayOpeningHours(label, tt.isFullday ? resources.getString(R.string.twentyfour_seven) : tt.workingTimespan.toWideString(), color);
          refreshTodayNonBusinessTime(tt.closedTimespans);
        }
      }
      else if (tt.isFullday)
      {
        refreshTodayOpeningHours(resources.getString(R.string.twentyfour_seven), color);
        UiUtils.hide(mTodayNonBusinessTime);
      }
      else
      {
        refreshTodayOpeningHours(resources.getString(R.string.daily), tt.workingTimespan.toWideString(), color);
        refreshTodayNonBusinessTime(tt.closedTimespans);
      }
      UiUtils.hide(mFullWeekOpeningHours);
      return;
    }

    // Weekday timetable
    int firstDayOfWeek = Calendar.getInstance(Locale.getDefault()).getFirstDayOfWeek();
    mOpeningHoursAdapter.setTimetables(timetables, firstDayOfWeek);
    UiUtils.show(mFullWeekOpeningHours);

    int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
    boolean containsCurrentWeekday = false;

    for (Timetable tt : timetables)
    {
      if (tt.containsWeekday(currentDay))
      {
        containsCurrentWeekday = true;
        String openTime = tt.isFullday ? Utils.unCapitalize(resources.getString(R.string.editor_time_allday))
                                       : tt.workingTimespan.toWideString();
        if (hasPHRules && isHoliday && ohInfo != null)
        {
          if (ohInfo.getState() == OpeningHoursInfo.State.CLOSED)
          {
            String label = resources.getString(R.string.day_off_today);
            if (holidayName != null) label += " (" + holidayName + ")";
            refreshTodayOpeningHours(label, ContextCompat.getColor(requireContext(), R.color.base_red));
            UiUtils.hide(mTodayNonBusinessTime);
          }
          else if (ohInfo.getState() == OpeningHoursInfo.State.OPEN)
          {
            String label = resources.getString(R.string.today);
            if (holidayName != null) label += " (" + holidayName + ")";
            refreshTodayOpeningHours(label, openTime, color);
            refreshTodayNonBusinessTime(tt.closedTimespans);
          }
        }
        else
        {
          refreshTodayOpeningHours(resources.getString(R.string.today), openTime, color);
          refreshTodayNonBusinessTime(tt.closedTimespans);
        }
        break;
      }
    }

    if (!containsCurrentWeekday)
    {
      if (hasPHRules && isHoliday && ohInfo != null && ohInfo.getState() == OpeningHoursInfo.State.CLOSED)
      {
        String label = resources.getString(R.string.day_off_today);
        if (holidayName != null) label += " (" + holidayName + ")";
        refreshTodayOpeningHours(label, ContextCompat.getColor(requireContext(), R.color.base_red));
      }
      else if (hasPHRules && isHoliday && ohInfo != null && ohInfo.getState() == OpeningHoursInfo.State.OPEN)
      {
        String label = resources.getString(R.string.today);
        if (holidayName != null) label += " (" + holidayName + ")";
        refreshTodayOpeningHours(label, ContextCompat.getColor(requireContext(), R.color.base_green));
      }
      else
      {
        refreshTodayOpeningHours(resources.getString(R.string.day_off_today),
                                 ContextCompat.getColor(requireContext(), R.color.base_red));
      }
      UiUtils.hide(mTodayNonBusinessTime);
    }
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject != null)
      refreshOpeningHours(mapObject);
  }
}
