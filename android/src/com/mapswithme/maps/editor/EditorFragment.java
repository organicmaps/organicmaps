package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v7.widget.SwitchCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.editor.data.TimeFormatUtils;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.util.UiUtils;

public class EditorFragment extends BaseMwmFragment implements View.OnClickListener
{
  private View mNameBlock;
  private View mAddressBlock;
  private View mMetadataBlock;
  private EditText mEtName;
  private TextView mTvLocalizedNames;
  private TextView mTvStreet;
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
  private View mOpeningHoursBlock;
  private View mEmptyOpeningHours;
  private TextView mOpeningHours;
  private View mEditOpeningHours;

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

    // TODO(yunikkk): Add multilanguages support.
    mEtName.setText(Editor.nativeGetDefaultName());
    mTvStreet.setText(Editor.nativeGetStreet());
    mEtHouseNumber.setText(Editor.nativeGetHouseNumber());
    mEtPhone.setText(Editor.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
    mEtWebsite.setText(Editor.getMetadata(Metadata.MetadataType.FMD_WEBSITE));
    mEtEmail.setText(Editor.getMetadata(Metadata.MetadataType.FMD_EMAIL));
    mTvCuisine.setText(Editor.nativeGetFormattedCuisine());
    mSwWifi.setChecked(Editor.nativeHasWifi());
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
    return mSwWifi.isChecked() ? "wlan" : "";
  }

  public String getOpeningHours()
  {
    return Editor.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
  }

  public Metadata getMetadata()
  {
    final Metadata res = new Metadata();
    res.addMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, mOpeningHours.getText().toString());
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

    final int[] editableMeta = Editor.nativeGetEditableFields();
    if (editableMeta.length == 0)
    {
      UiUtils.hide(mMetadataBlock);
      return;
    }

    UiUtils.show(mMetadataBlock);
    UiUtils.hide(mOpeningHoursBlock, mPhoneBlock, mWebBlock, mEmailBlock, mCuisineBlock, mWifiBlock);
    boolean anyEditableMeta = false;
    for (int type : editableMeta)
    {
      switch (Metadata.MetadataType.fromInt(type))
      {
      case FMD_OPEN_HOURS:
        anyEditableMeta = true;
        UiUtils.show(mOpeningHoursBlock);
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
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(Editor.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS));
    if (timetables == null)
    {
      UiUtils.show(mEmptyOpeningHours);
      UiUtils.hide(mOpeningHours, mEditOpeningHours);
    }
    else
    {
      UiUtils.hide(mEmptyOpeningHours);
      UiUtils.setTextAndShow(mOpeningHours, TimeFormatUtils.formatTimetables(timetables));
      UiUtils.show(mEditOpeningHours);
    }
  }

  private void initViews(View view)
  {
    mNameBlock = view.findViewById(R.id.cv__name);
    mAddressBlock = view.findViewById(R.id.cv__address);
    mMetadataBlock = view.findViewById(R.id.cv__metadata);
    mEtName = findInput(view.findViewById(R.id.name));
    view.findViewById(R.id.block_street).setOnClickListener(this);
    mTvStreet = (TextView) view.findViewById(R.id.street);
    mEtHouseNumber = findInput(view.findViewById(R.id.building));
    mPhoneBlock = view.findViewById(R.id.block_phone);
    mEtPhone = findInput(mPhoneBlock);
    mWebBlock = view.findViewById(R.id.block_website);
    mEtWebsite = findInput(mWebBlock);
    mEmailBlock = view.findViewById(R.id.block_email);
    mEtEmail = findInput(mEmailBlock);
    mCuisineBlock = view.findViewById(R.id.block_cuisine);
    mCuisineBlock.setOnClickListener(this);
    mTvCuisine = (TextView) view.findViewById(R.id.tv__cuisine);
    mWifiBlock = view.findViewById(R.id.block_wifi);
    mSwWifi = (SwitchCompat) view.findViewById(R.id.sw__wifi);
    mWifiBlock.setOnClickListener(this);
    mOpeningHoursBlock = view.findViewById(R.id.block_opening_hours);
    mEditOpeningHours = mOpeningHoursBlock.findViewById(R.id.edit_opening_hours);
    mEditOpeningHours.setOnClickListener(this);
    mEmptyOpeningHours = mOpeningHoursBlock.findViewById(R.id.empty_opening_hours);
    mEmptyOpeningHours.setOnClickListener(this);
    mOpeningHours = (TextView) mOpeningHoursBlock.findViewById(R.id.opening_hours);
    mOpeningHours.setOnClickListener(this);
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
    case R.id.edit_opening_hours:
    case R.id.empty_opening_hours:
    case R.id.opening_hours:
      mParent.editTimetable();
      break;
    case R.id.block_wifi:
      mSwWifi.toggle();
      break;
    case R.id.block_street:
      mParent.editStreet();
      break;
    case R.id.block_cuisine:
      mParent.editCuisine();
      break;
    }
  }
}
