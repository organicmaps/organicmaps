package com.mapswithme.country;

import android.app.Activity;
import android.database.DataSetObserver;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ListView;
import android.widget.TextView;

import com.mapswithme.maps.MapStorage;
import com.mapswithme.maps.R;

public class ExtendedDownloadAdapterWrapper extends DownloadAdapter
{
  protected static final int EXTENDED_VIEWS_COUNT = 3; // 3 more views at the top of baseadapter
  protected static final int DOWNLOADED_ITEM_POSITION = 1;
  protected static final int EXTENDED_VIEW_TYPE_COUNT = 2; // one for placeholder, other for downloader view
  protected static final int TYPE_PLACEHOLDER = 5;
  protected static final int TYPE_EXTENDED = 6;

  protected BaseDownloadAdapter mWrappedAdapter;

  protected DataSetObserver mObserver = new DataSetObserver() {
    @Override
    public void onChanged()
    {
      notifyDataSetChanged();
    }
  };

  public ExtendedDownloadAdapterWrapper(Activity activity, BaseDownloadAdapter wrappedAdapter)
  {
    super(activity);
    mWrappedAdapter = wrappedAdapter;
    mWrappedAdapter.registerDataSetObserver(mObserver);
  }

  @Override
  public boolean isEnabled(int position)
  {
    if (isRootScreen())
    {
      if (position == DOWNLOADED_ITEM_POSITION)
        return true;
      else if (position < EXTENDED_VIEWS_COUNT)
        return false;
    }

    return mWrappedAdapter.isEnabled(getPositionInBaseAdapter(position));
  }

  @Override
  public int getViewTypeCount()
  {
    if (isRootScreen())
      return mWrappedAdapter.getViewTypeCount() + EXTENDED_VIEW_TYPE_COUNT;

    return mWrappedAdapter.getViewTypeCount();
  }

  @Override
  public int getItemViewType(int position)
  {
    if (isRootScreen())
    {
      if (position == DOWNLOADED_ITEM_POSITION)
        return TYPE_EXTENDED;
      else if (position < EXTENDED_VIEWS_COUNT)
        return TYPE_PLACEHOLDER;
    }

    return mWrappedAdapter.getItemViewType(getPositionInBaseAdapter(position));
  }

  @Override
  public View getView(int position, View convertView, ViewGroup parent)
  {
    final int viewType = getItemViewType(position);
    ViewHolder holder = new ViewHolderExtended();
    if (viewType == TYPE_PLACEHOLDER)
    {
      final View view = getPlaceholderView(parent);
      holder.initFromView(view);
      view.setTag(holder);
      return view;
    }
    else if (viewType == TYPE_EXTENDED)
    {
      final View view = getExtendedView(parent);
      holder.initFromView(view);
      view.setTag(holder);
      return view;
    }

    return mWrappedAdapter.getView(getPositionInBaseAdapter(position), convertView, parent);
  }

  private int getPositionInBaseAdapter(int position)
  {
    if (isRootScreen())
      return position - EXTENDED_VIEWS_COUNT;

    return position;
  }

  protected View getPlaceholderView(ViewGroup parent)
  {
    return mInflater.inflate(R.layout.download_item_placeholder, parent, false);
  }

  protected View getExtendedView(ViewGroup parent)
  {
    return mInflater.inflate(R.layout.download_item_extended, parent, false);
  }

  @Override
  public CountryItem getItem(int position)
  {
    if (isRootScreen() && position < EXTENDED_VIEWS_COUNT)
      return null;

    return mWrappedAdapter.getItem(getPositionInBaseAdapter(position));
  }

  @Override
  public int getCount()
  {
    if (isRootScreen())
      return mWrappedAdapter.getCount() + EXTENDED_VIEWS_COUNT;

    return mWrappedAdapter.getCount();
  }

  @Override
  public void onItemClick(int position, View view)
  {
    mWrappedAdapter.onItemClick(getPositionInBaseAdapter(position), view);
  }

  @Override
  public boolean onBackPressed()
  {
    return mWrappedAdapter.onBackPressed();
  }

  protected boolean isRootScreen()
  {
    return mWrappedAdapter.isRoot();
  }

  @Override
  public void onResume(MapStorage.Listener listener)
  {
    mWrappedAdapter.onResume(listener);
  }

  @Override
  public void onPause()
  {
    mWrappedAdapter.onPause();
  }

  @Override
  public void onCountryProgress(ListView list, MapStorage.Index idx, long current, long total)
  {
    mWrappedAdapter.onCountryProgress(list, idx, current, total);
  }

  @Override
  public int onCountryStatusChanged(MapStorage.Index idx)
  {
    return mWrappedAdapter.onCountryStatusChanged(idx);
  }

  protected static class ViewHolderExtended extends ViewHolder
  {
    public TextView tvCount;

    @Override
    void initFromView(View v)
    {
      super.initFromView(v);
      tvCount = (TextView) v.findViewById(R.id.tv__count);
    }
  }
}
