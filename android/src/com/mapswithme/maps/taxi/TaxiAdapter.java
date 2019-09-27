package com.mapswithme.maps.taxi;

import android.content.Context;
import androidx.annotation.NonNull;
import androidx.viewpager.widget.PagerAdapter;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.util.UiUtils;

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
    boolean isApproxPrice = mType.isPriceApproximated();
    name.setText(isApproxPrice ? mContext.getString(mType.getTitle()) : product.getName());
    String separator = UiUtils.PHRASE_SEPARATOR + (isApproxPrice ? UiUtils.APPROXIMATE_SYMBOL : "");
    TextView timeAndPriceView = (TextView) v.findViewById(R.id.arrival_time_price);
    int time = Integer.parseInt(product.getTime());
    CharSequence waitTime = RoutingController.formatRoutingTime(mContext, time,
                                                                R.dimen.text_size_body_3);
    String formattedPrice = mType.getFormatPriceStrategy().format(product);
    String timeAndPriceValue = waitTime + separator + formattedPrice;
    String timeAndPrice = mContext.getString(mType.getWaitingTemplateResId(), timeAndPriceValue);
    timeAndPriceView.setText(timeAndPrice);
    container.addView(v, 0);
    return v;
  }

  @Override
  public void destroyItem(ViewGroup container, int position, Object object)
  {
    container.removeView((View) object);
  }
}
