package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;

public class BookmarkSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.bookmark_subscription_fragment, container, false);
  }

  @NonNull
  @Override
  SubscriptionType getSubscriptionType()
  {
    return SubscriptionType.BOOKMARKS_ALL;
  }

  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksAllSubscriptionController(requireContext());
  }

  @NonNull
  @Override
  SubscriptionFragmentDelegate createFragmentDelegate(@NonNull AbstractBookmarkSubscriptionFragment fragment)
  {
    return new TwoCardsSubscriptionFragmentDelegate(fragment);
  }
}
