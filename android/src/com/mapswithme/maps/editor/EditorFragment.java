package com.mapswithme.maps.editor;

import android.os.Bundle;
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

  private View mNameBlock;
  private View mAddressBlock;
  private View mMetadataBlock;
  private EditText mEtName;
  private TextView mTvLocalizedNames;
  private TextView mTvStreet;
  private View mOpeningHours;
  private View mEditOpeningHours;
  private TextView mTvOpeningHours;
  private EditText mEtHouseNumber;
  private View mPhoneBlock;
  private EditText mEtPhone;
  private View mWebBlock;
  private EditText mEtWebsite;
  private View mEmailBlock;
  private EditText mEtEmail;
  private View mCuisineBlock;
  private TextView mTvCuisine;
  private View mWifiBlock;
  private SwitchCompat mSwWifi;
  private TextView mEmptyOpeningHours;
  private TextView mTvSchedule;

  protected EditorHostFragment mParent;

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

    mParent = (EditorHostFragment) getParentFragment();

    initViews(view);

    mEditedPoi = getArguments().getParcelable(EditorHostFragment.EXTRA_MAP_OBJECT);
    if (mEditedPoi == null)
      throw new IllegalStateException("Valid MapObject should be passed to edit it.");
    mEtName.setText(mEditedPoi.getName());
    // TODO read names
    //    mTvLocalizedNames.setText();
    mTvStreet.setText(mEditedPoi.getStreet());
    mEtHouseNumber.setText(mEditedPoi.getHouseNumber());
    mEtPhone.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
    mEtWebsite.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_WEBSITE));
    mEtEmail.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_EMAIL));
    mTvCuisine.setText(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_CUISINE));
    mSwWifi.setChecked(!TextUtils.isEmpty(mEditedPoi.getMetadata(Metadata.MetadataType.FMD_INTERNET)));
    refreshOpeningTime();

    refreshEditableFields();
  }

  public String getName()
  {
    // TODO add localized names
    return mEtName.getText().toString();
  }

  public String getStreet()
  {
    return mTvStreet.getText().toString();
  }

  public String getHouseNumber()
  {
    return mEtHouseNumber.getText().toString();
  }

  public String getPhone()
  {
    return mEtPhone.getText().toString();
  }

  public String getWebsite()
  {
    return mEtWebsite.getText().toString();
  }

  public String getEmail()
  {
    return mEtEmail.getText().toString();
  }

  public String getCuisine()
  {
    return mTvCuisine.getText().toString();
  }

  public String getWifi()
  {
    return mSwWifi.isChecked() ? "Yes" : "";
  }

  public Metadata getMetadata()
  {
    final Metadata res = new Metadata();
    res.addMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, mTvOpeningHours.getText().toString());
    res.addMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER, mEtPhone.getText().toString());
    res.addMetadata(Metadata.MetadataType.FMD_WEBSITE, mEtWebsite.getText().toString());
    res.addMetadata(Metadata.MetadataType.FMD_EMAIL, mEtEmail.getText().toString());
    res.addMetadata(Metadata.MetadataType.FMD_CUISINE, mTvCuisine.getText().toString());
    res.addMetadata(Metadata.MetadataType.FMD_INTERNET, mSwWifi.isChecked() ? "Yes" : "");
    return res;
  }

  private void refreshEditableFields()
  {
    UiUtils.showIf(Editor.nativeIsNameEditable(), mNameBlock);
    UiUtils.showIf(Editor.nativeIsAddressEditable(), mAddressBlock);

    final int[] editableMeta = Editor.nativeGetEditableMetadata();
    if (editableMeta.length == 0)
    {
      UiUtils.hide(mMetadataBlock);
      return;
    }

    UiUtils.show(mMetadataBlock);
    UiUtils.hide(mOpeningHours, mEditOpeningHours, mPhoneBlock, mWebBlock, mEmailBlock, mCuisineBlock, mWifiBlock);
    boolean anyEditableMeta = false;
    for (int type : editableMeta)
    {
      switch (Metadata.MetadataType.fromInt(type))
      {
      case FMD_OPEN_HOURS:
        anyEditableMeta = true;
        UiUtils.show(mOpeningHours, mEditOpeningHours);
        break;
      case FMD_PHONE_NUMBER:
        anyEditableMeta = true;
        UiUtils.show(mPhoneBlock);
        break;
      case FMD_WEBSITE:
        anyEditableMeta = true;
        UiUtils.show(mWebBlock);
        break;
      case FMD_EMAIL:
        anyEditableMeta = true;
        UiUtils.show(mEmailBlock);
        break;
      case FMD_CUISINE:
        anyEditableMeta = true;
        UiUtils.show(mCuisineBlock);
        break;
      case FMD_INTERNET:
        anyEditableMeta = true;
        UiUtils.show(mWifiBlock);
        break;
      }
    }
    if (!anyEditableMeta)
      UiUtils.hide(mMetadataBlock);
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
    mNameBlock = view.findViewById(R.id.cv__name);
    mAddressBlock = view.findViewById(R.id.cv__address);
    mMetadataBlock = view.findViewById(R.id.cv__metadata);
    mEtName = findInput(view.findViewById(R.id.name));
    mTvStreet = (TextView) view.findViewById(R.id.street);
    mEtHouseNumber = findInput(view.findViewById(R.id.building));
    mPhoneBlock = view.findViewById(R.id.block_phone);
    mEtPhone = findInput(mPhoneBlock);
    mWebBlock = view.findViewById(R.id.block_website);
    mEtWebsite = findInput(mWebBlock);
    mEmailBlock = view.findViewById(R.id.block_email);
    mEtEmail = findInput(mEmailBlock);
    mCuisineBlock = view.findViewById(R.id.block_cuisine);
    mTvCuisine = (TextView) view.findViewById(R.id.tv__cuisine);
    mWifiBlock = view.findViewById(R.id.block_wifi);
    mSwWifi = (SwitchCompat) view.findViewById(R.id.sw__wifi);
    mWifiBlock.setOnClickListener(this);
    mOpeningHours = view.findViewById(R.id.opening_hours);
    mEditOpeningHours = view.findViewById(R.id.tv__edit_oh);
    mEditOpeningHours.setOnClickListener(this);
    mEmptyOpeningHours = (TextView) view.findViewById(R.id.et__empty_schedule);
    mTvSchedule = (TextView) view.findViewById(R.id.tv__place_schedule);
  }

  private EditText findInput(View view)
  {
    return (EditText) view.findViewById(R.id.input);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.tv__edit_oh:
      editOpeningHours();
      break;
    case R.id.block_wifi:
      mSwWifi.toggle();
      break;
    }
  }

  private void editOpeningHours()
  {
    mParent.editTimetable();
  }
}
