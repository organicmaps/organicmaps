package com.mapswithme.maps.bookmarks;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.app.Activity;
import android.content.Intent;
import android.net.Uri;
import android.net.http.SslError;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.SslErrorHandler;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import com.android.billingclient.api.SkuDetails;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.PrivateVariables;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.AuthorizationListener;
import com.mapswithme.maps.auth.BaseWebViewMwmFragment;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.dialog.AlertDialogCallback;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.purchase.AbstractProductDetailsLoadingCallback;
import com.mapswithme.maps.purchase.BillingManager;
import com.mapswithme.maps.purchase.BookmarksAllSubscriptionActivity;
import com.mapswithme.maps.purchase.BookmarksSightsSubscriptionActivity;
import com.mapswithme.maps.purchase.FailedPurchaseChecker;
import com.mapswithme.maps.purchase.PlayStoreBillingCallback;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.maps.purchase.PurchaseFactory;
import com.mapswithme.maps.purchase.PurchaseUtils;
import com.mapswithme.maps.purchase.SubscriptionType;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.CrashlyticsUtils;
import com.mapswithme.util.HttpClient;
import com.mapswithme.util.KeyValue;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.io.UnsupportedEncodingException;
import java.lang.ref.WeakReference;
import java.net.URLEncoder;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class BookmarksCatalogFragment extends BaseWebViewMwmFragment
    implements TargetFragmentCallback, AlertDialogCallback
{
  public static final String EXTRA_BOOKMARKS_CATALOG_URL = "bookmarks_catalog_url";
  private static final String FAILED_PURCHASE_DIALOG_TAG = "failed_purchase_dialog_tag";
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
  private static final String TAG = BookmarksCatalogFragment.class.getSimpleName();
  @SuppressWarnings("NullableProblems")
  @NonNull
  private WebViewBookmarksCatalogClient mWebViewClient;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private WebView mWebView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mRetryBtn;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mProgressView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PurchaseController<FailedPurchaseChecker> mFailedPurchaseController;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private FailedPurchaseChecker mPurchaseChecker;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BillingManager<PlayStoreBillingCallback> mProductDetailsLoadingManager;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private PlayStoreBillingCallback mProductDetailsLoadingCallback;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private BookmarksDownloadFragmentDelegate mDelegate;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private AlertDialogCallback mInvalidSubsDialogCallback;
  @Nullable
  private String mProductDetailsBundle;

  @NonNull
  private final AuthorizationListener mAuthorizationListener = success -> {
    if (!success)
      return;

    loadCatalog(mProductDetailsBundle);
  };

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle extra = requireActivity().getIntent()
                                    .getBundleExtra(BookmarksCatalogActivity.EXTRA_ARGS);
    mDelegate = new BookmarksDownloadFragmentDelegate(this, extra, mAuthorizationListener);
    mDelegate.onCreate(savedInstanceState);
    mInvalidSubsDialogCallback = new InvalidSubscriptionAlertDialogCallback(this);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mDelegate.onStart();
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mDelegate.onResume();
  }

  @Override
  public void onPause()
  {
    super.onPause();
    mDelegate.onPause();
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mDelegate.onStop();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mDelegate.onDestroyView();
    mWebViewClient.clear();
    mFailedPurchaseController.removeCallback();
    mFailedPurchaseController.destroy();
    mProductDetailsLoadingManager.removeCallback(mProductDetailsLoadingCallback);
    mProductDetailsLoadingManager.destroy();
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    setHasOptionsMenu(true);

    View root = inflater.inflate(R.layout.fragment_bookmarks_catalog, container, false);
    mWebView = root.findViewById(getWebViewResId());
    mRetryBtn = root.findViewById(R.id.retry_btn);
    mProgressView = root.findViewById(R.id.progress);
    initWebView();

    mFailedPurchaseController = PurchaseFactory.createFailedBookmarkPurchaseController(requireContext());
    mFailedPurchaseController.initialize(requireActivity());
    mPurchaseChecker = new FailedBookmarkPurchaseChecker();
    mFailedPurchaseController.addCallback(mPurchaseChecker);
    mFailedPurchaseController.validateExistingPurchases();

    mProductDetailsLoadingManager = PurchaseFactory.createInAppBillingManager();
    mProductDetailsLoadingManager.initialize(requireActivity());
    mProductDetailsLoadingCallback = new ProductDetailsLoadingCallback();
    mProductDetailsLoadingManager.addCallback(mProductDetailsLoadingCallback);

    mRetryBtn.setOnClickListener(v -> onRetryClick());
    mDelegate.onCreateView(savedInstanceState);
    return root;
  }

  private void onRetryClick()
  {
    mWebViewClient.retry();
    UiUtils.hide(mRetryBtn, mWebView);
    UiUtils.show(mProgressView);
    mFailedPurchaseController.validateExistingPurchases();
  }

  @SuppressLint("SetJavaScriptEnabled")
  private void initWebView()
  {
    mWebViewClient = new WebViewBookmarksCatalogClient(this);
    mWebView.setWebViewClient(mWebViewClient);
    final WebSettings webSettings = mWebView.getSettings();
    webSettings.setJavaScriptEnabled(true);
    webSettings.setUserAgentString(Framework.nativeGetUserAgent());
    if (Utils.isLollipopOrLater())
      webSettings.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
  }

  @NonNull
  private String getCatalogUrlOrThrow()
  {
    Bundle args = getArguments();
    String result = args != null ? args.getString(EXTRA_BOOKMARKS_CATALOG_URL) : null;

    if (result == null)
    {
      result = requireActivity().getIntent().getStringExtra(EXTRA_BOOKMARKS_CATALOG_URL);
    }

    if (result == null)
      throw new IllegalArgumentException("Catalog url not found in bundle");
    return result;
  }

  private boolean downloadBookmark(@NonNull String url)
  {
   return mDelegate.downloadBookmark(url);
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    mDelegate.onSaveInstanceState(outState);
  }

  @Override
  public void onActivityResult(int requestCode, int resultCode, Intent data)
  {
    super.onActivityResult(requestCode, resultCode, data);
    mDelegate.onActivityResult(requestCode, resultCode, data);

    if (resultCode != Activity.RESULT_OK)
      return;


    if (requestCode == PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION)
    {
      mWebView.reload();
      showSubscriptionSuccessDialog(this);
      return;
    }

    if (requestCode == PurchaseUtils.REQ_CODE_PAY_BOOKMARK)
      mWebView.reload();
  }

  private static void showSubscriptionSuccessDialog(@NonNull Fragment fragment)
  {
    PurchaseUtils.showSubscriptionSuccessDialog(fragment,
                                                PurchaseUtils.DIALOG_TAG_BMK_SUBSCRIPTION_SUCCESS,
                                                PurchaseUtils.REQ_CODE_BMK_SUBS_SUCCESS_DIALOG);
  }

  @Override
  public void onCreateOptionsMenu(Menu menu, MenuInflater inflater)
  {
    inflater.inflate(R.menu.menu_bookmark_catalog, menu);
  }

  @Override
  public boolean onOptionsItemSelected(MenuItem item)
  {
    if (item.getItemId() == R.id.close)
      requireActivity().finish();

    return super.onOptionsItemSelected(item);
  }

  @Override
  public void onTargetFragmentResult(int resultCode, @Nullable Intent data)
  {
    mDelegate.onTargetFragmentResult(resultCode, data);
  }

  @Override
  public boolean isTargetAdded()
  {
    return mDelegate.isTargetAdded();
  }

  private void loadCatalog(@Nullable String productDetailsBundle)
  {
    String token = Framework.nativeGetAccessToken();
    final Map<String, String> headers = new HashMap<>();

    if (!TextUtils.isEmpty(token))
      headers.put(HttpClient.HEADER_AUTHORIZATION, HttpClient.HEADER_BEARER_PREFFIX + token);

    if (!TextUtils.isEmpty(productDetailsBundle))
      headers.put(HttpClient.HEADER_BUNDLE_TIERS, productDetailsBundle);

    if (ThemeUtils.isNightTheme(requireContext()))
      headers.put(HttpClient.HEADER_THEME_KEY, HttpClient.HEADER_THEME_DARK);

    for (KeyValue header : BookmarkManager.INSTANCE.getCatalogHeaders())
    {
      if (!TextUtils.isEmpty(header.getValue()))
        headers.put(header.getKey(), header.getValue());
    }

    String url = mWebView.getUrl();
    if (TextUtils.isEmpty(url))
      url = getCatalogUrlOrThrow();
    mWebView.loadUrl(url, headers);

    UserActionsLogger.logBookmarksCatalogShownEvent();
  }

  @Override
  public void onAlertDialogPositiveClick(int requestCode, int which)
  {
    switch (requestCode)
    {
      case PurchaseUtils.REQ_CODE_CHECK_INVALID_SUBS_DIALOG:
        mInvalidSubsDialogCallback.onAlertDialogPositiveClick(requestCode, which);
        break;
      case PurchaseUtils.REQ_CODE_BMK_SUBS_SUCCESS_DIALOG:
        onRetryClick();
        break;
    }
  }

  @Override
  public void onAlertDialogNegativeClick(int requestCode, int which)
  {
    if (requestCode == PurchaseUtils.REQ_CODE_CHECK_INVALID_SUBS_DIALOG)
    {
      mInvalidSubsDialogCallback.onAlertDialogNegativeClick(requestCode, which);
    }
  }

  @Override
  public void onAlertDialogCancel(int requestCode)
  {
    switch (requestCode)
    {
      case PurchaseUtils.REQ_CODE_CHECK_INVALID_SUBS_DIALOG:
        mInvalidSubsDialogCallback.onAlertDialogCancel(requestCode);
        break;
      case PurchaseUtils.REQ_CODE_BMK_SUBS_SUCCESS_DIALOG:
        onRetryClick();
        break;
    }
  }

  private static class WebViewBookmarksCatalogClient extends WebViewClient
  {
    private static final String SUBSCRIBE_PATH_SEGMENT = "subscribe";
    private static final String VIEW_ON_MAP_PATH_SEGMENT = "map";
    private final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.BILLING);
    private final String TAG = WebViewBookmarksCatalogClient.class.getSimpleName();

    @NonNull
    private final WeakReference<BookmarksCatalogFragment> mReference;

    @Nullable
    private Object mError;

    WebViewBookmarksCatalogClient(@NonNull BookmarksCatalogFragment frag)
    {
      mReference = new WeakReference<>(frag);
    }

    @Override
    public boolean shouldOverrideUrlLoading(WebView view, String url)
    {
      BookmarksCatalogFragment fragment = mReference.get();
      if (fragment == null)
        return false;

      if (fragment.downloadBookmark(url))
        return true;

      Uri uri = Uri.parse(url);
      List<String> pathSegments = uri.getPathSegments();
      for (String each : pathSegments)
      {
        if (TextUtils.equals(each, SUBSCRIBE_PATH_SEGMENT))
          return handleSubscriptionUri(uri);

        if (TextUtils.equals(each, VIEW_ON_MAP_PATH_SEGMENT))
          return handleViewOnMapUri(fragment, uri);
      }

      return false;
    }

    private boolean handleViewOnMapUri(@NonNull BookmarksCatalogFragment fragment, @NonNull Uri uri)
    {
      String serverId = uri.getQueryParameter(PurchaseUtils.SERVER_ID);
      if (TextUtils.isEmpty(serverId))
        return false;

      BookmarkCategory category;
      try
      {
        category = BookmarkManager.INSTANCE.getCategoryByServerId(serverId);
      }
      catch (Throwable e)
      {
        CrashlyticsUtils.INSTANCE.logException(e);
        return false;
      }

      Intent intent = new Intent().putExtra(BookmarksCatalogActivity.EXTRA_DOWNLOADED_CATEGORY,
                                            category);
      fragment.requireActivity().setResult(Activity.RESULT_OK, intent);
      fragment.requireActivity().finish();
      return true;
    }

    private boolean handleSubscriptionUri(@NonNull Uri uri)
    {
      String group = PurchaseUtils.getTargetBookmarkGroupFromUri(uri);
      openSubscriptionScreen(SubscriptionType.getTypeByBookmarksGroup(group));
      return true;
    }

    private void openSubscriptionScreen(@NonNull SubscriptionType type)
    {
      BookmarksCatalogFragment frag = mReference.get();
      if (frag == null || frag.getActivity() == null)
        return;

      if (type.equals(SubscriptionType.BOOKMARKS_ALL))
        BookmarksAllSubscriptionActivity.startForResult(frag,
                                                        PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION,
                                                        Statistics.ParamValue.WEBVIEW);
      else
        BookmarksSightsSubscriptionActivity.startForResult(frag,
                                                           PurchaseUtils.REQ_CODE_PAY_SUBSCRIPTION,
                                                           Statistics.ParamValue.WEBVIEW);
    }

    @Override
    public void onPageFinished(WebView view, String url)
    {
      super.onPageFinished(view, url);
      BookmarksCatalogFragment frag;
      if ((frag = mReference.get()) == null || mError != null)
      {
        return;
      }

      UiUtils.show(frag.mWebView);
      UiUtils.hide(frag.mProgressView, frag.mRetryBtn);
    }

    @Override
    public void onReceivedError(WebView view, WebResourceRequest request, WebResourceError error)
    {
      super.onReceivedError(view, request, error);
      String description = Utils.isMarshmallowOrLater() ? makeDescription(error) : null;
      handleError(error, description);
    }

    @TargetApi(Build.VERSION_CODES.M)
    @NonNull
    private static String makeDescription(@NonNull WebResourceError error)
    {
      return error.getErrorCode() + "  " + error.getDescription();
    }

    @Override
    public void onReceivedSslError(WebView view, SslErrorHandler handler, SslError error)
    {
      super.onReceivedSslError(view, handler, error);
      handleError(error);
    }

    private void handleError(@NonNull Object error)
    {
      handleError(error, null);
    }

    private void handleError(@NonNull Object error, @Nullable String description)
    {
      mError = error;
      BookmarksCatalogFragment frag;
      if ((frag = mReference.get()) == null)
        return;

      UiUtils.show(frag.mRetryBtn);
      UiUtils.hide(frag.mWebView, frag.mProgressView);
      if (ConnectionState.INSTANCE.isConnected())
      {
        LOGGER.e(TAG, "Failed to load catalog: " + mError + ", description: " + description);
        Statistics.INSTANCE.trackDownloadCatalogError(Statistics.ParamValue.UNKNOWN);
        return;
      }

      Statistics.INSTANCE.trackDownloadCatalogError(Statistics.ParamValue.NO_INTERNET);
      Utils.showSnackbar(frag.requireContext(), frag.getViewOrThrow(),
                         R.string.common_check_internet_connection_dialog_title);
    }

    private void retry()
    {
      mError = null;
    }

    public void clear()
    {
      mReference.clear();
    }
  }

  private class FailedBookmarkPurchaseChecker implements FailedPurchaseChecker
  {
    @Override
    public void onFailedPurchaseDetected(boolean isDetected)
    {
      if (isDetected)
      {
        UiUtils.hide(mProgressView);
        UiUtils.show(mRetryBtn);
        AlertDialog dialog = new AlertDialog.Builder()
            .setTitleId(R.string.bookmarks_convert_error_title)
            .setMessageId(R.string.failed_purchase_support_message)
            .setPositiveBtnId(R.string.ok)
            .build();
        dialog.show(BookmarksCatalogFragment.this, FAILED_PURCHASE_DIALOG_TAG);
        return;
      }

      UiUtils.show(mProgressView);
      UiUtils.hide(mRetryBtn);

      mProductDetailsLoadingManager.queryProductDetails(Arrays.asList(PrivateVariables.bookmarkInAppIds()));
    }

    @Override
    public void onAuthorizationRequired()
    {
      mDelegate.authorize(() -> {
        mFailedPurchaseController.validateExistingPurchases();
        loadCatalog(mProductDetailsBundle);
      });
    }

    @Override
    public void onStoreConnectionFailed()
    {
      LOGGER.e(TAG, "Failed to check failed bookmarks due play store connection failure");
      loadCatalog(null);
    }
  }

  private class ProductDetailsLoadingCallback extends AbstractProductDetailsLoadingCallback
  {
    @Override
    public void onProductDetailsLoaded(@NonNull List<SkuDetails> details)
    {
      if (details.isEmpty())
      {
        LOGGER.i(TAG, "Product details not found.");
        loadCatalog(null);
        return;
      }

      LOGGER.i(TAG, "Product details for web catalog loaded: " + details);
      mProductDetailsBundle = toDetailsBundle(details);
      loadCatalog(mProductDetailsBundle);
    }

    @Nullable
    private String toDetailsBundle(@NonNull List<SkuDetails> details)
    {
      String bundle = PurchaseUtils.toProductDetailsBundle(details);
      String encodedBundle = null;
      try
      {
        encodedBundle = URLEncoder.encode(bundle, "UTF-8");
      }
      catch (UnsupportedEncodingException e)
      {
        String msg = "Failed to encode details bundle '" + bundle + "': ";
        LOGGER.e(TAG, msg, e);
        CrashlyticsUtils.INSTANCE.logException(new RuntimeException(msg, e));
      }

      return encodedBundle;
    }

    @Override
    public void onProductDetailsFailure()
    {
      LOGGER.e(TAG, "Failed to load product details for web catalog");
      loadCatalog(null);
    }
  }
}
