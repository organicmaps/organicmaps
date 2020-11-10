package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.text.Html;
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

import static com.mapswithme.maps.purchase.BookmarksAllSubscriptionFragment.BUNDLE_DATA;

public class BookmarksAllSubscriptionPageFragment extends Fragment
{
  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    BookmarksAllSubscriptionPageData data = requireArguments().getParcelable(BUNDLE_DATA);
    BookmarksAllSubscriptionPage page = Objects.requireNonNull(data).getPage();
    FragmentBookmarksAllSubscriptionBinding binding = makeBinding(inflater, container);
    binding.setPage(page);
    binding.description.setText(Html.fromHtml(getString(page.getDescriptionId())));
    return binding.getRoot();
  }

  @NonNull
  private static FragmentBookmarksAllSubscriptionBinding makeBinding(@NonNull LayoutInflater inflater,
                                                                @Nullable ViewGroup container)
  {
    return DataBindingUtil.inflate(inflater, R.layout.fragment_bookmarks_all_subscription, container, false);
  }

  @NonNull
  static Fragment newInstance(@NonNull BookmarksAllSubscriptionPageData data)
  {
    BookmarksAllSubscriptionPageFragment fragment = new BookmarksAllSubscriptionPageFragment();
    Bundle args = new Bundle();
    args.putParcelable(BUNDLE_DATA, data);
    fragment.setArguments(args);
    return fragment;
  }
}
