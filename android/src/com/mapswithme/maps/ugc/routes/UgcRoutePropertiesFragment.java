package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogCustomPropertyOption;
import com.mapswithme.maps.bookmarks.data.CatalogPropertyOptionAndKey;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.util.UiUtils;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

public class UgcRoutePropertiesFragment extends BaseMwmFragment implements BookmarkManager.BookmarksCatalogListener, AlertDialogCallback
{
  public static final String EXTRA_TAGS_ACTIVITY_RESULT = "tags_activity_result";
  public static final String EXTRA_CATEGORY_OPTIONS = "category_options";
  public static final int REQ_CODE_TAGS_ACTIVITY = 102;
  private static final int REQ_CODE_LOAD_FAILED = 101;
  private static final int FIRST_OPTION_INDEX = 0;
  private static final int SECOND_OPTION_INDEX = 1;
  private static final String BUNDLE_SELECTED_OPTION = "selected_property";
  private static final String BUNDLE_CUSTOM_PROPS = "custom_props";
  private static final String ERROR_LOADING_DIALOG_TAG = "error_loading_dialog";

  @NonNull
  private List<CatalogCustomProperty> mProps = Collections.emptyList();

  @Nullable
  private CatalogPropertyOptionAndKey mSelectedOption;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mProgress;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPropsContainer;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private Button mLeftBtn;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private Button mRightBtn;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_routes_properties, container, false);
    initPropsAndOptions(savedInstanceState);
    initViews(root);
    if (mProps.isEmpty())
      BookmarkManager.INSTANCE.requestCustomProperties();
    return root;
  }

  private void initPropsAndOptions(@Nullable Bundle savedInstanceState)
  {
    if (savedInstanceState == null)
      return;
    mProps =  Objects.requireNonNull(savedInstanceState.getParcelableArrayList(BUNDLE_CUSTOM_PROPS));
    mSelectedOption = savedInstanceState.getParcelable(BUNDLE_SELECTED_OPTION);
  }

  private void initViews(@NonNull View root)
  {
    mLeftBtn = root.findViewById(R.id.left_btn);
    mLeftBtn.setOnClickListener(v -> onLeftBtnClicked());
    mRightBtn = root.findViewById(R.id.right_btn);
    mRightBtn.setOnClickListener(v -> onRightBtnClicked());
    mPropsContainer = root.findViewById(R.id.properties_container);
    UiUtils.hideIf(mProps.isEmpty(), mPropsContainer);
    mProgress = root.findViewById(R.id.progress);
    UiUtils.showIf(mProps.isEmpty(), mProgress);
    if (mProps.isEmpty())
      return;

    initButtons();
  }

  private void initButtons()
  {
    CatalogCustomProperty property = mProps.get(0);
    CatalogCustomPropertyOption firstOption = property.getOptions().get(FIRST_OPTION_INDEX);
    CatalogCustomPropertyOption secondOption = property.getOptions().get(SECOND_OPTION_INDEX);
    mLeftBtn.setText(firstOption.getLocalizedName());
    mRightBtn.setText(secondOption.getLocalizedName());
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    if (mSelectedOption != null)
      outState.putParcelable(BUNDLE_SELECTED_OPTION, mSelectedOption);
    outState.putParcelableArrayList(BUNDLE_CUSTOM_PROPS, new ArrayList<>(mProps));
  }

  private void onBtnClicked(int index)
  {
    CatalogCustomProperty property = mProps.get(0);
    CatalogCustomPropertyOption option = property.getOptions().get(index);
    mSelectedOption = new CatalogPropertyOptionAndKey(property.getKey(), option);
    Intent intent = new Intent(getContext(), UgcRouteTagsActivity.class);
    startActivityForResult(intent, REQ_CODE_TAGS_ACTIVITY);
  }

  private void onRightBtnClicked()
  {
    onBtnClicked(SECOND_OPTION_INDEX);
  }

  private void onLeftBtnClicked()
  {
    onBtnClicked(FIRST_OPTION_INDEX);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    BookmarkManager.INSTANCE.addCatalogListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    BookmarkManager.INSTANCE.removeCatalogListener(this);
  }

  @Override
  public void onImportStarted(@NonNull String serverId)
  {
    /* Do noting by default */
  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {
    /* Do noting by default */
  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups)
  {
    /* Do noting by default */
  }

  @Override
  public void onCustomPropertiesReceived(boolean successful,
                                         @NonNull List<CatalogCustomProperty> properties)
  {
    if (!successful)
    {
      onLoadFailed();
      return;
    }

    if (properties.isEmpty())
    {
      onLoadFailed();
      return;
    }

    CatalogCustomProperty property = properties.iterator().next();
    if (property == null)
    {
      onLoadFailed();
      return;
    }

    List<CatalogCustomPropertyOption> options = property.getOptions();
    if (options.size() <= SECOND_OPTION_INDEX)
    {
      onLoadFailed();
      return;
    }

    onLoadSuccess(properties);
  }

  private void onLoadFailed()
  {
    showLoadFailedDialog();
    UiUtils.hide(mProgress, mPropsContainer);
  }

  private void showLoadFailedDialog()
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.discovery_button_viator_error_title)
        .setMessageId(R.string.properties_loading_error_subtitle)
        .setPositiveBtnId(R.string.try_again)
        .setNegativeBtnId(R.string.cancel)
        .setReqCode(REQ_CODE_LOAD_FAILED)
        .setFragManagerStrategy(new AlertDialog.ActivityFragmentManagerStrategy())
        .build();
    dialog.setTargetFragment(this, REQ_CODE_LOAD_FAILED);
    dialog.show(this, ERROR_LOADING_DIALOG_TAG);
  }

  private void onLoadSuccess(@NonNull List<CatalogCustomProperty> properties)
  {
    mProps = properties;
    mPropsContainer.setVisibility(View.VISIBLE);
    mProgress.setVisibility(View.GONE);
    initButtons();
  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {
    /* Do noting by default */
  }

  @Override
  public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult,
                               @NonNull String description, long originCategoryId,
                               long resultCategoryId)
  {
    /* Do noting by default */
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (requestCode == REQ_CODE_TAGS_ACTIVITY)
    {
      if (resultCode == Activity.RESULT_OK)
      {
        Intent intent = new Intent();
        ArrayList<CatalogPropertyOptionAndKey> options = new ArrayList<>(prepareSelectedOptions());
        intent.putParcelableArrayListExtra(EXTRA_CATEGORY_OPTIONS, options);
        intent.putExtra(EXTRA_TAGS_ACTIVITY_RESULT, data.getExtras());
        getActivity().setResult(Activity.RESULT_OK, intent);
        getActivity().finish();
      }
      else if (resultCode == Activity.RESULT_CANCELED)
      {
        getActivity().setResult(Activity.RESULT_CANCELED);
        getActivity().finish();
      }
    }
  }

  @NonNull
  private List<CatalogPropertyOptionAndKey> prepareSelectedOptions()
  {
    return mSelectedOption == null ? Collections.emptyList()
                                   : Collections.singletonList(mSelectedOption);
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    UiUtils.show(mProgress);
    UiUtils.hide(mPropsContainer);
    BookmarkManager.INSTANCE.requestCustomProperties();
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    getActivity().setResult(Activity.RESULT_CANCELED);
    getActivity().finish();
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    getActivity().setResult(Activity.RESULT_CANCELED);
    getActivity().finish();
  }
}
