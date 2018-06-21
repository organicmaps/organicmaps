package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmRecyclerFragment;
import com.mapswithme.maps.editor.data.HoursMinutes;
import com.mapswithme.maps.editor.data.Timetable;

public class SimpleTimetableFragment extends BaseMwmRecyclerFragment<SimpleTimetableAdapter>
                                  implements TimetableFragment.TimetableProvider,
                                             HoursMinutesPickerFragment.OnPickListener
{
  private SimpleTimetableAdapter mAdapter;
  private Timetable[] mInitTts;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  protected SimpleTimetableAdapter createAdapter()
  {
    mAdapter = new SimpleTimetableAdapter(this);
    if (mInitTts != null)
      mAdapter.setTimetables(mInitTts);
    return mAdapter;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_timetable_simple, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
  }

  @Override
  public String getTimetables()
  {
    return mAdapter.getTimetables();
  }

  @Override
  public void onHoursMinutesPicked(HoursMinutes from, HoursMinutes to, int id)
  {
    mAdapter.onHoursMinutesPicked(from, to, id);
  }

  public void setTimetables(String ttsString)
  {
    if (ttsString == null)
      return;
    mInitTts = OpeningHours.nativeTimetablesFromString(ttsString);
  }
}
