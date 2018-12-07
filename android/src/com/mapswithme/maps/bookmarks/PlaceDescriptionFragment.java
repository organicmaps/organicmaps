package com.mapswithme.maps.bookmarks;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;
import com.mapswithme.util.Utils;

import java.util.Objects;

public class PlaceDescriptionFragment extends BaseMwmFragment
{
  public static final String EXTRA_DESCRIPTION = "description";
  @SuppressWarnings("NullableProblems")
  @NonNull
  private WebView mWebView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private String mDesc;

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    mDesc = Objects.requireNonNull(getArguments().getString(EXTRA_DESCRIPTION));
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_place_description, container, false);
    mWebView = root.findViewById(R.id.webview);
    mWebView.loadData(mDesc, Utils.TEXT_HTML_CHARSET_UTF_8, null);
    mWebView.setVerticalScrollBarEnabled(true);
    return root;
  }

}
