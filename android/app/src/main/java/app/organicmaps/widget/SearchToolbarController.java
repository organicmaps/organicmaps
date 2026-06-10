package app.organicmaps.widget;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewParent;
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

public class SearchToolbarController extends ToolbarController implements View.OnClickListener
{
  @Nullable
  private final View mToolbarContainer;
  @NonNull
  private final View mSearchContainer;
  @NonNull
  private final View mBack;
  @NonNull
  private final TextInputEditText mQuery;
  private boolean mFromCategory = false;
  // Pending listener that shows the keyboard once the window gains focus (see activate()).
  @Nullable
  private ViewTreeObserver.OnWindowFocusChangeListener mShowKeyboardOnFocus;
  @NonNull
  private final View mProgress;
  @NonNull
  private final View mVoiceInput;
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
    mQuery.setOnClickListener(this);
    mQuery.addTextChangedListener(mTextWatcher);
    mQuery.setOnEditorActionListener((v, actionId, event) -> {
      boolean isSearchDown =
          (event != null && event.getAction() == KeyEvent.ACTION_DOWN && event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);

      boolean isSearchAction = (actionId == EditorInfo.IME_ACTION_SEARCH);

      return (isSearchDown || isSearchAction) && onStartSearchClick();
    });
    mProgress = mSearchContainer.findViewById(R.id.progress);
    mVoiceInput = root.findViewById(R.id.voice_input);
    mVoiceInput.setOnClickListener(this);
    showProgress(false);
    updateViewsVisibility(true);
  }

  private void updateViewsVisibility(boolean queryEmpty)
  {
    UiUtils.showIf(showBackButton(), mBack);
    UiUtils.showIf(supportsVoiceSearch() && queryEmpty && mVoiceInputSupported, mVoiceInput);
  }

  private void onQueryChanged(@Nullable CharSequence s, boolean resetCategoryFlag)
  {
    if (resetCategoryFlag)
      mFromCategory = false;
    final boolean isEmpty = TextUtils.isEmpty(s);
    mBackPressedCallback.setEnabled(!isEmpty);
    updateViewsVisibility(isEmpty);
    onTextChanged(s == null ? "" : s.toString());
  }

  protected boolean showBackButton()
  {
    return true;
  }

  protected void onQueryClick(String query) {}

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

  protected void disableQueryEditing()
  {
    mQuery.setFocusable(false);
    mQuery.setLongClickable(false);
    mQuery.setInputType(InputType.TYPE_NULL);
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
    if (UiUtils.isVisible(mProgress) == show)
      return;
    ViewParent parent = mSearchContainer.getParent();
    if (parent instanceof SuppressLayoutLinearLayout)
    {
      ((SuppressLayoutLinearLayout) parent).setSuppressLayout(true);
      UiUtils.showIf(show, mProgress);
      ((SuppressLayoutLinearLayout) parent).setSuppressLayout(false);
    }
    else
      UiUtils.showIf(show, mProgress);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.query)
      onQueryClick(getQuery());
    else if (id == R.id.voice_input)
      onVoiceInputClick();
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
  }
}
