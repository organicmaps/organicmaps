package com.mapswithme.maps.search;

import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import com.mapswithme.maps.R;
import com.mapswithme.maps.widget.SearchToolbarController;

class SearchHistoryAdapter extends RecyclerView.Adapter<SearchHistoryAdapter.ViewHolder>
{
  private static final int TYPE_ITEM = 0;
  private static final int TYPE_CLEAR = 1;

  private final SearchToolbarController mSearchToolbarController;

  public static class ViewHolder extends RecyclerView.ViewHolder
  {
    private final TextView mText;

    public ViewHolder(View itemView)
    {
      super(itemView);
      mText = (TextView) itemView;
    }
  }

  private void refresh()
  {
    SearchEngine.INSTANCE.refreshRecents();
    notifyDataSetChanged();
  }

  public SearchHistoryAdapter(SearchToolbarController searchToolbarController)
  {
    mSearchToolbarController = searchToolbarController;
    refresh();
  }

  @Override
  public ViewHolder onCreateViewHolder(ViewGroup viewGroup, int type)
  {
    final ViewHolder res;

    switch (type)
    {
      case TYPE_ITEM:
        res = new ViewHolder(LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.item_search_recent, viewGroup, false));
        res.mText.setOnClickListener(new View.OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
            mSearchToolbarController.setQuery(res.mText.getText());
          }
        });
        break;

      case TYPE_CLEAR:
        res = new ViewHolder(LayoutInflater.from(viewGroup.getContext()).inflate(R.layout.item_search_common, viewGroup, false));
        res.mText.setOnClickListener(new View.OnClickListener()
        {
          @Override
          public void onClick(View v)
          {
            SearchEngine.INSTANCE.clearRecents();
            notifyDataSetChanged();
          }
        });
        break;

      default:
        throw new IllegalArgumentException("Unsupported ViewHolder type given");
    }

    return res;
  }

  @Override
  public void onBindViewHolder(ViewHolder viewHolder, int position)
  {
    if (getItemViewType(position) == TYPE_ITEM)
      viewHolder.mText.setText(SearchEngine.INSTANCE.getRecent(position));
  }

  @Override
  public int getItemCount()
  {
    int res = SearchEngine.INSTANCE.getRecentsSize();
    return (res == 0 ? 0 : res + 1);
  }

  @Override
  public int getItemViewType(int position)
  {
    return (position < SearchEngine.INSTANCE.getRecentsSize() ? TYPE_ITEM : TYPE_CLEAR);
  }
}
