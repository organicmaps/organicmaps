package com.mapswithme.maps.widget;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Intent;
import android.text.InputType;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;

import com.mapswithme.maps.R;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class SearchToolbarController extends ToolbarController implements View.OnClickListener
{
  private static final int REQUEST_VOICE_RECOGNITION = 0xCA11;
  @Nullable
  private final View mToolbarContainer;
  @NonNull
  private final View mSearchContainer;
  @NonNull
  private final View mBack;
  @NonNull
  private final EditText mQuery;
  private boolean mFromCategory = false;
  @NonNull
  private final View mProgress;
  @NonNull
  private final View mClear;
  @NonNull
  private final View mVoiceInput;
  private final boolean mVoiceInputSupported = InputUtils.isVoiceInputSupported(requireActivity());
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

  public interface Container
  {
    SearchToolbarController getController();
  }

  public SearchToolbarController(@NonNull View root,
                                 @NonNull Activity activity)
  {
    super(root, activity);
    mToolbarContainer = getToolbar().findViewById(R.id.toolbar_container);
    mSearchContainer = getToolbar().findViewById(R.id.search_container);
    mBack = mSearchContainer.findViewById(R.id.back);
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

    showProgress(false);
    updateViewsVisibility(true);
  }

  private void updateViewsVisibility(boolean queryEmpty)
  {
    UiUtils.showIf(showBackButton(), mBack);
    UiUtils.showIf(supportsVoiceSearch() && queryEmpty && mVoiceInputSupported, mVoiceInput);
    UiUtils.showIf(alwaysShowClearButton() || !queryEmpty, mClear);
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
    }
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
  public boolean isCategory() { return mFromCategory; }

  public void setQuery(CharSequence query, boolean fromCategory)
  {
    mFromCategory = fromCategory;
    mQuery.setText(query);
    if (!TextUtils.isEmpty(query))
      mQuery.setSelection(query.length());
  }
  public void setQuery(CharSequence query) { setQuery(query, false); }

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
    case R.id.clear:
      onClearClick();
      break;

    case R.id.query:
      onQueryClick(getQuery());
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
}
