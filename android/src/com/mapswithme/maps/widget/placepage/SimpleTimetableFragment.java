package com.mapswithme.maps.widget.placepage;

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

public class SimpleTimetableFragment extends BaseMwmRecyclerFragment
                                  implements TimetableFragment.TimetableProvider,
                                             HoursMinutesPickerFragment.OnPickListener
{
  private SimpleTimetableAdapter mAdapter;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  protected RecyclerView.Adapter createAdapter()
  {
    mAdapter = new SimpleTimetableAdapter(this);
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
  public Timetable[] getTimetables()
  {
    return mAdapter.getTimetables();
  }

  @Override
  public void onHoursMinutesPicked(HoursMinutes from, HoursMinutes to, int id)
  {
    mAdapter.onHoursMinutesPicked(from, to, id);
  }

  public void setTimetables(Timetable[] tts)
  {
    if (tts == null)
      return;

    createAdapter();
    mAdapter.setTimetables(tts);
  }
}
