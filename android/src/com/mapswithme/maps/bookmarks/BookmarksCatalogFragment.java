package com.mapswithme.maps.bookmarks;

import android.annotation.SuppressLint;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebResourceError;
import android.webkit.WebResourceRequest;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Toast;

import com.mapswithme.maps.R;
import com.mapswithme.maps.auth.BaseWebViewMwmFragment;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.util.DialogUtils;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.UiUtils;

import java.lang.ref.WeakReference;

public class BookmarksCatalogFragment extends BaseWebViewMwmFragment
{
  public static final String EXTRA_BOOKMARKS_CATALOG_URL = "bookmarks_catalog_url";

  @SuppressWarnings("NullableProblems")
  @NonNull
  private String mCatalogUrl;

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
  private ImportCategoryListener mImportCategoryListener;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mCatalogUrl = getCatalogUrlOrThrow();
    mImportCategoryListener = new ImportCategoryListener(this);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mImportCategoryListener.attach(this);
    BookmarkManager.INSTANCE.addCatalogListener(mImportCategoryListener);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mImportCategoryListener.detach();
    BookmarkManager.INSTANCE.removeCatalogListener(mImportCategoryListener);
  }

  @Override
  public void onDestroyView()
  {
    super.onDestroyView();
    mWebViewClient.clear();
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_bookmarks_catalog, container, false);
    mWebView = root.findViewById(getWebViewResId());
    mRetryBtn = root.findViewById(R.id.retry_btn);
    mProgressView = root.findViewById(R.id.progress);
    initWebView(mWebView);
    mRetryBtn.setOnClickListener(v -> onRetryClick());
    mWebView.loadUrl(mCatalogUrl);
    return root;
  }

  private void onRetryClick()
  {
    mWebViewClient.retry();
    mRetryBtn.setVisibility(View.GONE);
    mProgressView.setVisibility(View.VISIBLE);
    mWebView.loadUrl(mCatalogUrl);
  }

  @SuppressLint("SetJavaScriptEnabled")
  private void initWebView(@NonNull WebView webView)
  {
    mWebViewClient = new WebViewBookmarksCatalogClient(this);
    webView.setWebViewClient(mWebViewClient);
    final WebSettings webSettings = webView.getSettings();
    webSettings.setCacheMode(WebSettings.LOAD_NO_CACHE);
    webSettings.setJavaScriptEnabled(true);
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

  private static class WebViewBookmarksCatalogClient extends WebViewClient
  {
    @NonNull
    private final WeakReference<BookmarksCatalogFragment> mReference;

    @Nullable
    private WebResourceError mError;

    WebViewBookmarksCatalogClient(@NonNull BookmarksCatalogFragment frag)
    {
      mReference = new WeakReference<>(frag);
    }

    @Override
    public boolean shouldOverrideUrlLoading(WebView view, String url)
    {
      try
      {
        return requestArchive(view, url);
      }
      catch (BookmarksDownloadManager.UnprocessedUrlException e)
      {
        return false;
      }
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
      mError = error;
      BookmarksCatalogFragment frag;
      if ((frag = mReference.get()) == null)
        return;

      UiUtils.show(frag.mRetryBtn);
      UiUtils.hide(frag.mWebView, frag.mProgressView);
      if (ConnectionState.isConnected())
        return;
      Toast.makeText(frag.getContext(), R.string.common_check_internet_connection_dialog_title, Toast.LENGTH_SHORT)
           .show();
    }

    private void retry()
    {
      mError = null;
    }

    private boolean requestArchive(@NonNull WebView view,
                                   @NonNull String url) throws BookmarksDownloadManager.UnprocessedUrlException
    {
      BookmarksDownloadManager dm = BookmarksDownloadManager.from(view.getContext());
      dm.enqueueRequest(url);
      return true;
    }

    public void clear()
    {
      mReference.clear();
    }
  }

  private static class ImportCategoryListener extends BookmarkManager.DefaultBookmarksCatalogListener
  {
    @Nullable
    private BookmarksCatalogFragment mFragment;

    ImportCategoryListener(@NonNull BookmarksCatalogFragment fragment)
    {
      mFragment = fragment;
    }

    @Override
    public void onImportFinished(@NonNull String serverId, boolean successful)
    {
      if (mFragment == null)
      {
        return;
      }

      if (successful)
      {
        Toast.makeText(mFragment.getContext(), R.string.bookmarks_webview_success_toast,
                       Toast.LENGTH_SHORT).show();
        return;
      }
      DialogUtils.showAlertDialog(mFragment.getActivity(),
                                  R.string.title_error_downloading_bookmarks,
                                  R.string.subtitle_error_downloading_bookmarks);
    }

    public void detach()
    {
      mFragment = null;
    }

    public void attach(@NonNull BookmarksCatalogFragment fragment)
    {
      mFragment = fragment;
    }
  }
}
