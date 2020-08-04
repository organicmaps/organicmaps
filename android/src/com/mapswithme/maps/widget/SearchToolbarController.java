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
import com.google.android.material.datepicker.MaterialDatePicker;
import com.google.android.material.datepicker.MaterialPickerOnPositiveButtonClickListener;
import com.mapswithme.maps.R;
import com.mapswithme.maps.search.BookingFilterParams;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.statistics.AlohaHelper;

import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Locale;

public class SearchToolbarController extends ToolbarController
                                  implements View.OnClickListener
{
  private static final int REQUEST_VOICE_RECOGNITION = 0xCA11;
  public static final String DAY_OF_MONTH_PATTERN = "MMM d";

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
  private Chip mRooms;
  private final boolean mVoiceInputSupported = InputUtils.isVoiceInputSupported(getActivity());
  @NonNull
  private final TextWatcher mTextWatcher = new StringUtils.SimpleTextWatcher()
  {
    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
      updateButtons(TextUtils.isEmpty(s));
      SearchToolbarController.this.onTextChanged(s.toString());
    }
  };
  @Nullable
  private Pair<Long, Long> mChosenDates;
  @NonNull
  private final View.OnClickListener mChooseDatesClickListener = v -> {
    MaterialDatePicker.Builder<Pair<Long, Long>> builder
        = MaterialDatePicker.Builder.dateRangePicker();
    if (mChosenDates != null)
      builder.setSelection(mChosenDates);
    final MaterialDatePicker<?> picker = builder.build();
    picker.addOnPositiveButtonClickListener(new MaterialPickerOnPositiveButtonClickListener<Object>()
    {
      @Override
      public void onPositiveButtonClick(Object selection)
      {
        //noinspection unchecked
        mChosenDates = (Pair<Long, Long>) selection;
        mChooseDatesChip.setText(picker.getHeaderText());
        for (FilterParamsChangedListener listener : mFilterParamsChangedListeners)
          listener.onBookingParamsChanged();
      }
    });
    picker.show(((AppCompatActivity) getActivity()).getSupportFragmentManager(), picker.toString());
  };
  @NonNull
  private List<FilterParamsChangedListener> mFilterParamsChangedListeners = new ArrayList<>();

  public interface Container
  {
    SearchToolbarController getController();
  }

  public SearchToolbarController(View root, Activity activity)
  {
    super(root, activity);
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
      mChooseDatesChip = mFilterContainer.findViewById(R.id.choose_dates);
      mRooms = mFilterContainer.findViewById(R.id.rooms);
      //noinspection ConstantConditions
      mChooseDatesChip.setOnClickListener(mChooseDatesClickListener);
      mChooseDatesChip.setOnCloseIconClickListener(mChooseDatesClickListener);
    }

    showProgress(false);
    updateButtons(true);
  }

  public void setFilterParams(@Nullable BookingFilterParams params)
  {
    if (mChooseDatesChip == null)
      return;

    if (params == null)
    {
      mChooseDatesChip.setText(R.string.date_picker_сhoose_dates_cta);
      mChosenDates = null;
      return;
    }

    mChosenDates = new Pair<>(params.getCheckinMillisec(), params.getCheckoutMillisec());
    SimpleDateFormat dateFormater = new SimpleDateFormat(DAY_OF_MONTH_PATTERN,
                                                         Locale.getDefault());
    @SuppressWarnings("ConstantConditions")
    String start = dateFormater.format(new Date(mChosenDates.first));
    @SuppressWarnings("ConstantConditions")
    String end = dateFormater.format(new Date(mChosenDates.second));
    mChooseDatesChip.setText(getActivity().getString(R.string.booking_filter_date_range,
                                                     start, end));
  }

  private void updateButtons(boolean queryEmpty)
  {
    UiUtils.showIf(supportsVoiceSearch() && queryEmpty && mVoiceInputSupported, mVoiceInput);
    UiUtils.showIf(alwaysShowClearButton() || !queryEmpty, mClear);
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
      startVoiceRecognition(InputUtils.createIntentForVoiceRecognition(getActivity().getString(getVoiceInputPrompt())), REQUEST_VOICE_RECOGNITION);
    } catch (ActivityNotFoundException e)
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
      break;

    case R.id.voice_input:
      onVoiceInputClick();
      break;
    }
  }

  public void showSearchControls(boolean show)
  {
    UiUtils.showIf(show, mSearchContainer);
  }

  public void showFilterControls(boolean show)
  {
    if (mFilterContainer == null)
      return;

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
    if (mChosenDates == null)
      return null;

    return BookingFilterParams.createParams(mChosenDates.first, mChosenDates.second);
  }

  public interface FilterParamsChangedListener
  {
    void onBookingParamsChanged();
  }
}
