package app.organicmaps.widget;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewTreeObserver;
import android.view.inputmethod.EditorInfo;
import androidx.activity.OnBackPressedCallback;
import androidx.activity.result.ActivityResult;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import app.organicmaps.R;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;

public class SearchToolbarController extends ToolbarController
{
  @Nullable
  private final View mToolbarContainer;
  @NonNull
  private final View mSearchContainer;
  @NonNull
  private final View mBack;
  @NonNull
  private final TextInputEditText mQuery;
  @Nullable
  private final TextInputLayout mQueryLayout;
  private boolean mEndIconQueryEmpty;
  private boolean mFromCategory = false;
  // Pending listener that shows the keyboard once the window gains focus (see activate()).
  @Nullable
  private ViewTreeObserver.OnWindowFocusChangeListener mShowKeyboardOnFocus;
  @Nullable
  private final View mProgress;
  private final boolean mVoiceInputSupported = InputUtils.isVoiceInputSupported(requireActivity());
  private final TextWatcher mTextWatcher = new StringUtils.SimpleTextWatcher() {
    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
      onQueryChanged(s, true);
    }
  };

  private final OnBackPressedCallback mBackPressedCallback = new OnBackPressedCallback(false) {
    @Override
    public void handleOnBackPressed()
    {
      clear();
      setEnabled(false);
    }
  };

  public SearchToolbarController(@NonNull View root, @NonNull Activity activity)
  {
    super(root, activity);
    mToolbarContainer = getToolbar().findViewById(R.id.toolbar_container);
    mSearchContainer = getToolbar().findViewById(R.id.search_container);
    mBack = mSearchContainer.findViewById(R.id.back);
    mQuery = mSearchContainer.findViewById(R.id.query);
    mQueryLayout = mSearchContainer.findViewById(R.id.query_input_layout);
    mQuery.addTextChangedListener(mTextWatcher);
    mQuery.setOnEditorActionListener((v, actionId, event) -> {
      boolean isSearchDown =
          (event != null && event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);

      boolean isSearchAction = (actionId == EditorInfo.IME_ACTION_SEARCH);

      return (isSearchDown || isSearchAction) && onStartSearchClick();
    });
    mProgress = mSearchContainer.findViewById(R.id.progress);
    showProgress(false);
    updateViewsVisibility(true);
  }

  private void updateViewsVisibility(boolean queryEmpty)
  {
    UiUtils.showIf(showBackButton(), mBack);
    updateEndIcon(queryEmpty);
  }

  // Single trailing slot: clear (X) while there is text, otherwise the voice-search mic when available.
  private void updateEndIcon(boolean queryEmpty)
  {
    if (mQueryLayout == null || mEndIconQueryEmpty == queryEmpty)
      return;
    mEndIconQueryEmpty = queryEmpty;
    if (!queryEmpty)
    {
      mQueryLayout.setEndIconDrawable(R.drawable.ic_close_rounded);
      mQueryLayout.setEndIconContentDescription(R.string.clear_the_search);
      mQueryLayout.setEndIconOnClickListener(v -> clear());
      mQueryLayout.setEndIconVisible(true);
    }
    else if (supportsVoiceSearch() && mVoiceInputSupported)
    {
      mQueryLayout.setEndIconDrawable(R.drawable.ic_mic_white);
      mQueryLayout.setEndIconContentDescription(R.string.voice_search);
      mQueryLayout.setEndIconOnClickListener(v -> onVoiceInputClick());
      mQueryLayout.setEndIconVisible(true);
    }
    else
      mQueryLayout.setEndIconVisible(false);
  }

  private void onQueryChanged(@Nullable CharSequence s, boolean resetCategoryFlag)
  {
    if (resetCategoryFlag)
      mFromCategory = false;
    syncForQueryChange(s);
    onTextChanged(s == null ? "" : s.toString());
  }

  private void syncForQueryChange(@Nullable CharSequence query)
  {
    final boolean isEmpty = TextUtils.isEmpty(query);
    mBackPressedCallback.setEnabled(!isEmpty);
    updateViewsVisibility(isEmpty);
  }

  protected boolean showBackButton()
  {
    return true;
  }

  protected void onTextChanged(String query) {}

  protected boolean onStartSearchClick()
  {
    return true;
  }

  protected void startVoiceRecognition(Intent intent)
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

  private void onVoiceInputClick()
  {
    try
    {
      startVoiceRecognition(
          InputUtils.createIntentForVoiceRecognition(requireActivity().getString(getVoiceInputPrompt())));
    }
    catch (ActivityNotFoundException e)
    {}
  }

  protected @StringRes int getVoiceInputPrompt()
  {
    return R.string.search;
  }

  public String getQuery()
  {
    return (UiUtils.isVisible(mSearchContainer) ? mQuery.getText().toString() : "");
  }
  public boolean isCategory()
  {
    return mFromCategory;
  }

  public void setQuery(CharSequence query, boolean fromCategory)
  {
    mFromCategory = fromCategory;
    mQuery.removeTextChangedListener(mTextWatcher);
    mQuery.setText(query);
    if (!TextUtils.isEmpty(query))
      mQuery.setSelection(query.length());
    mQuery.addTextChangedListener(mTextWatcher);
    onQueryChanged(query, false);
  }
  public void setQuery(CharSequence query)
  {
    setQuery(query, false);
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
    removeShowKeyboardOnFocusListener();
    mShowKeyboardOnFocus = hasFocus ->
    {
      if (hasFocus)
      {
        removeShowKeyboardOnFocusListener();
        mQuery.requestFocus();
        InputUtils.showKeyboard(mQuery);
      }
    };
    mQuery.getViewTreeObserver().addOnWindowFocusChangeListener(mShowKeyboardOnFocus);
    if (mQuery.hasWindowFocus())
    {
      removeShowKeyboardOnFocusListener();
      InputUtils.showKeyboard(mQuery);
    }
  }

  public void deactivate()
  {
    removeShowKeyboardOnFocusListener();
    InputUtils.hideKeyboard(mQuery);
    InputUtils.removeFocusEditTextHack(mQuery);
  }

  private void removeShowKeyboardOnFocusListener()
  {
    if (mShowKeyboardOnFocus == null)
      return;
    mQuery.getViewTreeObserver().removeOnWindowFocusChangeListener(mShowKeyboardOnFocus);
    mShowKeyboardOnFocus = null;
  }

  public void showProgress(boolean show)
  {
    if (mProgress == null)
      return;
    if (UiUtils.isVisible(mProgress) == show)
      return;
    UiUtils.showIf(show, mProgress);
  }

  public void showSearchControls(boolean show)
  {
    if (mToolbarContainer != null)
      UiUtils.showIf(show, mToolbarContainer);
    UiUtils.showIf(show, mSearchContainer);
  }

  public void onVoiceRecognitionResult(ActivityResult activityResult)
  {
    if (activityResult.getResultCode() == Activity.RESULT_OK)
    {
      if (activityResult.getData() == null)
      {
        return;
      }
      String recognitionResult = InputUtils.getBestRecognitionResult(activityResult.getData());
      if (!TextUtils.isEmpty(recognitionResult))
        setQuery(recognitionResult);
    }
  }

  public void setHint(@StringRes int hint)
  {
    mQuery.setHint(hint);
  }

  @NonNull
  public OnBackPressedCallback getBackPressedCallback()
  {
    return mBackPressedCallback;
  }

  public void setQuerySilently(CharSequence query, boolean fromCategory)
  {
    mFromCategory = fromCategory;
    mQuery.removeTextChangedListener(mTextWatcher);
    mQuery.setText(query);
    if (!TextUtils.isEmpty(query))
      mQuery.setSelection(query.length());
    mQuery.addTextChangedListener(mTextWatcher);
    syncForQueryChange(query);
  }
}
