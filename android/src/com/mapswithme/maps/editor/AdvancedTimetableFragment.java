package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.util.Constants;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.UiUtils;

public class AdvancedTimetableFragment extends BaseMwmFragment
                                    implements View.OnClickListener,
                                               TimetableFragment.TimetableProvider
{
  private boolean mIsExampleShown;
  private EditText mInput;
  private WebView mExample;
  private Timetable[] mInitTimetables;
  private TextView mExamplesTitle;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_timetable_advanced, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
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
    mInput = (EditText) view.findViewById(R.id.et__timetable);
    mExample = (WebView) view.findViewById(R.id.wv__examples);
    mExample.loadUrl(Constants.Url.OPENING_HOURS_MANUAL);
    mExamplesTitle = (TextView) view.findViewById(R.id.tv__examples_title);
    setExampleDrawables(R.drawable.ic_type_text, R.drawable.ic_expand_more);
  }

  private void showExample(boolean show)
  {
    mIsExampleShown = show;
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
    mExamplesTitle.setCompoundDrawablesWithIntrinsicBounds(Graphics.tint(getActivity(), left, R.attr.colorAccent), null,
                                                           Graphics.tint(getActivity(), right, R.attr.colorAccent), null);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.examples:
      showExample(!mIsExampleShown);
    }
  }

  @Override
  public String getTimetables()
  {
    return mInput.getText().toString();
  }

  public void setTimetables(String timetables)
  {
    mInitTimetables = OpeningHours.nativeTimetablesFromString(timetables);
    refreshTimetables();
  }

  private void refreshTimetables()
  {
    if (mInput != null && mInitTimetables != null)
      mInput.setText(OpeningHours.nativeTimetablesToString(mInitTimetables));
  }
}
