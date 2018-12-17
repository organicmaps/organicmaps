package com.mapswithme.maps.bookmarks;

import android.annotation.SuppressLint;
import android.annotation.TargetApi;
import android.content.Intent;
import android.net.http.SslError;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.SslErrorHandler;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

import com.mapswithme.maps.Framework;
import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.BaseWebViewMwmFragment;
import com.mapswithme.maps.auth.TargetFragmentCallback;
import com.mapswithme.maps.dialog.AlertDialog;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.purchase.FailedPurchaseChecker;
import com.mapswithme.maps.purchase.PurchaseController;
import com.mapswithme.maps.purchase.PurchaseFactory;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.HttpClient;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.statistics.Statistics;

import java.lang.ref.WeakReference;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

public class BookmarksCatalogFragment extends BaseWebViewMwmFragment
    implements TargetFragmentCallback
{
  public static final String EXTRA_BOOKMARKS_CATALOG_URL = "bookmarks_catalog_url";
  private static final String FAILED_PURCHASE_DIALOG_TAG = "failed_purchase_dialog_tag";

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
  private BookmarksDownloadFragmentDelegate mDelegate;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mDelegate = new BookmarksDownloadFragmentDelegate(this);
    mDelegate.onCreate(savedInstanceState);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mDelegate.onStart();
    mFailedPurchaseController.addCallback(mPurchaseChecker);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mDelegate.onStop();
    mFailedPurchaseController.removeCallback();
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mWebViewClient.clear();
    mFailedPurchaseController.destroy();
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mFailedPurchaseController = PurchaseFactory.createFailedBookmarkPurchaseController(getContext());
    mFailedPurchaseController.initialize(getActivity());
    mFailedPurchaseController.validateExistingPurchases();
    mPurchaseChecker = new FailedBookmarkPurchaseChecker();
    View root = inflater.inflate(R.layout.fragment_bookmarks_catalog, container, false);
    mWebView = root.findViewById(getWebViewResId());
    mRetryBtn = root.findViewById(R.id.retry_btn);
    mProgressView = root.findViewById(R.id.progress);
    initWebView(mWebView);
    mRetryBtn.setOnClickListener(v -> onRetryClick());
    return root;
  }

  private void onRetryClick()
  {
    mWebViewClient.retry();
    UiUtils.hide(mRetryBtn);
    UiUtils.show(mProgressView);
    mFailedPurchaseController.validateExistingPurchases();
  }

  @SuppressLint("SetJavaScriptEnabled")
  private void initWebView(@NonNull WebView webView)
  {
    mWebViewClient = new WebViewBookmarksCatalogClient(this);
    webView.setWebViewClient(mWebViewClient);
    final WebSettings webSettings = webView.getSettings();
    webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
    webSettings.setJavaScriptEnabled(true);
    webSettings.setUserAgentString(Framework.nativeGetUserAgent());
  }

  @NonNull
  private String getCatalogUrlOrThrow()
  {
    Bundle args = getArguments();
    String result = args != null ? args.getString(EXTRA_BOOKMARKS_CATALOG_URL) : null;

    if (result == null)
    {
      result = getActivity().getIntent().getStringExtra(EXTRA_BOOKMARKS_CATALOG_URL);
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

  private static class WebViewBookmarksCatalogClient extends WebViewClient
  {
    private final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
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

      return fragment.downloadBookmark(url);
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
      if (ConnectionState.isConnected())
      {
        LOGGER.e(TAG, "Failed to load catalog: " + mError + ", description: " + description);
        Statistics.INSTANCE.trackDownloadCatalogError(Statistics.ParamValue.UNKNOWN);
        return;
      }

      Statistics.INSTANCE.trackDownloadCatalogError(Statistics.ParamValue.NO_INTERNET);
      Toast.makeText(frag.getContext(), R.string.common_check_internet_connection_dialog_title,
                     Toast.LENGTH_SHORT).show();
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
      String token = Framework.nativeGetAccessToken();
      mWebView.loadUrl(getCatalogUrlOrThrow(), TextUtils.isEmpty(token) ? Collections.emptyMap()
                                                                        : makeHeaders(token));
      UserActionsLogger.logBookmarksCatalogShownEvent();
    }

    @Override
    public void onAuthorizationRequired()
    {
      mDelegate.authorize(() -> mFailedPurchaseController.validateExistingPurchases());
    }
  }

  @NonNull
  private static Map<String, String> makeHeaders(@NonNull String token)
  {
    Map<String, String> headers = new HashMap<>();
    headers.put(HttpClient.HEADER_AUTHORIZATION, HttpClient.HEADER_BEARER_PREFFIX + token);
    return headers;
  }
}
