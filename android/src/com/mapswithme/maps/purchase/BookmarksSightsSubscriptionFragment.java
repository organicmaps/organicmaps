package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;

@SuppressWarnings("WeakerAccess")
public class BookmarksSightsSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksSightsSubscriptionController(requireContext());
  }

  @NonNull
  @Override
  SubscriptionFragmentDelegate createFragmentDelegate(@NonNull AbstractBookmarkSubscriptionFragment fragment)
  {
    return new TwoButtonsSubscriptionFragmentDelegate(fragment);
  }

  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_sightseeing_subscription, container, false);
  }


  @NonNull
  @Override
  SubscriptionType getSubscriptionType()
  {
    return SubscriptionType.BOOKMARKS_SIGHTS;
  }
}
