package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.IdRes;
import android.support.annotation.Nullable;
import android.support.v7.widget.SwitchCompat;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.util.UiUtils;

public class EditorFragment extends BaseMwmFragment implements View.OnClickListener
{
  private MapObject mEditedPoi;

  private EditText mEtName;
  private TextView mTvLocalizedNames;
  private TextView mTvAddress;
  private TextView mTvOpeningHours;
  private EditText mEtBuilding;
  private EditText mEtPhone;
  private EditText mEtWebsite;
  private EditText mEtEmail;
  private TextView mTvCuisine;
  private SwitchCompat mSwWifi;
  private TextView mEmptyOpeningHours;
  private TextView mTvSchedule;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_editor, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    initViews(view);

    mEditedPoi = getArguments().getParcelable(EditorHostFragment.EXTRA_MAP_OBJECT);
    if (mEditedPoi == null)
      throw new IllegalStateException("Valid MapObject should be passed to edit it.");
    mEtName.setText(mEditedPoi.getName());
    //    mTvLocalizedNames.setText();
    //    mTvAddress.setText();
    mEtPhone.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
    mEtWebsite.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_WEBSITE));
    mEtEmail.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_EMAIL));
    mTvCuisine.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_CUISINE));
    mSwWifi.setChecked(!TextUtils.isEmpty(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_INTERNET)));
    refreshOpeningTime();
  }

  private void refreshOpeningTime()
  {
    final String openingTime = mEditedPoi.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    if (TextUtils.isEmpty(openingTime))
    {
      UiUtils.show(mEmptyOpeningHours);
      UiUtils.hide(mTvSchedule);
    }
    else
    {
      UiUtils.hide(mEmptyOpeningHours);
      UiUtils.setTextAndShow(mTvSchedule, formatOpeningHours(openingTime));
    }
  }

  private String formatOpeningHours(String openingTime)
  {
    // TODO
    return openingTime;
  }

  private void initViews(View view)
  {
    mEtName = findInput(view, R.id.name);
    mEtBuilding = findInput(view, R.id.building);
    mEtPhone = findInput(view, R.id.phone);
    mEtWebsite = findInput(view, R.id.website);
    mEtEmail = findInput(view, R.id.email);
    mTvCuisine = (TextView) view.findViewById(R.id.tv__cuisine);
    mSwWifi = (SwitchCompat) view.findViewById(R.id.sw__wifi);
    view.findViewById(R.id.tv__edit_oh).setOnClickListener(this);
    mEmptyOpeningHours = (TextView) view.findViewById(R.id.et__empty_schedule);
    mTvSchedule = (TextView) view.findViewById(R.id.tv__place_schedule);
    UiUtils.hide(view.findViewById(R.id.tv__today_schedule));
  }

  private EditText findInput(View view, @IdRes int name)
  {
    return (EditText) view.findViewById(name).findViewById(R.id.input);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.tv__edit_oh:
      editOpeningHours();
      break;
    }
  }

  private void editOpeningHours()
  {

  }
}
