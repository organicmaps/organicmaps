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
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.Bookmark;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.log.Logger;

public class PlacePageBookmarkFragment extends Fragment implements View.OnClickListener,
                                                                   View.OnLongClickListener,
                                                                   Observer<MapObject>,
                                                                   EditBookmarkFragment.EditBookmarkListener
{
  private static final String TAG = PlacePageBookmarkFragment.class.getSimpleName();

  private View mFrame;
  private TextView mTvBookmarkNote;
  @Nullable
  private WebView mWvBookmarkNote;

  private PlacePageViewModel viewModel;

  // TODO description header is not shown

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
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

    viewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    viewModel.getMapObject().observe(requireActivity(), this);
  }

  private void initWebView()
  {
    if (mWvBookmarkNote != null)
      return;
    mWvBookmarkNote = new WebView(requireContext());
    final WebSettings settings = mWvBookmarkNote.getSettings();
    settings.setJavaScriptEnabled(false);
    settings.setDefaultTextEncodingName("UTF-8");
    final LinearLayout linearLayout = mFrame.findViewById(R.id.place_page_bookmark_layout);
    // Add the webview in last position
    linearLayout.addView(mWvBookmarkNote, linearLayout.getChildCount() - 1);
  }

  @Nullable
  private MapObject getMapObject()
  {
    if (viewModel != null)
      return viewModel.getMapObject().getValue();
    return null;
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    viewModel.getMapObject().removeObserver(this);
  }

  private void updateBookmarkDetails(@NonNull MapObject mapObject)
  {
    if (mapObject.getMapObjectType() != MapObject.BOOKMARK)
      return;

    final String notes = ((Bookmark) mapObject).getBookmarkDescription();
    if (TextUtils.isEmpty(notes))
    {
      UiUtils.hide(mTvBookmarkNote);
      if (mWvBookmarkNote != null)
        UiUtils.hide(mWvBookmarkNote);
      return;
    }

    if (StringUtils.nativeIsHtml(notes))
    {
      // According to loadData documentation, HTML should be either base64 or percent encoded.
      // Default UTF-8 encoding for all content is set above in WebSettings.
      initWebView();
      if (mWvBookmarkNote != null)
      {
        final String b64encoded = Base64.encodeToString(notes.getBytes(), Base64.DEFAULT);
        mWvBookmarkNote.loadData(b64encoded, Utils.TEXT_HTML, "base64");
        UiUtils.show(mWvBookmarkNote);
        UiUtils.hide(mTvBookmarkNote);
      }
    }
    else
    {
      mTvBookmarkNote.setText(notes);
      Linkify.addLinks(mTvBookmarkNote, Linkify.ALL);
      UiUtils.show(mTvBookmarkNote);
      if (mWvBookmarkNote != null)
        UiUtils.hide(mWvBookmarkNote);
    }
  }

  @Override
  public void onClick(View v)
  {
    final MapObject mapObject = getMapObject();
    if (mapObject == null)
    {
      Logger.e(TAG, "A bookmark cannot be edited, mMapObject is null!");
      return;
    }
    Bookmark bookmark = (Bookmark) mapObject;
    EditBookmarkFragment.editBookmark(bookmark.getCategoryId(),
                                      bookmark.getBookmarkId(),
                                      requireActivity(),
                                      requireActivity().getSupportFragmentManager(),
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
    if (mapObject != null)
      updateBookmarkDetails(mapObject);
  }

  @Override
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    Bookmark updatedBookmark = BookmarkManager.INSTANCE.updateBookmarkPlacePage(bookmarkId);
    if (updatedBookmark == null)
      return;
    viewModel.setMapObject(updatedBookmark);
  }
}
