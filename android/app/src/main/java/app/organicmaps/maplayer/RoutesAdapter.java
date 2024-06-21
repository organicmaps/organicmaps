package app.organicmaps.maplayer;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.RecyclerView;

import java.util.List;

import app.organicmaps.R;
import app.organicmaps.util.SharedPropertiesUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.log.Logger;

public class RoutesAdapter extends RecyclerView.Adapter<RouteHolder>
{
  @NonNull
  private final List<RouteBottomSheetItem> mItems;

  public RoutesAdapter(@NonNull List<RouteBottomSheetItem> items)
  {
    mItems = items;
  }

  @NonNull
  @Override
  public RouteHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    LayoutInflater inflater = LayoutInflater.from(parent.getContext());
    View root = inflater.inflate(R.layout.item_myroute_button, parent, false);
    return new RouteHolder(root);
  }

  @Override
  public void onBindViewHolder(RouteHolder holder, int position)
  {
    RouteBottomSheetItem item = mItems.get(position);
    holder.mItem = item;

    holder.mTitle.setSelected(true);
    holder.mTitle.setText(item.getRouteName());

    holder.mTitleListener = item::onTitleClick;
    holder.mRenameListener = item::onRenameClick;
    holder.mDeleteListener = item::onDeleteClick;
  }

  @Override
  public int getItemCount()
  {
    return mItems.size();
  }

  public void addRoute(@NonNull RouteBottomSheetItem item)
  {
    // Compare strings toUpperCase to ignore case
    String routeName = item.getRouteName().toUpperCase();
    String iName;
    // Find index to add ordered
    int pos = mItems.size();
    for (int i = 0; i < mItems.size(); i++)
    {
      iName = mItems.get(i).getRouteName().toUpperCase();
      if(routeName.compareTo(iName) < 0)
      {
        pos = i;
        break;
      }
    }
    mItems.add(pos, item);
    notifyItemInserted(pos);
  }

  public void removeRoute(@NonNull RouteBottomSheetItem item)
  {
    int pos = mItems.indexOf(item);
    mItems.remove(item);
    notifyItemRemoved(pos);
  }
}
