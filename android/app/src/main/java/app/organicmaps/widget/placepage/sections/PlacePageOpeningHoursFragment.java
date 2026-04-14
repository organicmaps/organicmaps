package app.organicmaps.widget.placepage.sections;

import android.animation.ValueAnimator;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
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
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.placepage.PlacePageUtils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.text.DateFormat;
import java.text.DateFormatSymbols;
import java.util.Calendar;
import java.util.Date;

public class PlacePageOpeningHoursFragment extends Fragment implements Observer<MapObject>
{
  private View mFrame;
  private View mSchedulePreviewContainer;
  private View mDropdownContent;
  private TextView mTvSchedulePreviewOpenIndicator;
  private TextView mTvSchedulePreviewDescription;
  private TextView mTvSingleLineOpeningHours;
  private RecyclerView mFullWeekOpeningHours;
  private PlaceOpeningHoursAdapter mOpeningHoursAdapter;
  private View dropDownIcon;
  private View mOhContainer;
  private boolean isOhExpanded;

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
    mFullWeekOpeningHours = view.findViewById(R.id.rw__full_opening_hours);
    mSchedulePreviewContainer = view.findViewById(R.id.schedule_preview_container);
    mTvSchedulePreviewOpenIndicator = view.findViewById(R.id.tv__schedule_preview_open_indicator);
    mTvSchedulePreviewDescription = view.findViewById(R.id.tv__schedule_preview_description);
    mTvSingleLineOpeningHours = view.findViewById(R.id.tv__single_line_opening_hours);
    mDropdownContent = view.findViewById(R.id.dropdown_content);
    mOpeningHoursAdapter = new PlaceOpeningHoursAdapter();
    mFullWeekOpeningHours.setAdapter(mOpeningHoursAdapter);
    dropDownIcon = view.findViewById(R.id.dropdown_icon);
    mDropdownContent.getLayoutParams().height = 0;
    UiUtils.hide(dropDownIcon);
    isOhExpanded = false;
    mOhContainer = mFrame.findViewById(R.id.oh_container);
    var touchListener = new RecyclerView.OnItemTouchListener() {
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

  private void refreshOpeningHours(MapObject mapObject)
  {
    final String ohStr = mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
    mOhContainer.setOnLongClickListener((v) -> {
      PlacePageUtils.copyToClipboard(requireContext(), mOhContainer,
                                     TimeFormatUtils.formatTimetables(getResources(), ohStr, timetables));
      return true;
    });

    resetWeeklyViewState();

    final boolean noOhString = ohStr.isEmpty();
    final boolean previewAvailable = refreshSchedulePreview(mapObject);
    final boolean isEmptyTT = timetables == null || timetables.length == 0;

    if (noOhString)
    {
      // no 'opening_hours' tag
      UiUtils.hide(mFrame);
    }
    else if (!previewAvailable)
    {
      // couldn't read anything from 'opening_hours' tag
      UiUtils.show(mFrame);
      UiUtils.show(mSchedulePreviewContainer);
      UiUtils.hide(mTvSchedulePreviewOpenIndicator);
      UiUtils.setTextAndShow(mTvSchedulePreviewDescription, ohStr);
    }
    else if (isEmptyTT)
    {
      // couldn't extract timetables from 'opening_hours' tag
      UiUtils.show(mFrame);
      UiUtils.setTextAndShow(mTvSingleLineOpeningHours, ohStr);
      enableDropdownContent();
    }
    else
    {
      UiUtils.show(mFullWeekOpeningHours);
      UiUtils.show(mFrame);

      final boolean isTwentyFourSeven = timetables[0].isFullWeek() && timetables[0].isFullday;
      if (!isTwentyFourSeven) // Show whole week time table, except if it's open 24/7
      {
        int firstDayOfWeek = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
        mOpeningHoursAdapter.setTimetables(timetables, firstDayOfWeek);
        enableDropdownContent();
      }
    }
  }

  private void expandOpeningHours()
  {
    int targetHeight, startHeight;
    if (!isOhExpanded)
    {
      UiUtils.show(mDropdownContent);
      startHeight = 0;
      targetHeight = getDropdownContentHeight();
      dropDownIcon.animate().rotation(-180f).setDuration(200).start();
      isOhExpanded = true;
    }
    else
    {
      startHeight = getDropdownContentHeight();
      targetHeight = 0;
      dropDownIcon.animate().rotation(0f).setDuration(200).start();
      isOhExpanded = false;
    }
    mDropdownContent.getLayoutParams().height = startHeight;
    final ValueAnimator va = ValueAnimator.ofInt(startHeight, targetHeight);
    va.setDuration(200);
    va.addUpdateListener(animation -> {
      mDropdownContent.getLayoutParams().height = (int) animation.getAnimatedValue();
      mDropdownContent.requestLayout();
      if (mFrame.getParent() instanceof View)
        ((View) mFrame.getParent()).requestLayout();
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
    UiUtils.hide(dropDownIcon);
    UiUtils.hide(mDropdownContent);
    UiUtils.hide(mTvSingleLineOpeningHours);
    UiUtils.hide(mFullWeekOpeningHours);
  }

  private void enableDropdownContent()
  {
    UiUtils.show(mDropdownContent);
    UiUtils.show(dropDownIcon);
    mOhContainer.setOnClickListener((v) -> expandOpeningHours());

    if (isOhExpanded)
    {
      mDropdownContent.getLayoutParams().height = getDropdownContentHeight();
      mDropdownContent.requestLayout();
    }
  }

  private int getDropdownContentHeight()
  {
    // request a layout with dropdown content at its full size to make sure mFrame.getWidth() returns the right result
    mDropdownContent.getLayoutParams().height = ViewGroup.LayoutParams.WRAP_CONTENT;
    mFrame.requestLayout();

    mDropdownContent.measure(View.MeasureSpec.makeMeasureSpec(mFrame.getWidth(), View.MeasureSpec.EXACTLY),
                             View.MeasureSpec.makeMeasureSpec(0, View.MeasureSpec.UNSPECIFIED));
    return mDropdownContent.getMeasuredHeight();
  }

  private boolean refreshSchedulePreview(MapObject mapObject)
  {
    final String ohStr = mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);

    final long currentTime = System.currentTimeMillis() / 1000L;

    final OpeningHoursInfo ohInfo = OpeningHours.nativeGetOpeningHoursInfoFromString(ohStr, currentTime);

    if (ohInfo == null || ohInfo.state == OpeningHoursInfo.RuleState.Unknown)
    {
      UiUtils.hide(mSchedulePreviewContainer);
      return false;
    }

    UiUtils.show(mSchedulePreviewContainer);

    if (ohInfo.isTwentyFourSeven)
    {
      UiUtils.setTextAndShow(mTvSchedulePreviewOpenIndicator, getString(R.string.twentyfour_seven));
      mTvSchedulePreviewOpenIndicator.setTextColor(ContextCompat.getColor(requireContext(), R.color.base_green));
      UiUtils.hide(mTvSchedulePreviewDescription);
    }
    else if (ohInfo.state == OpeningHoursInfo.RuleState.Open)
    {
      String descriptionString;

      final long timeLeftMinutes = (ohInfo.nextTimeClosed - currentTime) / 60;

      Date closeDate = new Date(ohInfo.nextTimeClosed * 1000L);
      DateFormat dateFormat = android.text.format.DateFormat.getTimeFormat(requireContext());

      if (ohInfo.nextTimeClosed == OpeningHoursInfo.TIME_NEVER) // Will stay open forever
        descriptionString = "";
      else if (timeLeftMinutes < 3 * 60) // Less than 3 hours
        descriptionString = " • " + getString(R.string.closes_in, getTimeIntervalString(timeLeftMinutes)) + " • "
                          + dateFormat.format(closeDate);
      else if (timeLeftMinutes < 24 * 60) // Less than 24 hours
        descriptionString = " • " + getString(R.string.closes_at, dateFormat.format(closeDate));
      else
        descriptionString = "";

      UiUtils.setTextAndShow(mTvSchedulePreviewOpenIndicator, getString(R.string.editor_time_open));
      mTvSchedulePreviewOpenIndicator.setTextColor(ContextCompat.getColor(requireContext(), R.color.base_green));

      UiUtils.setTextAndHideIfEmpty(mTvSchedulePreviewDescription, descriptionString);
    }
    else // ohInfo.state == OpeningHoursInfo.RuleState.Closed
    {
      String descriptionString;

      final long timeLeftMinutes = (ohInfo.nextTimeOpen - currentTime) / 60;

      final Calendar nowCal = Calendar.getInstance();

      Calendar openCal = Calendar.getInstance();
      openCal.setTimeInMillis(ohInfo.nextTimeOpen * 1000L);

      Date openDate = new Date(ohInfo.nextTimeOpen * 1000L);
      DateFormat dateFormat = android.text.format.DateFormat.getTimeFormat(requireContext());

      boolean willOpenToday = nowCal.get(Calendar.DAY_OF_YEAR) == openCal.get(Calendar.DAY_OF_YEAR)
                           && nowCal.get(Calendar.YEAR) == openCal.get(Calendar.YEAR);

      if (ohInfo.nextTimeOpen == OpeningHoursInfo.TIME_NEVER) // Will stay closed forever
        descriptionString = "";
      else if (timeLeftMinutes < 3 * 60) // Less than 3 hours
        descriptionString = " • " + getString(R.string.opens_in, getTimeIntervalString(timeLeftMinutes)) + " • "
                          + dateFormat.format(openDate);
      else if (willOpenToday) // Today
        descriptionString = " • " + getString(R.string.opens_at, dateFormat.format(openDate));
      else if (timeLeftMinutes < 24 * 60) // Less than 24 hours
        descriptionString = " • " + getString(R.string.opens_tomorrow_at, dateFormat.format(openDate));
      else if (timeLeftMinutes < 7 * 24 * 60) // Less than 1 week
      {
        final int openDay = openCal.get(Calendar.DAY_OF_WEEK);
        final String openDayName = DateFormatSymbols.getInstance().getWeekdays()[openDay];
        descriptionString = " • " + getString(R.string.opens_dayoftheweek_at, openDayName, dateFormat.format(openDate));
      }
      else
        descriptionString = "";

      UiUtils.setTextAndShow(mTvSchedulePreviewOpenIndicator, getString(R.string.closed_now));
      mTvSchedulePreviewOpenIndicator.setTextColor(ContextCompat.getColor(requireContext(), R.color.base_red));

      UiUtils.setTextAndHideIfEmpty(mTvSchedulePreviewDescription, descriptionString);
    }

    return true;
  }

  private String getTimeIntervalString(long minutes)
  {
    if (minutes >= 60)
    {
      if (minutes % 60 != 0)
      {
        return String.format("%d %s %d %s", minutes / 60, getString(R.string.hour), minutes % 60,
                             getString(R.string.minute));
      }
      else
      {
        return String.format("%d %s", minutes / 60, getString(R.string.hour));
      }
    }
    else
    {
      return String.format("%d %s", minutes % 60, getString(R.string.minute));
    }
  }
}
