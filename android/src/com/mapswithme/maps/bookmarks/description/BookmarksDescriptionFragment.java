package com.mapswithme.maps.bookmarks.description;

import android.os.Bundle;
import android.util.Base64;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.ActionBar;
import androidx.appcompat.app.AppCompatActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.maps.bookmarks.BookmarkHeaderView;
import com.mapswithme.maps.bookmarks.data.BookmarkCategory;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

import java.util.Objects;

import static com.mapswithme.maps.bookmarks.description.BookmarksDescriptionActivity.EXTRA_CATEGORY;

public class BookmarksDescriptionFragment extends BaseMwmFragment
{
  @NonNull
  private BookmarkCategory mBookmarkCategory;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    Bundle argument = getArguments();
    if (argument != null && argument.containsKey(EXTRA_CATEGORY))
      mBookmarkCategory = Objects.requireNonNull(argument.getParcelable(EXTRA_CATEGORY), "No argument provided!");
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_bookmarks_description, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    ActionBar bar = ((AppCompatActivity) requireActivity()).getSupportActionBar();
    if (bar != null)
      bar.setTitle(R.string.description_guide);

    BookmarkHeaderView headerView = view.findViewById(R.id.guide_info);
    headerView.setCategory(mBookmarkCategory);
    TextView btnDescription = view.findViewById(R.id.btn_description);
    UiUtils.hide(btnDescription);
    WebView webView = view.findViewById(R.id.webview);
    String base64version = Base64.encodeToString(mBookmarkCategory.getDescription().getBytes(),
                                                 Base64.DEFAULT);
    webView.loadData(base64version, Utils.TEXT_HTML, Utils.BASE_64);
  }
}
