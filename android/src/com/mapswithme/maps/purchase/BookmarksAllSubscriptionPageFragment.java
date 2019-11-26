package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.Fragment;
import com.mapswithme.maps.R;
import com.mapswithme.maps.databinding.FragmentBookmarksAllSubscriptionBinding;

import java.util.Objects;

public class BookmarksAllSubscriptionPageFragment extends Fragment
{
  private static final String BUNDLE_INDEX = "index";

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    int index = Objects.requireNonNull(getArguments()).getInt(BUNDLE_INDEX);
    BookmarksAllSubscriptionPage page = BookmarksAllSubscriptionPage.values()[index];
    FragmentBookmarksAllSubscriptionBinding binding = makeBinding(inflater, container);
    binding.setPage(page);
    return binding.getRoot();
  }

  @NonNull
  private static FragmentBookmarksAllSubscriptionBinding makeBinding(@NonNull LayoutInflater inflater,
                                                                @Nullable ViewGroup container)
  {
    return DataBindingUtil.inflate(inflater, R.layout.fragment_bookmarks_all_subscription, container, false);
  }

  @NonNull
  static Fragment newInstance(int index)
  {
    BookmarksAllSubscriptionPageFragment fragment = new BookmarksAllSubscriptionPageFragment();
    Bundle args = new Bundle();
    args.putInt(BUNDLE_INDEX, index);
    fragment.setArguments(args);
    return fragment;
  }
}
