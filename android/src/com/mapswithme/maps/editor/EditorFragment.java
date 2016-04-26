package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.DrawableRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.design.widget.TextInputLayout;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.SwitchCompat;
import android.util.SparseArray;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.Metadata.MetadataType;
import com.mapswithme.maps.editor.data.TimeFormatUtils;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.util.InputUtils;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;
import org.solovyev.android.views.llm.LinearLayoutManager;

public class EditorFragment extends BaseMwmFragment implements View.OnClickListener
{
  private TextView mCategory;
  private View mCardName;
  private View mCardAddress;
  private View mCardMetadata;
  private EditText mName;
  private RecyclerView mLocalizedNames;
  private View mLocalizedAdd;
  private View mLocalizedShow;
  private TextView mStreet;
  private EditText mHouseNumber;
  private EditText mZipcode;
  private View mBlockLevels;
  private EditText mBuildingLevels;
  private TextInputLayout mInputHouseNumber;
  private EditText mPhone;
  private EditText mWebsite;
  private EditText mEmail;
  private TextView mCuisine;
  private EditText mOperator;
  private SwitchCompat mWifi;
  private View mEmptyOpeningHours;
  private TextView mOpeningHours;
  private View mEditOpeningHours;
  private EditText mDescription;
  private final SparseArray<View> mMetaBlocks = new SparseArray<>(7);

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

