package com.mapswithme.maps.ugc.routes;

import android.os.Bundle;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.FragmentActivity;
import androidx.core.app.ShareCompat;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseAuthFragment;
import com.mapswithme.maps.bookmarks.AuthBundleFactory;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.CatalogCustomProperty;
import com.mapswithme.maps.bookmarks.data.CatalogTagsGroup;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.util.sharing.TargetUtils;
import com.mapswithme.util.statistics.Statistics;

import java.util.List;
import java.util.Objects;

public class SendLinkPlaceholderFragment extends BaseAuthFragment implements BookmarkManager.BookmarksCatalogListener,
                                                                             AlertDialogCallback
{
  public static final String EXTRA_CATEGORY = "bookmarks_category";
  private static final String BODY_STRINGS_SEPARATOR = "\n\n";
  private static final String ERROR_EDITED_ON_WEB_DIALOG_REQ_TAG = "error_edited_on_web_dialog";
  private static final int REQ_CODE_ERROR_EDITED_ON_WEB_DIALOG = 105;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarkCategory mCategory;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle args = getArguments();
    if (args == null)
      throw new IllegalArgumentException("Please, setup arguments");

    mCategory = Objects.requireNonNull(args.getParcelable(EXTRA_CATEGORY));
  }

  @SuppressWarnings("NullableProblems")
  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_route_send_link, container, false);
    View closeBtn = root.findViewById(R.id.close_btn);
    View.OnClickListener finishClickListener = v -> getActivity().finish();
    closeBtn.setOnClickListener(finishClickListener);
    View cancelBtn = root.findViewById(R.id.cancel_btn);
    cancelBtn.setOnClickListener(finishClickListener);
    View sendMeLinkBtn = root.findViewById(R.id.send_me_link_btn);
    sendMeLinkBtn.setOnClickListener(v -> onSendMeLinkBtnClicked());
    return root;
  }

  private void onSendMeLinkBtnClicked()
  {
    if (mCategory.getAccessRules() == BookmarkCategory.AccessRules.ACCESS_RULES_LOCAL)
      requestUpload();
    else
      shareLink();
  }

  private void requestUpload()
  {
    showProgress();
    BookmarkManager.INSTANCE.uploadToCatalog(BookmarkCategory.AccessRules.ACCESS_RULES_AUTHOR_ONLY,
                                             mCategory);
  }

  private void shareLink()
  {
    shareLink(BookmarkManager.INSTANCE.getWebEditorUrl(mCategory.getServerId()), requireActivity());
    Statistics.INSTANCE.trackEvent(Statistics.EventName.BM_EDIT_ON_WEB_CLICK);
  }

  static void shareLink(@NonNull String url, @NonNull FragmentActivity activity)
  {
    String emailBody = activity.getString(R.string.edit_your_guide_email_body)
                       + BODY_STRINGS_SEPARATOR + url;

    ShareCompat.IntentBuilder.from(activity)
                             .setType(TargetUtils.TYPE_TEXT_PLAIN)
                             .setSubject(activity.getString(R.string.edit_guide_title))
                             .setText(emailBody)
                             .setChooserTitle(activity.getString(R.string.share))
                             .startChooser();
  }

  @Override
  public void onUploadFinished(@NonNull BookmarkManager.UploadResult uploadResult, @NonNull
      String description, long originCategoryId, long resultCategoryId)
  {
    hideProgress();

    if (uploadResult == BookmarkManager.UploadResult.UPLOAD_RESULT_SUCCESS)
      onUploadSucceeded();
    else if (uploadResult == BookmarkManager.UploadResult.UPLOAD_RESULT_AUTH_ERROR)
      authorize(AuthBundleFactory.exportBookmarks());
    else
      onUploadFailed();
  }

  private void onUploadFailed()
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.bookmarks_convert_error_title)
        .setMessageId(R.string.upload_error_toast)
        .setPositiveBtnId(R.string.try_again)
        .setNegativeBtnId(R.string.cancel)
        .setReqCode(REQ_CODE_ERROR_EDITED_ON_WEB_DIALOG)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .build();
    dialog.setTargetFragment(this, REQ_CODE_ERROR_EDITED_ON_WEB_DIALOG);
    dialog.show(this, ERROR_EDITED_ON_WEB_DIALOG_REQ_TAG);
  }

  private void onUploadSucceeded()
  {
    mCategory = BookmarkManager.INSTANCE.getAllCategoriesSnapshot().refresh(mCategory);
    shareLink();
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
    /* do noting by default */
  }

  @Override
  public void onImportFinished(@NonNull String serverId, long catId, boolean successful)
  {
    /* do noting by default */
  }

  @Override
  public void onTagsReceived(boolean successful, @NonNull List<CatalogTagsGroup> tagsGroups,
                             int tagsLimit)
  {
    /* do noting by default */
  }

  @Override
  public void onCustomPropertiesReceived(boolean successful, @NonNull List<CatalogCustomProperty> properties)
  {
    /* do noting by default */
  }

  @Override
  public void onUploadStarted(long originCategoryId)
  {
    /* do noting by default */
  }

  @Override
  public void onAuthorizationFinish(boolean success)
  {
    if (success)
      requestUpload();
  }

  @Override
  public void onAuthorizationStart()
  {
    /* do noting by default */
  }

  @Override
  public void onSocialAuthenticationCancel(int type)
  {
    /* do noting by default */
  }

  @Override
  public void onSocialAuthenticationError(int type, @Nullable String error)
  {
    /* do noting by default */
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    shareLink();
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    /* do noting by default */
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    /* do noting by default */
  }
}
