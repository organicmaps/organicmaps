package app.organicmaps.editor;

import android.content.Context;
import android.os.Bundle;
import android.text.InputType;
import android.text.TextUtils;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;
import androidx.annotation.CallSuper;
import androidx.annotation.DrawableRes;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.appcompat.widget.SwitchCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.dialog.EditTextDialogFragment;
import app.organicmaps.editor.data.TimeFormatUtils;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.editor.OpeningHours;
import app.organicmaps.sdk.editor.data.LocalizedName;
import app.organicmaps.sdk.editor.data.LocalizedStreet;
import app.organicmaps.sdk.editor.data.Timetable;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.Utils;
import app.organicmaps.util.Graphics;
import app.organicmaps.util.InputUtils;
import app.organicmaps.util.UiUtils;
import com.google.android.material.dialog.MaterialAlertDialogBuilder;
import com.google.android.material.textfield.TextInputEditText;
import com.google.android.material.textfield.TextInputLayout;
import java.util.HashMap;
import java.util.Map;

public class EditorFragment extends BaseMwmFragment implements View.OnClickListener
{
  final static String LAST_INDEX_OF_NAMES_ARRAY = "LastIndexOfNamesArray";

  private TextView mCategory;
  private View mCardName;
  private View mCardAddress;
  private View mCardDetails;
  private View mCardSocialMedia;
  private View mCardBuilding;

  private RecyclerView mNamesView;

