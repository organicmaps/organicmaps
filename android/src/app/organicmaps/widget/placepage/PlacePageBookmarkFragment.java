package app.organicmaps.widget.placepage;

import android.content.Context;
import android.os.Bundle;
import android.text.TextUtils;
import android.text.util.Linkify;
import android.util.Base64;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.Bookmark;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;

public class PlacePageBookmarkFragment extends Fragment implements View.OnClickListener,
                                                                   View.OnLongClickListener,
                                                                   Observer<MapObject>,
                                                                   EditBookmarkFragment.EditBookmarkListener
{
  private View mFrame;
  private TextView mTvBookmarkNote;
  @Nullable
  private WebView mWvBookmarkNote;

  private PlacePageViewModel mViewModel;

  private Bookmark currentBookmark;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_bookmark_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    mFrame = view;
    mTvBookmarkNote = mFrame.findViewById(R.id.tv__bookmark_notes);
    mTvBookmarkNote.setOnLongClickListener(this);
    final View editBookmarkBtn = mFrame.findViewById(R.id.tv__bookmark_edit);
    editBookmarkBtn.setOnClickListener(this);
  }

  private void initWebView()
  {
    mWvBookmarkNote = new WebView(requireContext());
    final WebSettings settings = mWvBookmarkNote.getSettings();
    settings.setJavaScriptEnabled(false);
    settings.setDefaultTextEncodingName("UTF-8");
    final LinearLayout linearLayout = mFrame.findViewById(R.id.place_page_bookmark_layout);
    // Add the webview in last position
    linearLayout.addView(mWvBookmarkNote, linearLayout.getChildCount() - 1);
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onPause()
  {
    super.onPause();
    mViewModel.getMapObject().removeObserver(this);
  }

  private void updateBookmarkDetails()
  {
    final String notes = currentBookmark.getBookmarkDescription();
    if (TextUtils.isEmpty(notes))
    {
      UiUtils.hide(mTvBookmarkNote);
      if (mWvBookmarkNote != null)
        UiUtils.hide(mWvBookmarkNote);
    }
    else if (StringUtils.nativeIsHtml(notes))
    {
      if (mWvBookmarkNote == null)
        initWebView();
      // According to loadData documentation, HTML should be either base64 or percent encoded.
      // Default UTF-8 encoding for all content is set above in WebSettings.
      final String b64encoded = Base64.encodeToString(notes.getBytes(), Base64.DEFAULT);
      mWvBookmarkNote.loadData(b64encoded, Utils.TEXT_HTML, "base64");
      UiUtils.show(mWvBookmarkNote);
      UiUtils.hide(mTvBookmarkNote);
    }
    else
    {
      mTvBookmarkNote.setText(notes);
      Linkify.addLinks(mTvBookmarkNote, Linkify.WEB_URLS | Linkify.EMAIL_ADDRESSES | Linkify.PHONE_NUMBERS);
      UiUtils.show(mTvBookmarkNote);
      if (mWvBookmarkNote != null)
        UiUtils.hide(mWvBookmarkNote);
    }
  }

  @Override
  public void onClick(View v)
  {
    final FragmentActivity activity = requireActivity();
    EditBookmarkFragment.editBookmark(currentBookmark.getCategoryId(),
                                      currentBookmark.getBookmarkId(),
                                      activity,
                                      activity.getSupportFragmentManager(),
                                      PlacePageBookmarkFragment.this);
  }

  @Override
  public boolean onLongClick(View v)
  {
    final String notes = mTvBookmarkNote.getText().toString();

    final Context ctx = requireContext();
    Utils.copyTextToClipboard(ctx, notes);
    Utils.showSnackbarAbove(mFrame,
                            mFrame.getRootView().findViewById(R.id.pp_buttons_layout),
                            ctx.getString(R.string.copied_to_clipboard, notes));
    return true;
  }

  @Override
  public void onChanged(MapObject mapObject)
  {
    // MapObject could be something else than a bookmark if the user already has the place page
    // opened and clicks on a non-bookmarked POI.
    // This callback would be called before the fragment had time to be destroyed
    if (mapObject.getMapObjectType() == MapObject.BOOKMARK)
    {
      currentBookmark = (Bookmark) mapObject;
      updateBookmarkDetails();
    }
  }

  @Override
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    Bookmark updatedBookmark = BookmarkManager.INSTANCE.updateBookmarkPlacePage(bookmarkId);
    if (updatedBookmark == null)
      return;
    mViewModel.setMapObject(updatedBookmark);
  }
}
