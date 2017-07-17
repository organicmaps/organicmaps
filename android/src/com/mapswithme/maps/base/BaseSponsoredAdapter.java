package com.mapswithme.maps.base;

import android.support.annotation.CallSuper;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.placepage.Sponsored;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

public abstract class BaseSponsoredAdapter extends RecyclerView.Adapter<BaseSponsoredAdapter.ViewHolder>
{
  private static final int MAX_ITEMS = 5;

  protected static final int TYPE_PRODUCT = 0;
  private static final int TYPE_MORE = 1;

  private static final String MORE = MwmApplication.get().getString(R.string.placepage_more_button);

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_PRODUCT, TYPE_MORE })
  @interface ViewType{}

  @NonNull
  private final List<Item> mItems;
  @Nullable
  private final ItemSelectedListener mListener;

  public BaseSponsoredAdapter(@Sponsored.SponsoredType int sponsoredType,
                              @NonNull List<? extends Item> items, @NonNull String url,
                              @Nullable ItemSelectedListener listener)
  {
    mItems = new ArrayList<>();
    mListener = listener;
    boolean showMoreItem = items.size() >= MAX_ITEMS;
    int size = showMoreItem ? MAX_ITEMS : items.size();
    for (int i = 0; i < size; i++)
    {
      Item product = items.get(i);
      mItems.add(product);
    }
    if (showMoreItem)
      mItems.add(new Item(TYPE_MORE, sponsoredType, MORE, url));
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup parent, @ViewType int viewType)
  {
    switch (viewType)
    {
      case TYPE_PRODUCT:
        return createViewHolder(LayoutInflater.from(parent.getContext()), parent);
      case TYPE_MORE:
        return new ViewHolder(LayoutInflater.from(parent.getContext())
                                            .inflate(R.layout.item_viator_more,
                                                     parent, false), this);
    }
    return null;
  }

  @Override
  public void onBindViewHolder(ViewHolder holder, int position)
  {
    holder.bind(mItems.get(position));
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  @Override
  @ViewType
  public int getItemViewType(int position)
  {
    return mItems.get(position).mType;
  }

  @NonNull
  protected abstract ViewHolder createViewHolder(@NonNull LayoutInflater inflater,
                                                 @NonNull ViewGroup parent);

  public static class ViewHolder extends RecyclerView.ViewHolder
      implements View.OnClickListener
  {
    @NonNull
    TextView mTitle;
    @NonNull
    BaseSponsoredAdapter mAdapter;

    protected ViewHolder(@NonNull View itemView, @NonNull BaseSponsoredAdapter adapter)
    {
      super(itemView);
      mTitle = (TextView) itemView.findViewById(R.id.tv__title);
      mAdapter = adapter;
      itemView.setOnClickListener(this);
    }

    @CallSuper
    public void bind(@NonNull Item item)
    {
      mTitle.setText(item.mTitle);
    }

    @Override
    public void onClick(View v)
    {
      int position = getAdapterPosition();
      if (position == RecyclerView.NO_POSITION)
        return;

      onItemSelected(mAdapter.mItems.get(position));
    }

    void onItemSelected(@NonNull Item item)
    {
      if (mAdapter.mListener != null)
      {
        if (item.mType == TYPE_PRODUCT)
          mAdapter.mListener.onItemSelected(item.mUrl, item.mSponsoredType);
        else if (item.mType == TYPE_MORE)
          mAdapter.mListener.onMoreItemSelected(item.mUrl, item.mSponsoredType);
      }
    }
  }

  public static class Item
  {
    @ViewType
    private final int mType;
    @Sponsored.SponsoredType
    private final int mSponsoredType;
    @NonNull
    private final String mTitle;
    @NonNull
    private final String mUrl;

    protected Item(@ViewType int type, @Sponsored.SponsoredType int sponsoredType,
                   @NonNull String title, @NonNull String url)
    {
      mType = type;
      mSponsoredType = sponsoredType;
      mTitle = title;
      mUrl = url;
    }
  }

  public interface ItemSelectedListener
  {
    void onItemSelected(@NonNull String url, @Sponsored.SponsoredType int type);
    void onMoreItemSelected(@NonNull String url, @Sponsored.SponsoredType int type);
  }
}
