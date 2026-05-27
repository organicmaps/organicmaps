package app.organicmaps.widget.placepage.sections;

import android.app.KeyguardManager;
import android.content.Context;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.text.util.Linkify;
import android.util.Base64;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewStub;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.TextView;
import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.widget.placepage.PlacePageViewModel;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

public class PlacePageNotesFragment extends Fragment implements View.OnLongClickListener, Observer<MapObject>
{
  private View mFrame;
  private TextView mTvNotes;
  // Inflated lazily from ViewStub on the first HTML description — most descriptions are plain
  // and bookmark/track place pages without HTML stay WebView-free (~10-20 MB saved per open).
  @Nullable
  private WebView mWvNotes;
  @Nullable
  private String mLastRenderedNotes;

  private PlacePageViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_notes_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mFrame = view;
    mTvNotes = mFrame.findViewById(R.id.tv__notes);
    mTvNotes.setOnLongClickListener(this);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onDestroyView()
  {
    if (mWvNotes != null)
    {
      mWvNotes.destroy();
      mWvNotes = null;
    }
    mFrame = null;
    mTvNotes = null;
    mLastRenderedNotes = null;
    super.onDestroyView();
  }

  private void updateNotes(@NonNull String notes)
  {
    // Skip when content hasn't changed (onChanged fires on every PlacePageViewModel emission).
    if (notes.equals(mLastRenderedNotes))
      return;

    if (TextUtils.isEmpty(notes))
    {
      UiUtils.hide(mTvNotes);
      hideWebView();
    }
    else if (StringUtils.nativeIsHtml(notes))
    {
      UiUtils.hide(mTvNotes);
      final WebView webView = ensureWebView();
      final String themedHtml = wrapWithThemeStyles(notes);
      // According to loadData documentation, HTML should be either base64 or percent encoded.
      // Default UTF-8 encoding for all content is set above in WebSettings.
      final String b64encoded = Base64.encodeToString(themedHtml.getBytes(StandardCharsets.UTF_8), Base64.DEFAULT);
      webView.loadData(b64encoded, Utils.TEXT_HTML, "base64");
      UiUtils.show(webView);
    }
    else
    {
      hideWebView();
      mTvNotes.setText(notes);
      Linkify.addLinks(mTvNotes, Linkify.WEB_URLS | Linkify.EMAIL_ADDRESSES | Linkify.PHONE_NUMBERS);
      UiUtils.show(mTvNotes);
    }
    mLastRenderedNotes = notes;
  }

  @NonNull
  private WebView ensureWebView()
  {
    if (mWvNotes == null)
    {
      final ViewStub stub = mFrame.findViewById(R.id.wv__notes_stub);
      stub.inflate();
      mWvNotes = mFrame.findViewById(R.id.wv__notes);
      mWvNotes.setBackgroundColor(ContextCompat.getColor(requireContext(), R.color.bg_cards));
      final WebSettings settings = mWvNotes.getSettings();
      settings.setJavaScriptEnabled(false);
      settings.setDefaultTextEncodingName("UTF-8");
    }
    return mWvNotes;
  }

  private void hideWebView()
  {
    if (mWvNotes == null)
      return;
    UiUtils.hide(mWvNotes);
    // Release the previously rendered DOM so it doesn't hold ~1-2 MB until the fragment is destroyed.
    mWvNotes.loadUrl("about:blank");
  }

  @NonNull
  private String wrapWithThemeStyles(@NonNull String userHtml)
  {
    final String bgCss = toCssColor(ContextCompat.getColor(requireContext(), R.color.bg_cards));
    final String textCss = toCssColor(mTvNotes.getCurrentTextColor());
    final int fontSizeDp = (int) (mTvNotes.getTextSize() / getResources().getDisplayMetrics().density);
    return "<!DOCTYPE html><html><head>"
  + "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  + "<meta name=\"color-scheme\" content=\"light dark\">"
  + "<style>body{background:" + bgCss + ";color:" + textCss + ";font-size:" + fontSizeDp + "px;margin:0}</style>"
  + "</head><body>" + userHtml + "</body></html>";
  }

  @NonNull
  private static String toCssColor(@ColorInt int color)
  {
    final int a = (color >>> 24) & 0xFF;
    final int r = (color >>> 16) & 0xFF;
    final int g = (color >>> 8) & 0xFF;
    final int b = color & 0xFF;
    return String.format(Locale.ROOT, "rgba(%d,%d,%d,%.3f)", r, g, b, a / 255f);
  }

  @Override
  public boolean onLongClick(View v)
  {
    final String notes = mTvNotes.getText().toString();

    final Context ctx = requireContext();
    Utils.copyTextToClipboard(ctx, notes);

    KeyguardManager keyguardManager = (KeyguardManager) ctx.getSystemService(Context.KEYGUARD_SERVICE);
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU || keyguardManager.isDeviceLocked())
    {
      Utils.showSnackbarAbove(mFrame.getRootView().findViewById(R.id.pp_buttons_layout), mFrame,
                              ctx.getString(R.string.copied_to_clipboard, notes));
    }
    return true;
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    // MapObject could be something without notes if the user already has the place page
    // opened and clicks on a non-bookmark/non-track POI.
    // This callback would be called before the fragment had time to be destroyed.
    if (mapObject == null)
      return;
    updateNotes(mapObject.getDescription());
  }
}
