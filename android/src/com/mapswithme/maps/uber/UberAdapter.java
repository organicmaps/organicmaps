package com.mapswithme.maps.uber;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v4.view.PagerAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;

public class UberAdapter extends PagerAdapter
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final UberInfo.Product[] mProducts;

  public UberAdapter(@NonNull Context context, @NonNull UberInfo.Product[] products)
  {
    mContext = context;
    mProducts = products;
  }

  @Override
  public int getCount()
  {
    return mProducts.length;
  }

  @Override
  public boolean isViewFromObject(View view, Object object)
  {
    return view == object;
  }

  @Override
  public Object instantiateItem(ViewGroup container, int position)
  {
    UberInfo.Product product = mProducts[position];

    View v = LayoutInflater.from(mContext).inflate(R.layout.uber_pager_item, container, false);
    TextView name = (TextView) v.findViewById(R.id.product_name);
    name.setText(product.getName());
    TextView timeAndPrice = (TextView) v.findViewById(R.id.arrival_time_price);
    int time = Integer.parseInt(product.getTime());
    CharSequence waitTime = RoutingController.formatRoutingTime(mContext, time, R.dimen.text_size_body_3);
    timeAndPrice.setText(mContext.getString(R.string.taxi_wait, waitTime + " • " + product.getPrice()));
    container.addView(v, 0);
    return v;
  }

  @Override
  public void destroyItem(ViewGroup container, int position, Object object)
  {
    container.removeView((View) object);
  }
}
