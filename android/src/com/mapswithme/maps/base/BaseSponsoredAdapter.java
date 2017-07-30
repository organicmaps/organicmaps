package com.mapswithme.maps.base;

import android.support.annotation.CallSuper;
import android.support.annotation.IntDef;
import android.support.annotation.LayoutRes;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.placepage.Sponsored;
import com.mapswithme.util.UiUtils;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayList;
import java.util.List;

public abstract class BaseSponsoredAdapter extends RecyclerView.Adapter<BaseSponsoredAdapter.ViewHolder>
{
  private static final int MAX_ITEMS = 5;

  protected static final int TYPE_PRODUCT = 0;
  private static final int TYPE_MORE = 1;
  private static final int TYPE_LOADING = 2;

  private static final String MORE = MwmApplication.get().getString(R.string.placepage_more_button);
  private static final int TARGET_LOAD_WIDTH = UiUtils.dimen(MwmApplication.get(),
                                                        R.dimen.viator_product_width);
  private static final int MARGING_QUARTER = UiUtils.dimen(MwmApplication.get(),
                                                             R.dimen.margin_quarter);
  private static final String ERROR_SUBTITLE = MwmApplication
      .get().getString(R.string.error_load_information);

  @Retention(RetentionPolicy.SOURCE)
  @IntDef({ TYPE_PRODUCT, TYPE_MORE, TYPE_LOADING })
  @interface ViewType{}

  @NonNull
  private final List<Item> mItems;
  @Nullable
  private final ItemSelectedListener mListener;
  @Sponsored.SponsoredType
  private final int mSponsoredType;

  public BaseSponsoredAdapter(@Sponsored.SponsoredType int sponsoredType, @NonNull String url,
                              boolean hasError, @Nullable ItemSelectedListener listener)
  {
    mSponsoredType = sponsoredType;
    mItems = new ArrayList<>();
    mListener = listener;
    String subtitle = hasError ? ERROR_SUBTITLE : getLoadingSubtitle();
    mItems.add(new Item(TYPE_LOADING, sponsoredType, getLoadingTitle(), url, subtitle, hasError,
                        false));
  }

  public BaseSponsoredAdapter(@Sponsored.SponsoredType int sponsoredType,
                              @NonNull List<? extends Item> items, @NonNull String url,
                              @Nullable ItemSelectedListener listener)
  {
    mSponsoredType = sponsoredType;
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
      mItems.add(new Item(TYPE_MORE, sponsoredType, MORE, url, null, false, false));
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
                                            .inflate(getMoreLayout(), parent, false), this);
      case TYPE_LOADING:
        return createLoadingViewHolder(LayoutInflater.from(parent.getContext()), parent);
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

  public boolean containsLoading()
  {
    return mItems.size() == 1 && mItems.get(0).mType == TYPE_LOADING;
  }

  public void setLoadingError(@Sponsored.SponsoredType int sponsoredType, @NonNull String url)
  {
    mItems.clear();
    mItems.add(new Item(TYPE_LOADING, sponsoredType, getLoadingTitle(), url, ERROR_SUBTITLE,
                        true, false));
    notifyItemChanged(0/* position */);
  }

  public void setLoadingCompleted(@Sponsored.SponsoredType int sponsoredType, @NonNull String url)
  {
    mItems.clear();
    mItems.add(new Item(TYPE_LOADING, sponsoredType, getLoadingTitle(), url, getLoadingSubtitle(),
                        false, true));
    notifyItemChanged(0/* position */);
  }

  @NonNull
  protected abstract ViewHolder createViewHolder(@NonNull LayoutInflater inflater,
                                                 @NonNull ViewGroup parent);

  @NonNull
  protected abstract ViewHolder createLoadingViewHolder(@NonNull LayoutInflater inflater,
                                                        @NonNull ViewGroup parent);

  @NonNull
  protected abstract String getLoadingTitle();

  @Nullable
  protected abstract String getLoadingSubtitle();

  @LayoutRes
  protected abstract int getMoreLayout();

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
        else if (item.mType == TYPE_MORE || item.mType == TYPE_LOADING)
          mAdapter.mListener.onMoreItemSelected(item.mUrl, item.mSponsoredType);
      }
    }
  }

  public static class LoadingViewHolder extends ViewHolder
      implements View.OnClickListener
  {
    @NonNull
    ProgressBar mProgressBar;
    @NonNull
    TextView mSubtitle;

    public LoadingViewHolder(@NonNull View itemView, @NonNull BaseSponsoredAdapter adapter)
    {
      super(itemView, adapter);
      mProgressBar = (ProgressBar) itemView.findViewById(R.id.pb__progress);
      mSubtitle = (TextView) itemView.findViewById(R.id.tv__subtitle);
    }

    @CallSuper
    @Override
    public void bind(@NonNull Item item)
    {
      super.bind(item);
      UiUtils.setTextAndHideIfEmpty(mSubtitle, item.mSubtitle);
      if (item.mFinished)
      {
        UiUtils.hide(mTitle, mSubtitle, mProgressBar);
        ViewGroup.LayoutParams lp = itemView.getLayoutParams();
        lp.width = TARGET_LOAD_WIDTH;
        itemView.setLayoutParams(lp);
        itemView.setPadding(itemView.getLeft(), itemView.getTop(),
                            MARGING_QUARTER, itemView.getBottom());
      }
      if (item.mLoadingError)
        UiUtils.hide(mProgressBar);
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
        else if (item.mType == TYPE_LOADING && item.mLoadingError)
          mAdapter.mListener.onItemSelected(item.mUrl, item.mSponsoredType);
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
    @Nullable
    private final String mSubtitle;
    private final boolean mLoadingError;
    private final boolean mFinished;

    protected Item(@ViewType int type, @Sponsored.SponsoredType int sponsoredType,
                   @NonNull String title, @NonNull String url, @Nullable String subtitle,
                   boolean loadingError, boolean finished)
    {
      mType = type;
      mSponsoredType = sponsoredType;
      mTitle = title;
      mUrl = url;
      mSubtitle = subtitle;
      mLoadingError = loadingError;
      mFinished = finished;
    }
  }

  public interface ItemSelectedListener
  {
    void onItemSelected(@NonNull String url, @Sponsored.SponsoredType int type);
    void onMoreItemSelected(@NonNull String url, @Sponsored.SponsoredType int type);
  }
}
