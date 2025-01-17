package app.organicmaps.widget.placepage;

import android.content.res.Configuration;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;
import app.organicmaps.R;
import app.organicmaps.base.BaseMwmFragment;
import app.organicmaps.util.Utils;
import java.util.Locale;

public class WikiArticleFragment extends BaseMwmFragment
{
  public static final String EXTRA_WIKI_ARTICLE = "description";
  public static final String EXTRA_WIKI_URL = "wiki_url";

  @NonNull
  private String mDescription = "";
  @NonNull
  private String mWikiUrl = "";
  private WebView mWebView;
  private int mLastBottomInset = -1;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mDescription = requireArguments().getString(EXTRA_WIKI_ARTICLE, "");
    mWikiUrl = requireArguments().getString(EXTRA_WIKI_URL, "");
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_place_description, container, false);
    mWebView = root.findViewById(R.id.webview);
    mWebView.setVerticalScrollBarEnabled(true);
    mWebView.getSettings().setBuiltInZoomControls(true);
    mWebView.getSettings().setDisplayZoomControls(false);
    mWebView.setWebViewClient(new WebViewClient() {
      @Override
      public boolean shouldOverrideUrlLoading(WebView view, String url)
      {
        Utils.openUrl(requireContext(), url);
        return true;
      }
    });
    ViewCompat.setOnApplyWindowInsetsListener(root, (v, windowInsets) -> {
      Insets insets = windowInsets.getInsets(WindowInsetsCompat.Type.systemBars());
      v.setPadding(insets.left, 0, insets.right, 0);
      if (mLastBottomInset != insets.bottom)
      {
        mLastBottomInset = insets.bottom;
        loadDescription(insets.bottom);
      }
      return windowInsets;
    });
    return root;
  }

  private void loadDescription(int bottomInsetPx)
  {
    String source = mWikiUrl.isEmpty() ? "<p>" + getString(R.string.article_from_wikipedia) + "</p>"
                                       : "<p><a href='" + TextUtils.htmlEncode(mWikiUrl) + "'>"
                                             + getString(R.string.article_from_wikipedia) + "</a></p>";

    String textColor = colorToHex(isDarkMode() ? R.color.text_light : R.color.text_dark);
    String bgColor = colorToHex(R.color.bg_window);
    String linkColor = colorToHex(R.color.base_accent);

    String html = "<!DOCTYPE html><html><head>"
                + "<meta charset='utf-8'>"
                + "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                + "<link rel='stylesheet' href='wikipedia.css'>"
                + "<style>:root{--text:" + textColor + ";--bg:" + bgColor + ";--link:" + linkColor
                + "}body{padding-bottom:" + (int) (bottomInsetPx / getResources().getDisplayMetrics().density)
                + "px}</style>"
                + "</head><body>" + mDescription + source + "</body></html>";
    mWebView.loadDataWithBaseURL("file:///android_asset/", html, "text/html", "UTF-8", null);
  }

  private boolean isDarkMode()
  {
    int nightFlags = getResources().getConfiguration().uiMode & Configuration.UI_MODE_NIGHT_MASK;
    return nightFlags == Configuration.UI_MODE_NIGHT_YES;
  }

  private String colorToHex(int colorRes)
  {
    return String.format(Locale.ROOT, "#%06X", 0xFFFFFF & ContextCompat.getColor(requireContext(), colorRes));
  }
}