    mCategory.setText(Editor.nativeGetCategory());
    mName.setText(Editor.nativeGetDefaultName());
    mStreet.setText(Editor.nativeGetStreet());
    mHouseNumber.setText(Editor.nativeGetHouseNumber());
    mHouseNumber.addTextChangedListener(new StringUtils.SimpleTextWatcher()
    {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        final String text = s.toString();

        if (!Editor.nativeIsHouseValid(text))
        {
          mInputHouseNumber.setError(getString(R.string.error_enter_correct_house_number));
          return;
        }

        mInputHouseNumber.setError(null);
      }
    });
    mZipcode.setText(Editor.nativeGetZipCode());
    mBuildingLevels.setText(Editor.nativeGetBuildingLevels());
    mPhone.setText(Editor.nativeGetPhone());
    mWebsite.setText(Editor.nativeGetWebsite());
    mEmail.setText(Editor.nativeGetEmail());
    mCuisine.setText(Editor.nativeGetFormattedCuisine());
    mOperator.setText(Editor.nativeGetOperator());
    mWifi.setChecked(Editor.nativeHasWifi());
    refreshOpeningTime();
    refreshEditableFields();
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    setEdits();
  }

  protected boolean setEdits()
  {
    if (!validateFields())
      return false;

    Editor.nativeSetDefaultName(mName.getText().toString());
    Editor.nativeSetHouseNumber(mHouseNumber.getText().toString());
    Editor.nativeSetZipCode(mZipcode.getText().toString());
    Editor.nativeSetBuildingLevels(mBuildingLevels.getText().toString());
    Editor.nativeSetPhone(mPhone.getText().toString());
    Editor.nativeSetWebsite(mWebsite.getText().toString());
    Editor.nativeSetEmail(mEmail.getText().toString());
    Editor.nativeSetHasWifi(mWifi.isChecked());

    return true;
  }

  @NonNull
  protected String getDescription()
  {
    return mDescription.getText().toString().trim();
  }

  private boolean validateFields()
  {
    if (!Editor.nativeIsHouseValid(mHouseNumber.getText().toString()))
    {
      mHouseNumber.requestFocus();
      InputUtils.showKeyboard(mHouseNumber);
      return false;
    }

    return true;
  }

  private void refreshEditableFields()
  {
    UiUtils.showIf(Editor.nativeIsNameEditable(), mCardName);
    UiUtils.showIf(Editor.nativeIsAddressEditable(), mCardAddress);
    UiUtils.showIf(Editor.nativeIsBuilding(), mBlockLevels);

    final int[] editableMeta = Editor.nativeGetEditableFields();
    if (editableMeta.length == 0)
    {
      UiUtils.hide(mCardMetadata);
      return;
    }

    for (int i = 0; i < mMetaBlocks.size(); i++)
      UiUtils.hide(mMetaBlocks.valueAt(i));

    boolean anyEditableMeta = false;
    for (int type : editableMeta)
    {
      final View metaBlock = mMetaBlocks.get(type);
      if (metaBlock == null)
        continue;

      anyEditableMeta = true;
      UiUtils.show(metaBlock);
    }
    UiUtils.showIf(anyEditableMeta, mCardMetadata);
  }

  private void refreshOpeningTime()
  {
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(Editor.nativeGetOpeningHours());
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
    final View categoryBlock = view.findViewById(R.id.category);
    categoryBlock.setOnClickListener(this);
    // TODO show icon and fill it when core will implement that
    UiUtils.hide(categoryBlock.findViewById(R.id.icon));
    mCategory = (TextView) categoryBlock.findViewById(R.id.name);
    mCardName = view.findViewById(R.id.cv__name);
    mCardAddress = view.findViewById(R.id.cv__address);
    mCardMetadata = view.findViewById(R.id.cv__metadata);
    mName = findInput(mCardName);
    mLocalizedAdd = view.findViewById(R.id.add_langs);
    mLocalizedShow = view.findViewById(R.id.show_langs);
    mLocalizedNames = (RecyclerView) view.findViewById(R.id.names);
    mLocalizedNames.setLayoutManager(new LinearLayoutManager(getActivity()));
    mLocalizedNames.setAdapter(new MultilanguageAdapter(Editor.nativeGetLocalizedNames()));
    // Address
    view.findViewById(R.id.block_street).setOnClickListener(this);
    mStreet = (TextView) view.findViewById(R.id.street);
    View blockHouseNumber = view.findViewById(R.id.block_building);
    mHouseNumber = findInputAndInitBlock(blockHouseNumber, 0, R.string.house_number);
    mInputHouseNumber = (TextInputLayout) blockHouseNumber.findViewById(R.id.custom_input);
    View blockZipcode = view.findViewById(R.id.block_zipcode);
    mZipcode = findInputAndInitBlock(blockZipcode, 0, R.string.editor_zip_code);
    mBlockLevels = view.findViewById(R.id.block_levels);
    // TODO set levels limit (25 or more, get it from the core)
    mBuildingLevels = findInputAndInitBlock(mBlockLevels, 0, R.string.editor_storey_number);
    // Details
    View blockPhone = view.findViewById(R.id.block_phone);
    mPhone = findInputAndInitBlock(blockPhone, R.drawable.ic_phone, R.string.phone);
    View blockWeb = view.findViewById(R.id.block_website);
    mWebsite = findInputAndInitBlock(blockWeb, R.drawable.ic_website, R.string.website);
    View blockEmail = view.findViewById(R.id.block_email);
    mEmail = findInputAndInitBlock(blockEmail, R.drawable.ic_email, R.string.email);
    View blockCuisine = view.findViewById(R.id.block_cuisine);
    blockCuisine.setOnClickListener(this);
    mCuisine = (TextView) view.findViewById(R.id.cuisine);
    View blockOperator = view.findViewById(R.id.block_operator);
    mOperator = findInputAndInitBlock(blockOperator, R.drawable.ic_operator, R.string.editor_operator);
    View blockWifi = view.findViewById(R.id.block_wifi);
    mWifi = (SwitchCompat) view.findViewById(R.id.sw__wifi);
    blockWifi.setOnClickListener(this);
    View blockOpeningHours = view.findViewById(R.id.block_opening_hours);
    mEditOpeningHours = blockOpeningHours.findViewById(R.id.edit_opening_hours);
    mEditOpeningHours.setOnClickListener(this);
    mEmptyOpeningHours = blockOpeningHours.findViewById(R.id.empty_opening_hours);
    mEmptyOpeningHours.setOnClickListener(this);
    mOpeningHours = (TextView) blockOpeningHours.findViewById(R.id.opening_hours);
    mOpeningHours.setOnClickListener(this);
    mDescription = findInput(view.findViewById(R.id.cv__more));

    mMetaBlocks.append(MetadataType.FMD_OPEN_HOURS.toInt(), blockOpeningHours);
    mMetaBlocks.append(MetadataType.FMD_PHONE_NUMBER.toInt(), blockPhone);
    mMetaBlocks.append(MetadataType.FMD_WEBSITE.toInt(), blockWeb);
    mMetaBlocks.append(MetadataType.FMD_EMAIL.toInt(), blockEmail);
    mMetaBlocks.append(MetadataType.FMD_CUISINE.toInt(), blockCuisine);
    mMetaBlocks.append(MetadataType.FMD_OPERATOR.toInt(), blockOperator);
    mMetaBlocks.append(MetadataType.FMD_INTERNET.toInt(), blockWifi);
  }

  private EditText findInput(View blockWithInput)
  {
    return (EditText) blockWithInput.findViewById(R.id.input);
  }

  private EditText findInputAndInitBlock(View blockWithInput, @DrawableRes int icon, @StringRes int hint)
  {
    ((ImageView) blockWithInput.findViewById(R.id.icon)).setImageResource(icon);
    final TextInputLayout input = (TextInputLayout) blockWithInput.findViewById(R.id.custom_input);
    input.setHint(getString(hint));
    return (EditText) input.findViewById(R.id.input);
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
      mWifi.toggle();
      break;
    case R.id.block_street:
      mParent.editStreet();
      break;
    case R.id.block_cuisine:
      mParent.editCuisine();
      break;
    case R.id.category:
      mParent.editCategory();
    }
  }
}
