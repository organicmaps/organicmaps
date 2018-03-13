package com.mapswithme.maps.taxi;

import android.content.Context;
import android.support.annotation.NonNull;
import android.support.v4.view.PagerAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.Utils;

import java.util.List;

public class TaxiAdapter extends PagerAdapter
{
  @NonNull
  private final Context mContext;
  @NonNull
  private final List<TaxiInfo.Product> mProducts;
  @NonNull
  private final TaxiType mType;

  public TaxiAdapter(@NonNull Context context, @NonNull TaxiType type,
                     @NonNull List<TaxiInfo.Product> products)
  {
    mContext = context;
    mType = type;
    mProducts = products;
  }

  @Override
  public int getCount()
  {
    return mProducts.size();
  }

  @Override
  public boolean isViewFromObject(View view, Object object)
  {
    return view == object;
  }

  @Override
  public Object instantiateItem(ViewGroup container, int position)
  {
    TaxiInfo.Product product = mProducts.get(position);

    View v = LayoutInflater.from(mContext).inflate(R.layout.taxi_pager_item, container, false);
    TextView name = (TextView) v.findViewById(R.id.product_name);
    String separator;
    // We ignore all Yandex.Taxi product names until they do support of passing product parameters
    // to their app via deeplink.
    if (mType == TaxiType.PROVIDER_YANDEX || mType == TaxiType.PROVIDER_MAXIM)
    {
      name.setText(mType.getTitle());
      separator = " • ~";
    }
    else
    {
      name.setText(product.getName());
      separator = " • ";
    }
    TextView timeAndPrice = (TextView) v.findViewById(R.id.arrival_time_price);
    int time = Integer.parseInt(product.getTime());
    CharSequence waitTime = RoutingController.formatRoutingTime(mContext, time,
                                                                R.dimen.text_size_body_3);
    timeAndPrice.setText(mContext.getString(R.string.taxi_wait, waitTime + separator
                                                                + formatPrice(product)));
    container.addView(v, 0);
    return v;
  }

  @NonNull
  private String formatPrice(@NonNull TaxiInfo.Product product)
  {
    if (mType == TaxiType.PROVIDER_YANDEX)
      return Utils.formatCurrencyString(product.getPrice(), product.getCurrency());
    // For Uber and Maxim we don't do formatting, because Uber and Maxim does it on its side.
    return product.getPrice();
  }

  @Override
  public void destroyItem(ViewGroup container, int position, Object object)
  {
    container.removeView((View) object);
  }
}
