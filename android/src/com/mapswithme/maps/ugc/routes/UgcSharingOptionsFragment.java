package com.mapswithme.maps.ugc.routes;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.fragment.app.Fragment;
import android.text.Html;
import android.text.Spanned;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseToolbarAuthFragment;
import com.mapswithme.maps.base.FinishActivityToolbarController;
import com.mapswithme.maps.bookmarks.AuthBundleFactory;
import com.mapswithme.maps.bookmarks.data.AbstractCategoriesSnapshot;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogPropertyOptionAndKey;
import com.mapswithme.maps.bookmarks.data.CatalogTag;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.dialog.ConfirmationDialogFactory;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Objects;

public class UgcSharingOptionsFragment extends BaseToolbarAuthFragment implements BookmarkManager.BookmarksCatalogListener,
                                                                                  AlertDialogCallback
{
  public static final int REQ_CODE_CUSTOM_PROPERTIES = 101;
  private static final int REQ_CODE_NO_NETWORK_CONNECTION_DIALOG = 103;
  private static final int REQ_CODE_ERROR_COMMON = 106;
  private static final int REQ_CODE_ERROR_NOT_ENOUGH_BOOKMARKS = 107;
  private static final int REQ_CODE_UPLOAD_CONFIRMATION_DIALOG = 108;
  private static final int REQ_CODE_ERROR_HTML_FORMATTING_DIALOG = 109;
  private static final int REQ_CODE_UPDATE_CONFIRMATION_DIALOG = 110;

  private static final String BUNDLE_CURRENT_MODE = "current_mode";
  private static final String NO_NETWORK_CONNECTION_DIALOG_TAG = "no_network_connection_dialog";
  private static final String NOT_ENOUGH_BOOKMARKS_DIALOG_TAG = "not_enough_bookmarks_dialog";
  private static final String ERROR_COMMON_DIALOG_TAG = "error_common_dialog";
  private static final String UPLOAD_CONFIRMATION_DIALOG_TAG = "upload_confirmation_dialog";
  private static final String UPDATE_CONFIRMATION_DIALOG_TAG = "update_confirmation_dialog";
  private static final String ERROR_HTML_FORMATTING_DIALOG_TAG = "error_html_formatting_dialog";
  private static final int MIN_REQUIRED_CATEGORY_SIZE = 3;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mGetDirectLinkContainer;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkCategory mCategory;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPublishCategoryImage;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mGetDirectLinkImage;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mEditOnWebBtn;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPublishingCompletedStatusContainer;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mGetDirectLinkCompletedStatusContainer;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mUploadAndPublishText;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mGetDirectLinkText;

  @Nullable
  private BookmarkCategory.AccessRules mCurrentMode;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mUpdateGuideDirectLinkBtn;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mUpdateGuidePublicAccessBtn;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mDirectLinkCreatedText;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mDirectLinkDescriptionText;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mShareDirectLinkBtn;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = Objects.requireNonNull(getArguments());
    BookmarkCategory rawCategory = args.getParcelable(UgcRouteSharingOptionsActivity.EXTRA_BOOKMARK_CATEGORY);
    AbstractCategoriesSnapshot.Default snapshot = BookmarkManager.INSTANCE.getAllCategoriesSnapshot();
    mCategory = snapshot.refresh(Objects.requireNonNull(rawCategory));
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_routes_sharing_options, container, false);
    initViews(root);
    initClickListeners(root);
    mCurrentMode = getCurrentMode(savedInstanceState);
    toggleViews();
    return root;
  }

  private void initViews(View root)
  {
    mGetDirectLinkContainer = root.findViewById(R.id.get_direct_link_container);
    View publishCategoryContainer = root.findViewById(R.id.upload_and_publish_container);
    mPublishCategoryImage = publishCategoryContainer.findViewById(R.id.upload_and_publish_image);
    mGetDirectLinkImage = mGetDirectLinkContainer.findViewById(R.id.get_direct_link_image);
    mEditOnWebBtn = root.findViewById(R.id.edit_on_web_btn);
    mPublishingCompletedStatusContainer = root.findViewById(R.id.publishing_completed_status_container);
    mGetDirectLinkCompletedStatusContainer = root.findViewById(R.id.get_direct_link_completed_status_container);
    mUploadAndPublishText = root.findViewById(R.id.upload_and_publish_text);
    mGetDirectLinkText = root.findViewById(R.id.get_direct_link_text);
    mUpdateGuideDirectLinkBtn = root.findViewById(R.id.direct_link_update_btn);
    mUpdateGuidePublicAccessBtn = root.findViewById(R.id.upload_and_publish_update_btn);
    mDirectLinkCreatedText = root.findViewById(R.id.direct_link_created_text);
    mDirectLinkDescriptionText = root.findViewById(R.id.direct_link_description_text);
    mShareDirectLinkBtn = mGetDirectLinkCompletedStatusContainer.findViewById(R.id.share_direct_link_btn);
    mUpdateGuideDirectLinkBtn.setSelected(true);
    mUpdateGuidePublicAccessBtn.setSelected(true);

    TextView licenseAgreementText = root.findViewById(R.id.license_agreement_message);
    String src = getResources().getString(R.string.ugc_routes_user_agreement,
                                          Framework.nativeGetPrivacyPolicyLink());
    Spanned spanned = Html.fromHtml(src);
    licenseAgreementText.setMovementMethod(LinkMovementMethod.getInstance());
    licenseAgreementText.setText(spanned);
  }

  private void toggleViews()
  {
    boolean isPublished = mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC;
    UiUtils.hideIf(isPublished, mUploadAndPublishText);
    UiUtils.showIf(isPublished, mPublishingCompletedStatusContainer, mUpdateGuidePublicAccessBtn);
    mPublishCategoryImage.setSelected(!isPublished);

    boolean isLinkSuccessFormed = mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK;
    UiUtils.hideIf(isLinkSuccessFormed
                   || isPublished, mGetDirectLinkText);
    UiUtils.showIf(isLinkSuccessFormed
                   || isPublished, mGetDirectLinkCompletedStatusContainer);

    UiUtils.showIf(isLinkSuccessFormed, mUpdateGuideDirectLinkBtn);
    mGetDirectLinkImage.setSelected(!isLinkSuccessFormed && !isPublished);
    mGetDirectLinkCompletedStatusContainer.setEnabled(!isPublished);

    mDirectLinkCreatedText.setText(isPublished ? R.string.upload_and_publish_success
                                               : R.string.direct_link_success);
    mDirectLinkDescriptionText.setText(isPublished ? R.string.unable_get_direct_link_desc
                                                   : R.string.get_direct_link_desc);
    UiUtils.hideIf(isPublished, mShareDirectLinkBtn);
  }

  @Nullable
  private static BookmarkCategory.AccessRules getCurrentMode(@Nullable Bundle bundle)
  {
    return bundle != null && bundle.containsKey(BUNDLE_CURRENT_MODE)
                   ? BookmarkCategory.AccessRules.values()[bundle.getInt(BUNDLE_CURRENT_MODE)]
                   : null;
  }

  @NonNull
  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new FinishActivityToolbarController(root, requireActivity());
  }

  private void initClickListeners(@NonNull View root)
  {
    View getDirectLinkView = root.findViewById(R.id.get_direct_link_text);
    getDirectLinkView.setOnClickListener(directLinkListener -> onGetDirectLinkClicked());
    mUpdateGuideDirectLinkBtn.setOnClickListener(v -> onUpdateDirectLinkClicked());
    View uploadAndPublishView = root.findViewById(R.id.upload_and_publish_text);
    uploadAndPublishView.setOnClickListener(uploadListener -> onUploadAndPublishBtnClicked());
    mUpdateGuidePublicAccessBtn.setOnClickListener(v -> onUpdatePublicAccessClicked());
    mShareDirectLinkBtn.setOnClickListener(v -> onDirectLinkShared());
    View sharePublishedBtn = mPublishingCompletedStatusContainer.findViewById(R.id.share_published_category_btn);
    sharePublishedBtn.setOnClickListener(v -> onPublishedCategoryShared());
    View editOnWebBtn = root.findViewById(R.id.edit_on_web_btn);
    editOnWebBtn.setOnClickListener(v -> onEditOnWebClicked());
  }

  private void onUpdatePublicAccessClicked()
  {
    mCurrentMode = BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC;
    onUpdateClickedInternal();
  }

  private void onUpdateDirectLinkClicked()
  {
    mCurrentMode = BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK;
    onUpdateClickedInternal();
  }

  private void onUpdateClickedInternal()
  {
    showUpdateCategoryConfirmationDialog();
  }

  private void onEditOnWebClicked()
  {
    if (isNetworkConnectionAbsent())
    {
      showNoNetworkConnectionDialog();
      return;
    }

    Intent intent = new Intent(getContext(), SendLinkPlaceholderActivity.class)
        .putExtra(SendLinkPlaceholderFragment.EXTRA_CATEGORY, mCategory);
    startActivity(intent);
    Statistics.INSTANCE.trackSharingOptionsClick(Statistics.ParamValue.EDIT_ON_WEB);
  }

  private void onPublishedCategoryShared()
  {
    shareCategory(BookmarkManager.INSTANCE.getCatalogPublicLink(mCategory.getId()));
  }

  private void shareCategory(@NonNull String link)
  {
    Intent intent = new Intent(Intent.ACTION_SEND)
        .setType(TargetUtils.TYPE_TEXT_PLAIN)
        .putExtra(Intent.EXTRA_TEXT, getString(R.string.share_bookmarks_email_body_link, link));
    startActivity(Intent.createChooser(intent, getString(R.string.share)));
    Statistics.INSTANCE.trackSharingOptionsClick(Statistics.ParamValue.COPY_LINK);
  }

  private void onDirectLinkShared()
  {
    shareCategory(BookmarkManager.INSTANCE.getCatalogDeeplink(mCategory.getId()));
  }

  private void showNoNetworkConnectionDialog()
  {
    Fragment fragment = requireFragmentManager().findFragmentByTag(NO_NETWORK_CONNECTION_DIALOG_TAG);
    if (fragment != null)
      return;

    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.common_check_internet_connection_dialog_title)
        .setMessageId(R.string.common_check_internet_connection_dialog)
        .setPositiveBtnId(R.string.try_again)
        .setNegativeBtnId(R.string.cancel)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .setReqCode(REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
        .build();
    dialog.setTargetFragment(this, REQ_CODE_NO_NETWORK_CONNECTION_DIALOG);
    dialog.show(this, NO_NETWORK_CONNECTION_DIALOG_TAG);
    Statistics.INSTANCE.trackSharingOptionsError(Statistics.EventName.BM_SHARING_OPTIONS_ERROR,
                                        Statistics.NetworkErrorType.NO_NETWORK);
  }

  private boolean isNetworkConnectionAbsent()
  {
    return !ConnectionState.INSTANCE.isConnected();
  }

  private void openTagsScreen()
  {
    Intent intent = new Intent(getContext(), EditCategoryNameActivity.class);
    intent.putExtra(EditCategoryNameFragment.BUNDLE_BOOKMARK_CATEGORY, mCategory);
    startActivityForResult(intent, REQ_CODE_CUSTOM_PROPERTIES);
  }

  private void onUploadAndPublishBtnClicked()
  {
    if (mCategory.size() < MIN_REQUIRED_CATEGORY_SIZE)
    {
      showNotEnoughBookmarksDialog();
      return;
    }

    mCurrentMode = BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC;
    onUploadBtnClicked();
    Statistics.INSTANCE.trackSharingOptionsClick(Statistics.ParamValue.PUBLIC);
  }

  private void onGetDirectLinkClicked()
  {
    if (isNetworkConnectionAbsent())
    {
      showNoNetworkConnectionDialog();
      return;
    }
    mCurrentMode = BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK;
    requestUpload();
    Statistics.INSTANCE.trackSharingOptionsClick(Statistics.ParamValue.PRIVATE);
  }

  private void onUploadBtnClicked()
  {
    if (isNetworkConnectionAbsent())
    {
      showNoNetworkConnectionDialog();
      return;
    }

    showUploadCatalogConfirmationDialog();
  }

  private void requestUpload()
  {
    if (isAuthorized())
      onPostAuthCompleted();
    else
      authorize(AuthBundleFactory.exportBookmarks());
  }

  private void onPostAuthCompleted()
  {
    if (isDirectLinkUploadMode())
      requestDirectLink();
    else if (isPublishRefreshManual())
      requestPublishingImmediately();
    else
      openTagsScreen();
  }

  private boolean isPublishRefreshManual()
  {
    return mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC;
  }

  private void requestPublishingImmediately()
  {
    showProgress();
    BookmarkManager.INSTANCE.uploadToCatalog(BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC,
                                             mCategory);
  }

  private boolean isDirectLinkUploadMode()
  {
    return mCurrentMode == BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK;
  }

  private void requestDirectLink()
  {
    if (mCurrentMode == null)
      throw new IllegalStateException("CurrentMode must be initialized");
    showProgress();
    BookmarkManager.INSTANCE.uploadRoutes(mCurrentMode.ordinal(), mCategory);
  }

  @Override
  public void onSaveInstanceState(@NonNull Bundle outState)
  {
    super.onSaveInstanceState(outState);
    if (mCurrentMode != null)
      outState.putInt(BUNDLE_CURRENT_MODE, mCurrentMode.ordinal());
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    if (requestCode == REQ_CODE_CUSTOM_PROPERTIES && resultCode == Activity.RESULT_OK)
      requestPublishing(data);
  }

  private void requestPublishing(@NonNull Intent data)
  {
    showProgress();
    Bundle tagsActivityResult = data.getParcelableExtra(UgcRoutePropertiesFragment.EXTRA_TAGS_ACTIVITY_RESULT);

    List<CatalogTag> tags = tagsActivityResult.getParcelableArrayList(UgcRouteTagsActivity.EXTRA_TAGS);
    List<CatalogPropertyOptionAndKey> options = data.getParcelableArrayListExtra(UgcRoutePropertiesFragment.EXTRA_CATEGORY_OPTIONS);
    BookmarkManager.INSTANCE.setCategoryTags(mCategory, Objects.requireNonNull(tags));
    BookmarkManager.INSTANCE.setCategoryProperties(mCategory, options);
    BookmarkManager.INSTANCE.uploadToCatalog(BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC, mCategory);
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
  public void onAuthorizationFinish(boolean success)
  {
    if (success)
      onPostAuthCompleted();
    else
      onPostAuthFailed();
  }

  private void onPostAuthFailed()
  {
    Statistics.INSTANCE.trackSharingOptionsError(Statistics.EventName.BM_SHARING_OPTIONS_ERROR,
                                                 Statistics.NetworkErrorType.AUTH_FAILED);
  }

  @Override
  public void onAuthorizationStart()
  {

  }

  @Override
  public void onSocialAuthenticationCancel(int type)
  {

  }

  @Override
  public void onSocialAuthenticationError(int type, @Nullable String error)
  {
  }

  @Override
  public void onImportStarted(@NonNull String serverId)
  {

  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {

  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups,
                             int tagsLimit)
  {

  }

  @Override
  public void onCustomPropertiesReceived(boolean successful,
                                         @NonNull List<CatalogCustomProperty> properties)
  {

  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {

  }

  @Override
  public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult,
                               @NonNull String description, long originCategoryId,
                               long resultCategoryId)
  {
    hideProgress();
    if (isOkResult(uploadResult))
      onUploadSuccess();
    else
      onUploadError(uploadResult);
  }

  private void onUploadError(@NonNull BookmarkManager.UploadResult uploadResult)
  {
    Statistics.INSTANCE.trackSharingOptionsError(Statistics.EventName.BM_SHARING_OPTIONS_UPLOAD_ERROR,
                                                 uploadResult.ordinal());
    if (uploadResult == BookmarkManager.UploadResult.UPLOAD_RESULT_MALFORMED_DATA_ERROR)
    {
      showHtmlFormattingError();
      return;
    }

    if (uploadResult == BookmarkManager.UploadResult.UPLOAD_RESULT_ACCESS_ERROR)
    {
      showUnresolvedConflictsErrorDialog();
      return;
    }

    showCommonErrorDialog();
  }

  private void showCommonErrorDialog()
  {
    showUploadErrorDialog(R.string.upload_error_toast,
                          REQ_CODE_ERROR_COMMON,
                          ERROR_COMMON_DIALOG_TAG);
  }

  private void onUploadSuccess()
  {
    boolean isRefreshManual = mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC
                        || mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK;
    mCategory = BookmarkManager.INSTANCE.getAllCategoriesSnapshot().refresh(mCategory);
    checkSuccessUploadedCategoryAccessRules();

    boolean isDirectLinkMode = mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK;
    int successMsgResId = isRefreshManual ? R.string.direct_link_updating_success
                                          : isDirectLinkMode ? R.string.direct_link_success
                                                             : R.string.upload_and_publish_success;
    Utils.showSnackbar(requireContext(), getViewOrThrow(), successMsgResId);
    toggleViews();
    Statistics.INSTANCE.trackSharingOptionsUploadSuccess(mCategory);
  }

  private void checkSuccessUploadedCategoryAccessRules()
  {
    if (mCategory.getAccessRules() != BookmarkCategory.AccessRules.ACCESS_RULES_PUBLIC
        && mCategory.getAccessRules() != BookmarkCategory.AccessRules.ACCESS_RULES_DIRECT_LINK)
      throw new IllegalStateException("Access rules must be ACCESS_RULES_PUBLIC or ACCESS_RULES_DIRECT_LINK." +
                                      " Current value = " + mCategory.getAccessRules());
  }

  private void showUploadErrorDialog(@StringRes int subtitle, int reqCode, @NonNull String tag)
  {
    showErrorDialog(R.string.unable_upadate_error_title, subtitle, reqCode, tag);
  }

  private void showNotEnoughBookmarksDialog()
  {
    showErrorDialog(R.string.error_public_not_enought_title,
                    R.string.error_public_not_enought_subtitle,
                    REQ_CODE_ERROR_NOT_ENOUGH_BOOKMARKS, NOT_ENOUGH_BOOKMARKS_DIALOG_TAG);
  }

  private void showErrorDialog(@StringRes int title, @StringRes int subtitle, int reqCode,
                               @NonNull String tag)
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(title)
        .setMessageId(subtitle)
        .setPositiveBtnId(R.string.ok)
        .setReqCode(reqCode)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .build();
    dialog.setTargetFragment(this, reqCode);
    dialog.show(this, tag);
  }

  private boolean isOkResult(@NonNull BookmarkManager.UploadResult uploadResult)
  {
    return uploadResult == BookmarkManager.UploadResult.UPLOAD_RESULT_SUCCESS;
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    if (requestCode == REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
      Utils.showSystemConnectionSettings(requireContext());
    else if (requestCode == REQ_CODE_UPLOAD_CONFIRMATION_DIALOG
             || requestCode == REQ_CODE_UPDATE_CONFIRMATION_DIALOG)
      requestUpload();
    else if (requestCode == REQ_CODE_ERROR_HTML_FORMATTING_DIALOG)
      SendLinkPlaceholderFragment.shareLink(BookmarkManager.INSTANCE.getWebEditorUrl(mCategory.getServerId()),
                                            requireActivity());
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {

  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {

  }

  private void showConfirmationDialog(@StringRes int title, @StringRes int description,
                                      @StringRes int acceptBtn,
                                      @StringRes int declineBtn,
                                      String tag, int reqCode)
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(title)
        .setMessageId(description)
        .setPositiveBtnId(acceptBtn)
        .setNegativeBtnId(declineBtn)
        .setReqCode(reqCode)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .setDialogViewStrategyType(AlertDialog.DialogViewStrategyType.CONFIRMATION_DIALOG)
        .setDialogFactory(new ConfirmationDialogFactory())
        .build();
    dialog.setTargetFragment(this, reqCode);
    dialog.show(this, tag);
  }

  private void showHtmlFormattingError()
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.html_format_error_title)
        .setMessageId(R.string.html_format_error_subtitle)
        .setPositiveBtnId(R.string.edit_on_web)
        .setNegativeBtnId(R.string.cancel)
        .setReqCode(REQ_CODE_ERROR_HTML_FORMATTING_DIALOG)
        .setImageResId(R.drawable.ic_error_red)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .setDialogViewStrategyType(AlertDialog.DialogViewStrategyType.CONFIRMATION_DIALOG)
        .setDialogFactory(new ConfirmationDialogFactory())
        .build();
    dialog.setTargetFragment(this, REQ_CODE_ERROR_HTML_FORMATTING_DIALOG);
    dialog.show(this, ERROR_HTML_FORMATTING_DIALOG_TAG);
  }

  private void showUploadCatalogConfirmationDialog()
  {
    showConfirmationDialog(R.string.bookmark_public_upload_alert_title,
                           R.string.bookmark_public_upload_alert_subtitle,
                           R.string.bookmark_public_upload_alert_ok_button,
                           R.string.cancel,
                           UPLOAD_CONFIRMATION_DIALOG_TAG, REQ_CODE_UPLOAD_CONFIRMATION_DIALOG
                          );
  }

  private void showUpdateCategoryConfirmationDialog()
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.any_access_update_alert_title)
        .setMessageId(R.string.any_access_update_alert_message)
        .setPositiveBtnId(R.string.any_access_update_alert_update)
        .setNegativeBtnId(R.string.cancel)
        .setReqCode(REQ_CODE_UPDATE_CONFIRMATION_DIALOG)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .build();
    dialog.setTargetFragment(this, REQ_CODE_UPDATE_CONFIRMATION_DIALOG);
    dialog.show(this, UPDATE_CONFIRMATION_DIALOG_TAG);
  }

  private void showUnresolvedConflictsErrorDialog()
  {
    showConfirmationDialog(R.string.public_or_limited_access_after_edit_online_error_title,
                           R.string.public_or_limited_access_after_edit_online_error_message,
                           R.string.edit_on_web,
                           R.string.cancel,
                           UPLOAD_CONFIRMATION_DIALOG_TAG, REQ_CODE_UPLOAD_CONFIRMATION_DIALOG
                          );
  }
}
