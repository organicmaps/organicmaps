package com.mapswithme.maps.purchase;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import com.mapswithme.maps.R;

public class BookmarksSightsSubscriptionFragment extends AbstractBookmarkSubscriptionFragment
{
  @SuppressWarnings("NullableProblems")
  @NonNull
  private SubscriptionFragmentDelegate mDelegate;

  @NonNull
  @Override
  PurchaseController<PurchaseCallback> createPurchaseController()
  {
    return PurchaseFactory.createBookmarksSightsSubscriptionController(requireContext());
  }

  @Nullable
  @Override
  View onSubscriptionCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                                @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_sightseeing_subscription, container, false);

    mDelegate = new SubscriptionFragmentDelegate(this);
    mDelegate.onSubscriptionCreateView(root);

    return root;
  }

  @Override
  void onSubscriptionDestroyView()
  {
    mDelegate.onSubscriptionDestroyView();
  }

  @NonNull
  @Override
  SubscriptionType getSubscriptionType()
  {
    return SubscriptionType.BOOKMARKS_SIGHTS;
  }

  @Override
  public void onProductDetailsLoading()
  {
    super.onProductDetailsLoading();
    mDelegate.onProductDetailsLoading();
  }

  @Override
  void hideButtonProgress()
  {
    mDelegate.hideButtonProgress();
  }

  @Override
  void showButtonProgress()
  {
    mDelegate.showButtonProgress();
  }

  @NonNull
  @Override
  PurchaseUtils.Period getSelectedPeriod()
  {
    return mDelegate.getSelectedPeriod();
  }

  @Override
  public void onReset()
  {
    mDelegate.onReset();
  }

  @Override
  public void onPriceSelection()
  {
    mDelegate.onPriceSelection();
  }
}
