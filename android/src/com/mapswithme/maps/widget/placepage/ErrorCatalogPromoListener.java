package com.mapswithme.maps.widget.placepage;

import androidx.annotation.NonNull;
import androidx.fragment.app.FragmentActivity;

import com.mapswithme.maps.gallery.Items;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.Utils;

public class ErrorCatalogPromoListener<T extends Items.Item> implements com.mapswithme.maps.gallery.ItemSelectedListener<T>
{
  @NonNull
  private final FragmentActivity mActivity;
  @NonNull
  private final NetworkPolicy.NetworkPolicyListener mListener;

  public ErrorCatalogPromoListener(@NonNull FragmentActivity activity,
                                   @NonNull NetworkPolicy.NetworkPolicyListener listener)
  {
    mActivity = activity;
    mListener = listener;
  }

  @Override
  public void onMoreItemSelected(@NonNull T item)
  {
  }

  @Override
  public void onActionButtonSelected(@NonNull T item, int position)
  {
  }

  @Override
  public void onItemSelected(@NonNull T item, int position)
  {
    if (ConnectionState.INSTANCE.isConnected())
      NetworkPolicy.checkNetworkPolicy(mActivity.getSupportFragmentManager(),  mListener, true);
    else
      Utils.showSystemConnectionSettings(getActivity());
  }

  @NonNull
  protected FragmentActivity getActivity()
  {
    return mActivity;
  }
}
