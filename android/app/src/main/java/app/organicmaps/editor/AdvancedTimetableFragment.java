package app.organicmaps.editor;

import android.os.Bundle;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.TextView;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.view.AccessibilityDelegateCompat;
import androidx.core.view.ViewCompat;
import androidx.core.view.accessibility.AccessibilityNodeInfoCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.sdk.editor.OpeningHours;
import app.organicmaps.sdk.util.Constants;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.widget.ExpandableSectionAccessibility;
import com.google.android.material.textfield.TextInputEditText;

public class AdvancedTimetableFragment extends BaseMwmFragment implements View.OnClickListener, TimetableProvider
{
  private boolean mIsExampleShown;
  private TextInputEditText mInput;
  private WebView mExample;
  private TextView mExamplesTitle;
  private ExpandableSectionAccessibility mExamplesAccessibility;
  private static TextView mSaveButton;
  @Nullable
  private String mInitTimetables;
  @Nullable
  TimetableChangedListener mListener;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_timetable_advanced, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    initViews(view);
    refreshTimetables();
    showExample(false);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    refreshTimetables();
  }

  private void initViews(View view)
  {
    view.findViewById(R.id.examples).setOnClickListener(this);
    view.findViewById(R.id.examples).setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
    view.findViewById(R.id.examples).setFocusable(false);
    mInput = view.findViewById(R.id.et__timetable);
    ViewCompat.setAccessibilityDelegate(mInput, new AccessibilityDelegateCompat() {
      @Override
      public void onInitializeAccessibilityNodeInfo(@NonNull View host, @NonNull AccessibilityNodeInfoCompat info)
      {
        super.onInitializeAccessibilityNodeInfo(host, info);
        info.setHintText(getString(R.string.editor_opening_hours_input));
        info.setShowingHintText(TextUtils.isEmpty(info.getText()));
      }
    });
    mExample = view.findViewById(R.id.wv__examples);
    mExample.getSettings().setJavaScriptEnabled(true);
    mExample.loadUrl(Constants.Url.OPENING_HOURS_MANUAL);
    mExamplesTitle = view.findViewById(R.id.tv__examples_title);
    mExamplesTitle.setOnClickListener(v -> showExample(!mIsExampleShown));
    mExamplesAccessibility = new ExpandableSectionAccessibility(mExamplesTitle, () -> true, () -> mIsExampleShown);
    setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_more);
    setTextChangedListener(mInput, mListener);
    mSaveButton = getParentFragment().getParentFragment().getView().findViewById(R.id.save);
  }

  private void showExample(boolean show)
  {
    mIsExampleShown = show;
    mExamplesAccessibility.update();
    if (mIsExampleShown)
    {
      UiUtils.show(mExample);
      setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_less);
    }
    else
    {
      UiUtils.hide(mExample);
      setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_more);
    }
  }

  private void setExampleDrawables(@DrawableRes int left, @DrawableRes int right)
  {
    mExamplesTitle.setCompoundDrawablesRelativeWithIntrinsicBounds(
        Graphics.tint(requireActivity(), left, androidx.appcompat.R.attr.colorAccent), null,
        Graphics.tint(requireActivity(), right, androidx.appcompat.R.attr.colorAccent), null);
  }

  @Override
  public void onClick(View v)
  {
    if (v.getId() == R.id.examples)
      showExample(!mIsExampleShown);
  }

  @Nullable
  @Override
  public String getTimetables()
  {
    return mInput.getText().toString();
  }

  @Override
  public void setTimetables(@Nullable String timetables)
  {
    mInitTimetables = timetables;
    refreshTimetables();
  }

  private void refreshTimetables()
  {
    if (mInput == null || mInitTimetables == null)
      return;

    mInput.setText(mInitTimetables);
    mInput.requestFocus();
    InputUtils.showKeyboard(mInput);
  }

  void setTimetableChangedListener(@NonNull TimetableChangedListener listener)
  {
    mListener = listener;
    setTextChangedListener(mInput, mListener);
  }

  private static void setTextChangedListener(@Nullable TextInputEditText input,
                                             @Nullable TimetableChangedListener listener)
  {
    if (input == null || listener == null)
      return;

    input.addTextChangedListener(new TextWatcher() {
      @Override
      public void beforeTextChanged(CharSequence s, int start, int count, int after)
      {}

      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        mSaveButton.setEnabled(OpeningHours.nativeIsTimetableStringValid(s.toString()));
      }

      @Override
      public void afterTextChanged(Editable s)
      {
        listener.onTimetableChanged(s.toString());
      }
    });
  }
}
