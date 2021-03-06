package com.mapswithme.maps.widget;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.util.Pair;
import com.google.android.material.chip.Chip;
import com.google.android.material.datepicker.CalendarConstraints;
import com.google.android.material.datepicker.MaterialDatePicker;
import com.google.android.material.datepicker.MaterialPickerOnPositiveButtonClickListener;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.BookingFilterParams;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.maps.widget.menu.MenuController;
import com.mapswithme.maps.widget.menu.MenuControllerFactory;
import com.mapswithme.maps.widget.menu.MenuRoomsGuestsListener;
import com.mapswithme.maps.widget.menu.MenuStateObserver;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class SearchToolbarController extends ToolbarController
                                  implements View.OnClickListener, MenuRoomsGuestsListener,
                                             FilterUtils.RoomsGuestsCountProvider
{
  private static final int REQUEST_VOICE_RECOGNITION = 0xCA11;
  @Nullable
  private final View mToolbarContainer;
  @NonNull
  private final View mSearchContainer;
  @NonNull
  private final EditText mQuery;
  @NonNull
  private final View mProgress;
  @NonNull
  private final View mClear;
  @NonNull
  private final View mVoiceInput;
  @Nullable
  private final View mFilterContainer;
  @Nullable
  private Chip mChooseDatesChip;
  @Nullable
  private Chip mRoomsChip;
  private final boolean mVoiceInputSupported = InputUtils.isVoiceInputSupported(getActivity());
  @NonNull
  private final TextWatcher mTextWatcher = new StringUtils.SimpleTextWatcher()
  {
    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
      updateViewsVisibility(TextUtils.isEmpty(s));
      SearchToolbarController.this.onTextChanged(s.toString());
    }
  };
  @Nullable
  private Pair<Long, Long> mChosenDates;
  @Nullable
  private FilterUtils.RoomGuestCounts mRoomGuestCounts;
  @NonNull
  private final View.OnClickListener mChooseDatesClickListener = v -> {
    if (!ConnectionState.INSTANCE.isConnected())
    {
      Statistics.INSTANCE.trackQuickFilterClickError(Statistics.EventParam.HOTEL,
                                                     Statistics.ParamValue.DATE,
                                                     Statistics.ParamValue.NO_INTERNET);
      FilterUtils.showNoNetworkConnectionDialog((AppCompatActivity) requireActivity());
      return;
    }

    MaterialDatePicker.Builder<Pair<Long, Long>> builder
        = MaterialDatePicker.Builder.dateRangePicker();
    CalendarConstraints.Builder constraintsBuilder = FilterUtils.createDateConstraintsBuilder();
    builder.setCalendarConstraints(constraintsBuilder.build());
    if (mChosenDates != null)
      builder.setSelection(mChosenDates);
    final MaterialDatePicker<Pair<Long, Long>> picker = builder.build();
    picker.addOnPositiveButtonClickListener(new DatePickerPositiveClickListener(picker));
    picker.show(((AppCompatActivity) requireActivity()).getSupportFragmentManager(), picker.toString());
  };
  @NonNull
  private final List<FilterParamsChangedListener> mFilterParamsChangedListeners = new ArrayList<>();
  @Nullable
  private MenuController mGuestsRoomsMenuController;
  @NonNull
  private final View.OnClickListener mRoomsClickListener = v -> {
    if (!ConnectionState.INSTANCE.isConnected())
    {
      Statistics.INSTANCE.trackQuickFilterClickError(Statistics.EventParam.HOTEL,
                                                     Statistics.ParamValue.ROOMS,
                                                     Statistics.ParamValue.NO_INTERNET);
      FilterUtils.showNoNetworkConnectionDialog((AppCompatActivity) requireActivity());
      return;
    }

    InputUtils.hideKeyboard(v);

    if (!mGuestsRoomsMenuController.isClosed())
      return;

    mGuestsRoomsMenuController.open();
  };

  @Override
  public void onRoomsGuestsApplied(@NonNull FilterUtils.RoomGuestCounts counts)
  {
    if (mRoomsChip == null)
      return;

    if (counts.equals(mRoomGuestCounts))
      return;

    formatAndSetRoomGuestsCounts(counts);
    if (mChosenDates == null)
    {
      long checkinMillis = MaterialDatePicker.todayInUtcMilliseconds();
      long checkoutMillis = FilterUtils.getDayAfter(checkinMillis);
      formatAndSetChosenDates(checkinMillis, checkoutMillis);
    }
    for (FilterParamsChangedListener listener : mFilterParamsChangedListeners)
      listener.onBookingParamsChanged();
  }

  @Nullable
  @Override
  public FilterUtils.RoomGuestCounts getRoomGuestCount()
  {
    return mRoomGuestCounts;
  }

  public interface Container
  {
    SearchToolbarController getController();
  }

  public SearchToolbarController(@NonNull View root, @NonNull Activity activity)
  {
    this(root, activity, null);
  }

  public SearchToolbarController(@NonNull View root,
                                 @NonNull Activity activity,
                                 @Nullable RoomsGuestsMenuStateCallback callback)
  {
    super(root, activity);
    mToolbarContainer = getToolbar().findViewById(R.id.toolbar_container);
    mSearchContainer = getToolbar().findViewById(R.id.search_container);
    mQuery = mSearchContainer.findViewById(R.id.query);
    mQuery.setOnClickListener(this);
    mQuery.addTextChangedListener(mTextWatcher);
    mQuery.setOnEditorActionListener(
        (v, actionId, event) ->
        {
          boolean isSearchDown = (event != null &&
                                  event.getAction() == KeyEvent.ACTION_DOWN &&
                                  event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);

          boolean isSearchAction = (actionId == EditorInfo.IME_ACTION_SEARCH);

          return (isSearchDown || isSearchAction) && onStartSearchClick();
        });
    mProgress = mSearchContainer.findViewById(R.id.progress);
    mVoiceInput = mSearchContainer.findViewById(R.id.voice_input);
    mVoiceInput.setOnClickListener(this);
    mClear = mSearchContainer.findViewById(R.id.clear);
    mClear.setOnClickListener(this);
    mFilterContainer = getToolbar().findViewById(R.id.filter_container);
    if (mFilterContainer != null)
    {
      mChooseDatesChip = mFilterContainer.findViewById(R.id.chip_choose_dates);
      mRoomsChip = mFilterContainer.findViewById(R.id.chip_rooms);
      //noinspection ConstantConditions
      mChooseDatesChip.setOnClickListener(mChooseDatesClickListener);
      mChooseDatesChip.setOnCloseIconClickListener(mChooseDatesClickListener);
      //noinspection ConstantConditions
      mRoomsChip.setOnClickListener(mRoomsClickListener);
      mRoomsChip.setOnCloseIconClickListener(mRoomsClickListener);
    }

    View coordinatorLayout = requireActivity().findViewById(R.id.coordinator);
    if (coordinatorLayout != null
        && coordinatorLayout.findViewById(R.id.guests_and_rooms_menu_sheet) != null)
    {
      MenuStateObserver stateObserver = new RoomsGuestsMenuStateObserver(requireActivity(), callback);
      mGuestsRoomsMenuController
          = MenuControllerFactory.createGuestsRoomsMenuController(this, stateObserver, this);
      mGuestsRoomsMenuController.initialize(requireActivity().findViewById(R.id.coordinator));
    }

    showProgress(false);
    updateViewsVisibility(true);
  }

  public void setFilterParams(@NonNull BookingFilterParams params)
  {
    formatAndSetChosenDates(params.getCheckinMillisec(), params.getCheckoutMillisec());
    formatAndSetRoomGuestsCounts(FilterUtils.toCounts(params.getRooms()));
  }

  private void formatAndSetChosenDates(long checkinMillis, long checkoutMillis)
  {
    if (mChooseDatesChip == null)
      return;

    mChooseDatesChip.setText(FilterUtils.makeDateRangeHeader(mChooseDatesChip.getContext(),
                                                             checkinMillis, checkoutMillis));
    mChosenDates = new Pair<>(checkinMillis, checkoutMillis);
  }

  private void formatAndSetRoomGuestsCounts(@NonNull FilterUtils.RoomGuestCounts counts)
  {
    if (mRoomsChip == null)
      return;

    int people = counts.getAdults() + counts.getChildren() + counts.getInfants();
    mRoomsChip.setText(String.valueOf(people));
    mRoomGuestCounts = counts;
  }

  public void resetFilterParams()
  {
    if (mChooseDatesChip != null)
      mChooseDatesChip.setText(R.string.date_picker_сhoose_dates_cta);
    mChosenDates = null;

    if (mRoomsChip != null)
      mRoomsChip.setText(R.string.guests_picker_rooms);
    mRoomGuestCounts = null;
  }

  private void updateViewsVisibility(boolean queryEmpty)
  {
    UiUtils.showIf(supportsVoiceSearch() && queryEmpty && mVoiceInputSupported, mVoiceInput);
    UiUtils.showIf(alwaysShowClearButton() || !queryEmpty, mClear);
    if (mFilterContainer != null && UiUtils.isVisible(mFilterContainer) && queryEmpty)
      UiUtils.hide(mFilterContainer);
  }

  protected void onQueryClick(String query) {}

  protected void onTextChanged(String query) {}

  protected boolean onStartSearchClick()
  {
    return true;
  }

  protected void onClearClick()
  {
    clear();
  }

  protected void startVoiceRecognition(Intent intent, int code)
  {
    throw new RuntimeException("To be used startVoiceRecognition() must be implemented by descendant class");
  }

  /**
   * Return true to display & activate voice search. Turned OFF by default.
   */
  protected boolean supportsVoiceSearch()
  {
    return false;
  }

  protected boolean alwaysShowClearButton()
  {
    return false;
  }

  private void onVoiceInputClick()
  {
    try
    {
      startVoiceRecognition(InputUtils.createIntentForVoiceRecognition(
          requireActivity().getString(getVoiceInputPrompt())), REQUEST_VOICE_RECOGNITION);
    }
    catch (ActivityNotFoundException e)
    {
      AlohaHelper.logException(e);
    }
  }

  protected @StringRes int getVoiceInputPrompt()
  {
    return R.string.search;
  }

  public String getQuery()
  {
    return (UiUtils.isVisible(mSearchContainer) ? mQuery.getText().toString() : "");
  }

  public void setQuery(CharSequence query)
  {
    mQuery.setText(query);
    if (!TextUtils.isEmpty(query))
      mQuery.setSelection(query.length());
  }

  public void clear()
  {
    setQuery("");
  }

  public boolean hasQuery()
  {
    return !getQuery().isEmpty();
  }

  public void activate()
  {
    mQuery.requestFocus();
    InputUtils.showKeyboard(mQuery);
  }

  public void deactivate()
  {
    InputUtils.hideKeyboard(mQuery);
    InputUtils.removeFocusEditTextHack(mQuery);
  }

  public void showProgress(boolean show)
  {
    UiUtils.showIf(show, mProgress);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.query:
      onQueryClick(getQuery());
      break;

    case R.id.clear:
      onClearClick();
      closeBottomMenu();
      break;

    case R.id.voice_input:
      onVoiceInputClick();
      break;
    }
  }

  public void showSearchControls(boolean show)
  {
    if (mToolbarContainer != null)
      UiUtils.showIf(show, mToolbarContainer);
    UiUtils.showIf(show, mSearchContainer);
  }

  public void showFilterControls(boolean show)
  {
    if (mFilterContainer == null)
      return;

    if (getActivity() != null && UiUtils.isHidden(mFilterContainer) && show)
      Statistics.INSTANCE.trackQuickFilterOpen(Statistics.EventParam.HOTEL);

    UiUtils.showIf(show, mFilterContainer);
  }

  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    if (requestCode == REQUEST_VOICE_RECOGNITION && resultCode == Activity.RESULT_OK)
    {
      String result = InputUtils.getBestRecognitionResult(data);
      if (!TextUtils.isEmpty(result))
        setQuery(result);
    }
  }

  public void setHint(@StringRes int hint)
  {
    mQuery.setHint(hint);
  }

  public void addBookingParamsChangedListener(@NonNull FilterParamsChangedListener listener)
  {
    mFilterParamsChangedListeners.add(listener);
  }

  public void removeBookingParamsChangedListener(@NonNull FilterParamsChangedListener listener)
  {
    mFilterParamsChangedListeners.remove(listener);
  }

  @Nullable
  public BookingFilterParams getFilterParams()
  {
    if (mChosenDates == null || mChosenDates.first == null || mChosenDates.second == null)
      return null;

    return BookingFilterParams.createParams(mChosenDates.first, mChosenDates.second,
                                            mRoomGuestCounts);
  }

  public boolean closeBottomMenu()
  {
    if (mGuestsRoomsMenuController == null)
      return false;

    if (!mGuestsRoomsMenuController.isClosed())
    {
      mGuestsRoomsMenuController.close();
      return true;
    }
    return false;
  }

  public interface FilterParamsChangedListener
  {
    void onBookingParamsChanged();
  }

  private class DatePickerPositiveClickListener
      implements MaterialPickerOnPositiveButtonClickListener<Pair<Long, Long>>
  {
    @NonNull
    private final MaterialDatePicker<Pair<Long, Long>> mPicker;

    private DatePickerPositiveClickListener(@NonNull MaterialDatePicker<Pair<Long, Long>> picker)
    {
      mPicker = picker;
    }

    @Override
    public void onPositiveButtonClick(Pair<Long, Long> selection)
    {
      if (selection == null)
        return;

      mChosenDates = selection;
      if (mChosenDates.first == null || mChosenDates.second == null)
        return;

      validateAndSetupDates(mChosenDates.first, mChosenDates.second);
      if (mRoomGuestCounts == null)
        formatAndSetRoomGuestsCounts(FilterUtils.toCounts(BookingFilterParams.Room.DEFAULT));

      for (FilterParamsChangedListener listener : mFilterParamsChangedListeners)
        listener.onBookingParamsChanged();
    }

    private void validateAndSetupDates(long checkinMillis, long checkoutMillis)
    {
      if (checkoutMillis <= checkinMillis)
      {
        formatAndSetChosenDates(checkinMillis, FilterUtils.getDayAfter(checkinMillis));
      }
      else if (!FilterUtils.isWithinMaxStayingDays(checkinMillis, checkoutMillis))
      {
        Utils.showSnackbar(requireActivity(), mToolbarContainer.getRootView(),
                           R.string.thirty_days_limit_dialog);
        formatAndSetChosenDates(checkinMillis, FilterUtils.getMaxCheckoutInMillis(checkinMillis));
      }
      else
      {
        Objects.requireNonNull(mChooseDatesChip);
        mChooseDatesChip.setText(mPicker.getHeaderText());
      }
    }
  }

  private static class RoomsGuestsMenuStateObserver implements MenuStateObserver
  {
    @NonNull
    private final Activity mActivity;
    @NonNull
    private final RoomsGuestsMenuStateCallback mCallback;

    private RoomsGuestsMenuStateObserver(@NonNull Activity activity,
                                         @Nullable RoomsGuestsMenuStateCallback callback)
    {
      mActivity = activity;
      mCallback = callback;
    }

    @Override
    public void onMenuOpen()
    {
      FadeView fadeView = mActivity.findViewById(R.id.fade_view);
      if (fadeView == null)
        return;

      fadeView.fadeIn();
      if (mCallback != null)
        mCallback.onRoomsGuestsMenuStateChange(true);
    }

    @Override
    public void onMenuClosed()
    {
      FadeView fadeView = mActivity.findViewById(R.id.fade_view);
      if (fadeView == null)
        return;

      if (mCallback != null)
        mCallback.onRoomsGuestsMenuStateChange(false);
      fadeView.fadeOut();
    }
  }

  public static interface RoomsGuestsMenuStateCallback
  {
    public void onRoomsGuestsMenuStateChange(boolean isOpen);
  }
}
