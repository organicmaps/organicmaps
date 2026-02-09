package app.organicmaps.widget.placepage.sections;

import android.animation.ValueAnimator;
import android.content.res.Resources;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
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
import app.organicmaps.sdk.editor.data.Timespan;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.util.Calendar;
import java.util.Locale;

public class PlacePageOpeningHoursFragment extends Fragment implements Observer<MapObject>
{
  private View mFrame;
  private TextView mTodayLabel;
  private TextView mTodayOpenTime;
  private TextView mTodayNonBusinessTime;
  private RecyclerView mFullWeekOpeningHours;
  private PlaceOpeningHoursAdapter mOpeningHoursAdapter;
  private View dropDownIcon;
  private View mOhContainer;
  private boolean isOhExpanded;

  private PlacePageViewModel mViewModel;
  private RecyclerView.OnItemTouchListener mRecyclerTouchListener;

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
    dropDownIcon = view.findViewById(R.id.dropdown_icon);
    mFullWeekOpeningHours.getLayoutParams().height = 0;
    UiUtils.hide(dropDownIcon);
    isOhExpanded = false;
    mOhContainer = mFrame.findViewById(R.id.oh_container);
    RecyclerView.OnItemTouchListener touchListener = new RecyclerView.OnItemTouchListener()
    {
        @Override
        public boolean onInterceptTouchEvent(@NonNull RecyclerView rv, @NonNull MotionEvent e)
        {
            if (e.getAction() == MotionEvent.ACTION_UP)
                expandOpeningHours();
            return false;
        }
        @Override
        public void onTouchEvent(@NonNull RecyclerView rv, @NonNull MotionEvent e)
          {}
        @Override
        public void onRequestDisallowInterceptTouchEvent(boolean disallowIntercept)
          {}
    };
    mFullWeekOpeningHours.addOnItemTouchListener(touchListener);
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
    mOhContainer.setOnLongClickListener((v) -> {
      PlacePageUtils.copyToClipboard(requireContext(), mOhContainer,
                                     TimeFormatUtils.formatTimetables(getResources(), ohStr, timetables));
      return true;
    });
    final boolean isEmptyTT = (timetables == null || timetables.length == 0);
    final int color = ThemeUtils.getColor(requireContext(), android.R.attr.textColorPrimary);

    if (isEmptyTT)
    {
      resetWeeklyViewState();
      // 'opening_hours' tag wasn't parsed either because it's empty or wrong format.
      if (!ohStr.isEmpty())
      {
        UiUtils.show(mFrame);
        refreshTodayOpeningHours(ohStr, color);
        UiUtils.hide(mTodayNonBusinessTime);
      }
      else
        UiUtils.hide(mFrame);
    }
    else
    {
      UiUtils.show(mFrame);
      final Resources resources = getResources();
      if (timetables[0].isFullWeek())
      {
        resetWeeklyViewState();
        final Timetable tt = timetables[0];
        if (tt.isFullday)
        {
          refreshTodayOpeningHours(resources.getString(R.string.twentyfour_seven), color);
          UiUtils.clearTextAndHide(mTodayNonBusinessTime);
        }
        else
        {
          refreshTodayOpeningHours(resources.getString(R.string.daily), tt.workingTimespan.toWideString(), color);
          refreshTodayNonBusinessTime(tt.closedTimespans);
        }
      }
      else
      {
        // Show whole week time table.
        int firstDayOfWeek = Calendar.getInstance(Locale.getDefault()).getFirstDayOfWeek();
        mOpeningHoursAdapter.setTimetables(timetables, firstDayOfWeek);
        if (isOhExpanded)
        {
            mFullWeekOpeningHours.post(() -> {
                mFullWeekOpeningHours.measure(
                        View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
                        View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
                int newHeight = mFullWeekOpeningHours.getMeasuredHeight();
                mFullWeekOpeningHours.getLayoutParams().height = newHeight;
                mFullWeekOpeningHours.requestLayout();
              });
          }
        UiUtils.show(dropDownIcon);
        mOhContainer.setOnClickListener((v) -> expandOpeningHours());

        // Show today's open time + non-business time.
        boolean containsCurrentWeekday = false;
        final int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
        for (Timetable tt : timetables)
        {
          if (tt.containsWeekday(currentDay))
          {
            containsCurrentWeekday = true;
            String openTime;

            if (tt.isFullday)
            {
              String allDay = resources.getString(R.string.editor_time_allday);
              openTime = Utils.unCapitalize(allDay);
            }
            else
              openTime = tt.workingTimespan.toWideString();

            refreshTodayOpeningHours(resources.getString(R.string.today), openTime, color);
            refreshTodayNonBusinessTime(tt.closedTimespans);

            break;
          }
        }

        // Show that place is closed today.
        if (!containsCurrentWeekday)
        {
          refreshTodayOpeningHours(resources.getString(R.string.day_off_today),
                                   ContextCompat.getColor(requireContext(), R.color.base_red));
          UiUtils.hide(mTodayNonBusinessTime);
        }
      }
    }
  }

  private void expandOpeningHours()
  {
    int targetHeight, startHeight;
    if (!isOhExpanded)
    {
      UiUtils.show(mFullWeekOpeningHours);
      startHeight = 0;
      mFullWeekOpeningHours.measure(View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED),
                                    View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
      targetHeight = mFullWeekOpeningHours.getMeasuredHeight();
      dropDownIcon.animate().rotation(-180f).setDuration(200).start();
      isOhExpanded = true;
    }
    else
    {
      startHeight = mFullWeekOpeningHours.getMeasuredHeight();
      targetHeight = 0;
      dropDownIcon.animate().rotation(0f).setDuration(200).start();
      isOhExpanded = false;
    }
    mFullWeekOpeningHours.getLayoutParams().height = startHeight;
    final ValueAnimator va = ValueAnimator.ofInt(startHeight, targetHeight);
    va.setDuration(200);
    va.addUpdateListener(animation -> {
      mFullWeekOpeningHours.getLayoutParams().height = (int) animation.getAnimatedValue();
      mFullWeekOpeningHours.requestLayout();
      if (mFrame.getParent() instanceof View)
      {
          ((View) mFrame.getParent()).requestLayout();
      }
    });
    va.start();
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
  private void resetWeeklyViewState()
  {
        isOhExpanded = false;
        mFullWeekOpeningHours.getLayoutParams().height = 0;
        UiUtils.hide(mFullWeekOpeningHours);
        UiUtils.hide(dropDownIcon);
        dropDownIcon.setRotation(0f);
        mOhContainer.setOnClickListener(null);
  }
}
