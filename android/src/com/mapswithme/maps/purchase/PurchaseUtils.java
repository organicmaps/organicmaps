package com.mapswithme.maps.purchase;

import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;

import com.mapswithme.maps.R;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.ConfirmationDialogFactory;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.List;

public class PurchaseUtils
{
  public static final String GROUPS = "groups";
  public static final String SERVER_ID = "server_id";
  static final int REQ_CODE_PRODUCT_DETAILS_FAILURE = 1;
  static final int REQ_CODE_PAYMENT_FAILURE = 2;
  static final int REQ_CODE_VALIDATION_SERVER_ERROR = 3;
  static final int REQ_CODE_START_TRANSACTION_FAILURE = 4;
  static final int REQ_CODE_PING_FAILURE = 5;
  public static final int REQ_CODE_CHECK_INVALID_SUBS_DIALOG = 6;
  public static final int REQ_CODE_BMK_SUBS_SUCCESS_DIALOG = 7;
  public static final int REQ_CODE_PAY_CONTINUE_SUBSCRIPTION = 8;
  public static final int REQ_CODE_PAY_BOOKMARK = 9;
  public static final int REQ_CODE_PAY_SUBSCRIPTION = 10;
  public static final String DIALOG_TAG_CHECK_INVALID_SUBS = "check_invalid_subs";
  public static final String DIALOG_TAG_BMK_SUBSCRIPTION_SUCCESS = "bmk_subscription_success";
  public static final String EXTRA_IS_SUBSCRIPTION = "extra_is_subscription";

  final static int WEEKS_IN_YEAR = 52;
  final static int MONTHS_IN_YEAR = 12;
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = PurchaseUtils.class.getSimpleName();
  static final int REQ_CODE_NO_NETWORK_CONNECTION_DIALOG = 11;
  static final String NO_NETWORK_CONNECTION_DIALOG_TAG = "no_network_connection_dialog_tag";

  private PurchaseUtils()
  {
    // Utility class.
  }


  static void showPaymentFailureDialog(@NonNull Fragment fragment, @Nullable String tag)
  {
    AlertDialog alertDialog = new AlertDialog.Builder()
        .setReqCode(PurchaseUtils.REQ_CODE_PAYMENT_FAILURE)
        .setTitleId(R.string.bookmarks_convert_error_title)
        .setMessageId(R.string.purchase_error_subtitle)
        .setPositiveBtnId(R.string.back)
        .build();
    alertDialog.show(fragment, tag);
  }

  static void showProductDetailsFailureDialog(@NonNull Fragment fragment, @NonNull String tag)
  {
    AlertDialog alertDialog = new AlertDialog.Builder()
        .setReqCode(PurchaseUtils.REQ_CODE_PRODUCT_DETAILS_FAILURE)
        .setTitleId(R.string.bookmarks_convert_error_title)
        .setMessageId(R.string.discovery_button_other_error_message)
        .setPositiveBtnId(R.string.ok)
        .build();
    alertDialog.show(fragment, tag);
  }

  static void showPingFailureDialog(@NonNull Fragment fragment)
  {
    AlertDialog alertDialog = new AlertDialog.Builder()
        .setReqCode(PurchaseUtils.REQ_CODE_PING_FAILURE)
        .setTitleId(R.string.subscription_error_ping_title)
        .setMessageId(R.string.subscription_error_message)
        .setPositiveBtnId(R.string.ok)
        .build();
    alertDialog.show(fragment, null);
  }

  static void showNoConnectionDialog(@NonNull Fragment fragment)
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.common_check_internet_connection_dialog_title)
        .setMessageId(R.string.common_check_internet_connection_dialog)
        .setPositiveBtnId(R.string.try_again)
        .setNegativeBtnId(R.string.cancel)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .setReqCode(REQ_CODE_NO_NETWORK_CONNECTION_DIALOG)
        .build();
    dialog.setTargetFragment(fragment, REQ_CODE_NO_NETWORK_CONNECTION_DIALOG);
    dialog.show(fragment, NO_NETWORK_CONNECTION_DIALOG_TAG);
  }

  @NonNull
  public static String getTargetBookmarkGroupFromUri(@NonNull Uri uri)
  {
    List<String> uriGroups = uri.getQueryParameters(GROUPS);
    if (uriGroups == null || uriGroups.isEmpty())
    {
      CrashlyticsUtils.INSTANCE.logException(
          new IllegalArgumentException("'" + GROUPS
                                       + "' parameter is required! URI: " + uri));
      return SubscriptionType.BOOKMARKS_ALL.getServerId();
    }


    List<String> priorityGroups = Arrays.asList(SubscriptionType.BOOKMARKS_ALL.getServerId(),
                                                SubscriptionType.BOOKMARKS_SIGHTS.getServerId());
    for (String priorityGroup : priorityGroups)
    {
      for (String uriGroup : uriGroups)
      {
        if (priorityGroup.equals(uriGroup))
        {
          return priorityGroup;
        }
      }
    }

    return SubscriptionType.BOOKMARKS_ALL.getServerId();
  }

  public static void showSubscriptionSuccessDialog(@NonNull Fragment targetFragment,
                                                   @NonNull String tag, int reqCode)
  {
    AlertDialog dialog = new AlertDialog.Builder()
        .setTitleId(R.string.subscription_success_dialog_title)
        .setMessageId(R.string.subscription_success_dialog_message)
        .setPositiveBtnId(R.string.subscription_error_button)
        .setReqCode(reqCode)
        .setFragManagerStrategyType(AlertDialog.FragManagerStrategyType.ACTIVITY_FRAGMENT_MANAGER)
        .setDialogViewStrategyType(AlertDialog.DialogViewStrategyType.CONFIRMATION_DIALOG)
        .setDialogFactory(new ConfirmationDialogFactory())
        .build();
    dialog.setTargetFragment(targetFragment, reqCode);
    dialog.show(targetFragment, tag);
  }

  enum Period
  {
    // Order is important.
    P1Y,
    P1M,
    P1W;

    @Nullable
    static Period getInstance(@Nullable String subscriptionPeriod)
    {
      for (Period each : values())
      {
        if (TextUtils.equals(each.name(), subscriptionPeriod))
          return each;
      }
      return null;
    }
  }
}