  // RecyclerView observer to refresh names caption when adapter changes
  private final RecyclerView.AdapterDataObserver mNamesObserver = new RecyclerView.AdapterDataObserver() {
    @Override
    public void onChanged() { refreshNamesCaption(); }
    @Override
    public void onItemRangeChanged(int positionStart, int itemCount) { refreshNamesCaption(); }
    @Override
    public void onItemRangeInserted(int positionStart, int itemCount) { refreshNamesCaption(); }
    @Override
    public void onItemRangeRemoved(int positionStart, int itemCount) { refreshNamesCaption(); }
    @Override
    public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) { refreshNamesCaption(); }
  };

  private MultilanguageAdapter mNamesAdapter;
  private TextView mNamesCaption;
  private TextView mAddLanguage;
  private TextView mMoreLanguages;

  private TextView mStreet;
  private TextInputEditText mHouseNumber;
  private TextInputEditText mBuildingLevels;

  private TextView mPhone;
  private TextView mEditPhoneLink;
  private TextView mCuisine;
  private SwitchCompat mWifi;
  private TextView mSelfService;
  private SwitchCompat mOutdoorSeating;

  // Default Metadata entries.
  private static final class MetadataEntry
  {
    TextInputEditText mEdit;
    TextInputLayout mInput;
  }
  Map<Metadata.MetadataType, MetadataEntry> mMetadata = new HashMap<>();

  private void initMetadataEntry(Metadata.MetadataType type, @StringRes int error)
  {
    final MetadataEntry e = mMetadata.get(type);
    final int id = type.toInt();
    e.mEdit.setText(Editor.nativeGetMetadata(id));
    e.mEdit.addTextChangedListener(new StringUtils.SimpleTextWatcher() {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        UiUtils.setInputError(e.mInput, Editor.nativeIsMetadataValid(id, s.toString()) ? 0 : error);
      }
    });
  }

  private TextInputLayout mInputHouseNumber;
  private TextInputLayout mInputBuildingLevels;

  private View mEmptyOpeningHours;
  private TextView mOpeningHours;
  private View mEditOpeningHours;
  private TextInputEditText mDescription;

  // Define Metadata entries that have more complex logic separately.
  private final Map<Metadata.MetadataType, View> mDetailsBlocks = new HashMap<>();
  private final Map<Metadata.MetadataType, View> mSocialMediaBlocks = new HashMap<>();

  private TextView mReset;

  private EditorHostFragment mParent;

  // ✅ Added variable to track expanded/collapsed state of Social Media
  private boolean mSocialMediaExpanded = false;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_editor, container, false);
  }

  @CallSuper
  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    mParent = (EditorHostFragment) getParentFragment();

    initViews(view);

    mCategory.setText(Utils.getLocalizedFeatureType(requireContext(), Editor.nativeGetCategory()));
    final LocalizedStreet street = Editor.nativeGetStreet();
    mStreet.setText(street.defaultName);

    mHouseNumber.setText(Editor.nativeGetHouseNumber());
    mHouseNumber.addTextChangedListener(new StringUtils.SimpleTextWatcher() {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        UiUtils.setInputError(mInputHouseNumber,
                              Editor.nativeIsHouseValid(s.toString()) ? 0 : R.string.error_enter_correct_house_number);
      }
    });

    // TODO Reimplement this to avoid https://github.com/organicmaps/organicmaps/issues/9049
    initMetadataEntry(Metadata.MetadataType.FMD_POSTCODE, R.string.error_enter_correct_zip_code);

    mBuildingLevels.setText(Editor.nativeGetBuildingLevels());
    mBuildingLevels.addTextChangedListener(new StringUtils.SimpleTextWatcher() {
      @Override
      public void onTextChanged(CharSequence s, int start, int before, int count)
      {
        final Context context = mInputBuildingLevels.getContext();
        final boolean isValid = Editor.nativeIsLevelValid(s.toString());
        UiUtils.setInputError(mInputBuildingLevels,
                              isValid ? null
                                      : context.getString(R.string.error_enter_correct_storey_number,
                                                          Editor.nativeGetMaxEditableBuildingLevels()));
      }
    });

    mPhone.setText(Editor.nativeGetPhone());

    initMetadataEntry(Metadata.MetadataType.FMD_WEBSITE, R.string.error_enter_correct_web);
    initMetadataEntry(Metadata.MetadataType.FMD_WEBSITE_MENU, R.string.error_enter_correct_web);
    initMetadataEntry(Metadata.MetadataType.FMD_EMAIL, R.string.error_enter_correct_email);
    initMetadataEntry(Metadata.MetadataType.FMD_CONTACT_FACEBOOK, R.string.error_enter_correct_facebook_page);
    initMetadataEntry(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM, R.string.error_enter_correct_instagram_page);
    initMetadataEntry(Metadata.MetadataType.FMD_CONTACT_TWITTER, R.string.error_enter_correct_twitter_page);
    initMetadataEntry(Metadata.MetadataType.FMD_CONTACT_VK, R.string.error_enter_correct_vk_page);
    initMetadataEntry(Metadata.MetadataType.FMD_CONTACT_LINE, R.string.error_enter_correct_line_page);

    mCuisine.setText(Editor.nativeGetFormattedCuisine());
    String selfServiceMetadata = Editor.nativeGetMetadata(Metadata.MetadataType.FMD_SELF_SERVICE.toInt());
    mSelfService.setText(Utils.getTagValueLocalized(view.getContext(), "self_service", selfServiceMetadata));
    initMetadataEntry(Metadata.MetadataType.FMD_OPERATOR, 0);
    mWifi.setChecked(Editor.nativeHasWifi());

    // Temporarily hide outdoor seating
    UiUtils.hide(findViewById(R.id.block_outdoor_seating));

    refreshOpeningTime();
    refreshEditableFields();
    refreshResetButton();
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    setEdits();
  }

  boolean setEdits()
  {
    if (!validateFields())
      return false;

    Editor.nativeSetHouseNumber(mHouseNumber.getText().toString());
    Editor.nativeSetBuildingLevels(mBuildingLevels.getText().toString());
    Editor.nativeSetHasWifi(mWifi.isChecked());
    Editor.nativeSetNames(mParent.getNamesAsArray());

    for (var e : mMetadata.entrySet())
      Editor.nativeSetMetadata(e.getKey().toInt(), e.getValue().mEdit.getText().toString());

    return true;
  }

  @NonNull
  protected String getDescription()
  {
    return mDescription.getText().toString().trim();
  }

  private boolean validateFields()
  {
    if (Editor.nativeIsAddressEditable())
    {
      if (!Editor.nativeIsHouseValid(mHouseNumber.getText().toString()))
      {
        mHouseNumber.requestFocus();
        InputUtils.showKeyboard(mHouseNumber);
        return false;
      }

      if (!Editor.nativeIsLevelValid(mBuildingLevels.getText().toString()))
      {
        mBuildingLevels.requestFocus();
        InputUtils.showKeyboard(mBuildingLevels);
        return false;
      }
    }

    for (var e : mMetadata.entrySet())
    {
      final TextInputEditText edit = e.getValue().mEdit;
      if (!Editor.nativeIsMetadataValid(e.getKey().toInt(), edit.getText().toString()))
      {
        edit.requestFocus();
        InputUtils.showKeyboard(edit);
        return false;
      }
    }

    return validateNames();
  }

  private boolean validateNames()
  {
    for (int pos = 0; pos < mNamesAdapter.getItemCount(); pos++)
    {
      LocalizedName localizedName = mNamesAdapter.getNameAtPos(pos);
      if (Editor.nativeIsNameValid(localizedName.name))
        continue;

      View nameView = mNamesView.getChildAt(pos);
      nameView.requestFocus();
      InputUtils.showKeyboard(nameView);
      return false;
    }
    return true;
  }

  private void refreshEditableFields()
  {
    UiUtils.showIf(Editor.nativeIsNameEditable(), mCardName);
    UiUtils.showIf(Editor.nativeIsAddressEditable(), mCardAddress);
    UiUtils.showIf(Editor.nativeIsBuilding() && !Editor.nativeIsPointType(), mCardBuilding);

    final int[] editableDetails = Editor.nativeGetEditableProperties();

    setCardVisibility(mCardDetails, mDetailsBlocks, editableDetails);
    setCardVisibility(mCardSocialMedia, mSocialMediaBlocks, editableDetails);
  }

  private void setCardVisibility(View card, Map<Metadata.MetadataType, View> blocks, int[] editableDetails)
  {
    for (var e : blocks.entrySet())
      UiUtils.hide(e.getValue());

    boolean anyBlockElement = false;
    for (int type : editableDetails)
    {
      final View blockElement = blocks.get(Metadata.MetadataType.fromInt(type));
      if (blockElement == null)
        continue;

      anyBlockElement = true;
      UiUtils.show(blockElement);
    }
    UiUtils.showIf(anyBlockElement, card);
  }

  private void initViews(View view)
  {
    final View categoryBlock = view.findViewById(R.id.category);
    // TODO show icon and fill it when core will implement that
    UiUtils.hide(categoryBlock.findViewById(R.id.icon));
    mCategory = categoryBlock.findViewById(R.id.name);
    mCardName = view.findViewById(R.id.cv__name);
    mCardAddress = view.findViewById(R.id.cv__address);
    mCardDetails = view.findViewById(R.id.cv__details);
    mCardSocialMedia = view.findViewById(R.id.cv__social_media);
    mCardBuilding = view.findViewById(R.id.cv__building);
    initNamesView(view);

    view.findViewById(R.id.block_street).setOnClickListener(this);
    mStreet = view.findViewById(R.id.street);

    View blockHouseNumber = view.findViewById(R.id.block_building);
    mHouseNumber = findInputAndInitBlock(blockHouseNumber, R.drawable.ic_building, R.string.house_number);
    mInputHouseNumber = blockHouseNumber.findViewById(R.id.custom_input);

    initBlock(view, Metadata.MetadataType.FMD_POSTCODE, R.id.block_zipcode, R.drawable.ic_address,
              R.string.editor_zip_code, 0);

    // Details
    View mBlockLevels = view.findViewById(R.id.block_levels);
    mBuildingLevels =
        findInputAndInitBlock(mBlockLevels, R.drawable.ic_floor,
                              getString(R.string.editor_storey_number, Editor.nativeGetMaxEditableBuildingLevels()));
    mBuildingLevels.setInputType(InputType.TYPE_CLASS_NUMBER);
    mInputBuildingLevels = mBlockLevels.findViewById(R.id.custom_input);

    View blockPhone = view.findViewById(R.id.block_phone);
    mPhone = blockPhone.findViewById(R.id.phone);
    mEditPhoneLink = blockPhone.findViewById(R.id.edit_phone);
    mEditPhoneLink.setOnClickListener(this);
    mPhone.setOnClickListener(this);

    View websiteBlock = initBlock(view, Metadata.MetadataType.FMD_WEBSITE, R.id.block_website, R.drawable.ic_website,
                                  R.string.website, InputType.TYPE_TEXT_VARIATION_URI);
    View websiteMenuBlock =
        initBlock(view, Metadata.MetadataType.FMD_WEBSITE_MENU, R.id.block_website_menu, R.drawable.ic_website_menu,
                  R.string.website_menu, InputType.TYPE_TEXT_VARIATION_URI);
    View emailBlock = initBlock(view, Metadata.MetadataType.FMD_EMAIL, R.id.block_email, R.drawable.ic_email,
                                R.string.email, InputType.TYPE_TEXT_VARIATION_EMAIL_ADDRESS);
    View facebookContactBlock =
        initBlock(view, Metadata.MetadataType.FMD_CONTACT_FACEBOOK, R.id.block_facebook, R.drawable.ic_facebook_white,
                  R.string.facebook, InputType.TYPE_TEXT_VARIATION_URI);
    View instagramContactBlock =
        initBlock(view, Metadata.MetadataType.FMD_CONTACT_INSTAGRAM, R.id.block_instagram,
                  R.drawable.ic_instagram_white, R.string.instagram, InputType.TYPE_TEXT_VARIATION_URI);
    View twitterContactBlock =
        initBlock(view, Metadata.MetadataType.FMD_CONTACT_TWITTER, R.id.block_twitter, R.drawable.ic_twitterx_white,
                  R.string.twitter, InputType.TYPE_TEXT_VARIATION_URI);
    View vkContactBlock = initBlock(view, Metadata.MetadataType.FMD_CONTACT_VK, R.id.block_vk, R.drawable.ic_vk_white,
                                    R.string.vk, InputType.TYPE_TEXT_VARIATION_URI);
    View lineContactBlock = initBlock(view, Metadata.MetadataType.FMD_CONTACT_LINE, R.id.block_line,
                                      R.drawable.ic_line_white, R.string.line, InputType.TYPE_TEXT_VARIATION_URI);
    View operatorBlock = initBlock(view, Metadata.MetadataType.FMD_OPERATOR, R.id.block_operator,
                                   R.drawable.ic_operator, R.string.editor_operator, 0);

    View blockCuisine = view.findViewById(R.id.block_cuisine);
    blockCuisine.setOnClickListener(this);
    mCuisine = view.findViewById(R.id.cuisine);

    View blockWifi = view.findViewById(R.id.block_wifi);
    mWifi = view.findViewById(R.id.sw__wifi);
    blockWifi.setOnClickListener(this);

    View blockSelfService = view.findViewById(R.id.block_self_service);
    blockSelfService.setOnClickListener(this);
    mSelfService = view.findViewById(R.id.self_service);

    View blockOutdoorSeating = view.findViewById(R.id.block_outdoor_seating);
    mOutdoorSeating = view.findViewById(R.id.sw__outdoor_seating);
    blockOutdoorSeating.setOnClickListener(this);

    View blockOpeningHours = view.findViewById(R.id.block_opening_hours);
    mEditOpeningHours = blockOpeningHours.findViewById(R.id.edit_opening_hours);
    mEditOpeningHours.setOnClickListener(this);
    mEmptyOpeningHours = blockOpeningHours.findViewById(R.id.empty_opening_hours);
    mEmptyOpeningHours.setOnClickListener(this);
    mOpeningHours = blockOpeningHours.findViewById(R.id.opening_hours);
    mOpeningHours.setOnClickListener(this);

    final View cardMore = view.findViewById(R.id.cv__more);
    mDescription = findInput(cardMore);
    TextView osmInfo = view.findViewById(R.id.osm_info);
    osmInfo.setMovementMethod(LinkMovementMethod.getInstance());
    mReset = view.findViewById(R.id.reset);
    mReset.setOnClickListener(this);

    // Address
    mDetailsBlocks.put(Metadata.MetadataType.FMD_OPEN_HOURS, blockOpeningHours);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_PHONE_NUMBER, blockPhone);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_CUISINE, blockCuisine);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_INTERNET, blockWifi);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_SELF_SERVICE, blockSelfService);
    UiUtils.hide(blockOutdoorSeating); // Temporarily hide
    mDetailsBlocks.put(Metadata.MetadataType.FMD_WEBSITE, websiteBlock);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_WEBSITE_MENU, websiteMenuBlock);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_EMAIL, emailBlock);
    mDetailsBlocks.put(Metadata.MetadataType.FMD_OPERATOR, operatorBlock);

    // Social Media
    mSocialMediaBlocks.put(Metadata.MetadataType.FMD_CONTACT_FACEBOOK, facebookContactBlock);
    mSocialMediaBlocks.put(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM, instagramContactBlock);
    mSocialMediaBlocks.put(Metadata.MetadataType.FMD_CONTACT_TWITTER, twitterContactBlock);
    mSocialMediaBlocks.put(Metadata.MetadataType.FMD_CONTACT_VK, vkContactBlock);
    mSocialMediaBlocks.put(Metadata.MetadataType.FMD_CONTACT_LINE, lineContactBlock);
  }

  @Override
  public void onClick(View v)
  {
    final int id = v.getId();
    if (id == R.id.edit_opening_hours || id == R.id.empty_opening_hours || id == R.id.opening_hours)
        mParent.editTimetable();
    else if (id == R.id.phone || id == R.id.edit_phone)
        mParent.editPhone();
    else if (id == R.id.block_wifi)
        mWifi.toggle();
    else if (id == R.id.block_self_service)
        mParent.editSelfService();
    else if (id == R.id.cv__social_media)  
    {
        mSocialMediaExpanded = !mSocialMediaExpanded;

        for (var block : mSocialMediaBlocks.values())
        {
            if (mSocialMediaExpanded)
                UiUtils.show

