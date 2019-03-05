package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.design.widget.TextInputLayout;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;

import java.util.Locale;
import java.util.Objects;

public abstract class BaseEditUserBookmarkCategoryFragment extends BaseMwmToolbarFragment
{
  public static final String BUNDLE_BOOKMARK_CATEGORY = "category";
  private static final String FORMAT_TEMPLATE = "%d / %d";
  private static final String TEXT_LENGTH_LIMIT = "text_length_limit";
  private static final int DEFAULT_TEXT_LENGTH_LIMIT = 42;
  private static final String DOUBLE_BREAK_LINE_CHAR = "\n\n";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private EditText mEditText;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mCharactersAmountText;
  private int mTextLimit;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkCategory mCategory;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = Objects.requireNonNull(getArguments());
    mCategory = Objects.requireNonNull(args.getParcelable(BUNDLE_BOOKMARK_CATEGORY));
    mTextLimit = args.getInt(TEXT_LENGTH_LIMIT, getDefaultTextLengthLimit());
  }

  protected int getDefaultTextLengthLimit()
  {
    return DEFAULT_TEXT_LENGTH_LIMIT;
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_bookmark_category_restriction, container,
                                    false);
    setHasOptionsMenu(true);
    mEditText = root.findViewById(R.id.edit_text_field);
    InputFilter[] inputFilters = { new InputFilter.LengthFilter(mTextLimit) };
    TextInputLayout editTextContainer = root.findViewById(R.id.edit_text_container);
    editTextContainer.setHint(getString(getHintText()));
    mEditText.setFilters(inputFilters);
    mEditText.setText(getEditableText());
    mEditText.addTextChangedListener(new TextRestrictionWatcher());
    mCharactersAmountText = root.findViewById(R.id.characters_amount);
    mCharactersAmountText.setText(makeFormattedCharsAmount(getEditableText(), mTextLimit));
    TextView summaryView = root.findViewById(R.id.summary);
    summaryView.setText(getTopSummaryText());
    summaryView.append(DOUBLE_BREAK_LINE_CHAR);
    summaryView.append(getBottomSummaryText());
    return root;
  }

  @NonNull
  protected abstract  CharSequence getTopSummaryText();

  @NonNull
  protected abstract CharSequence getBottomSummaryText();

  @StringRes
  protected abstract int getHintText();

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.menu_bookmark_category_restriction, menu);
  }

  @Override
  public void onPrepareOptionsMenu(Menu menu)
  {
    super.onPrepareOptionsMenu(menu);
    MenuItem item = menu.findItem(R.id.done);
    item.setVisible(mEditText.getEditableText().length() > 0);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (resultCode == Activity.RESULT_OK)
    {
      getActivity().setResult(Activity.RESULT_OK, data);
      getActivity().finish();
    }
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.done)
    {
      onDoneOptionItemClicked();
      return true;
    }

    return super.onOptionsItemSelected(item);
  }

  @NonNull
  protected BookmarkCategory getCategory()
  {
    return mCategory;
  }

  @NonNull
  protected EditText getEditText()
  {
    return mEditText;
  }

  protected abstract void onDoneOptionItemClicked();

  @NonNull
  protected abstract CharSequence getEditableText();

  @NonNull
  private static String makeFormattedCharsAmount(@Nullable CharSequence s, int limit)
  {
    return String.format(Locale.US, FORMAT_TEMPLATE, s == null ? 0 : Math.min(s.length(), limit), limit);
  }

  private class TextRestrictionWatcher implements TextWatcher
  {

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after)
    {
      /* Do nothing by default. */
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count)
    {
      if (s.length() == 0 || s.length() == 1)
        getActivity().invalidateOptionsMenu();
    }

    @Override
    public void afterTextChanged(Editable s)
    {
      String src = makeFormattedCharsAmount(s, mTextLimit);
      mCharactersAmountText.setText(src);
    }
  }
}
