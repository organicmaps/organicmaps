package com.mapswithme.maps.widget;

import android.app.Activity;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class SearchToolbarController extends ToolbarController
                                  implements View.OnClickListener
{
  public interface Container
  {
    SearchToolbarController getController();
  }

  private final EditText mQuery;
  private final View mProgress;
  private final View mClear;
  private final View mVoiceInput;

  private final boolean mVoiceInputSupported = InputUtils.isVoiceInputSupported(mActivity);

  private final TextWatcher mTextWatcher = new StringUtils.SimpleTextWatcher()
  {
    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
      updateButtons(TextUtils.isEmpty(s));
      SearchToolbarController.this.onTextChanged(s.toString());
    }
  };

  public SearchToolbarController(View root, Activity activity)
  {
    super(root, activity);

    mQuery = (EditText) mToolbar.findViewById(R.id.query);
    mQuery.setOnClickListener(this);
    mQuery.addTextChangedListener(mTextWatcher);
    mQuery.setOnEditorActionListener(new TextView.OnEditorActionListener()
    {
      @Override
      public boolean onEditorAction(TextView v, int actionId, KeyEvent event)
      {
        boolean isSearchDown = (event != null &&
                                event.getAction() == KeyEvent.ACTION_DOWN &&
                                event.getKeyCode() == KeyEvent.KEYCODE_SEARCH);

        boolean isSearchAction = (actionId == EditorInfo.IME_ACTION_SEARCH);

        return (isSearchDown || isSearchAction) && onStartSearchClick();
      }
    });

    mProgress = mToolbar.findViewById(R.id.progress);

    mVoiceInput = mToolbar.findViewById(R.id.voice_input);
    mVoiceInput.setOnClickListener(this);

    mClear = mToolbar.findViewById(R.id.clear);
    mClear.setOnClickListener(this);

    showProgress(false);
    updateButtons(true);
  }

  private void updateButtons(boolean queryEmpty)
  {
    UiUtils.showIf(queryEmpty && mVoiceInputSupported, mVoiceInput);
    UiUtils.showIf(!queryEmpty, mClear);
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

  protected void onVoiceInputClick() {}

  public String getQuery()
  {
    return mQuery.getText().toString();
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
}
