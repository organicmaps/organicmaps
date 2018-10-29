package com.mapswithme.maps.adapter;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;

import java.util.ArrayList;
import java.util.List;

public final class DefaultPositionConverter extends RecyclerCompositeAdapter.AbstractAdapterPositionConverter
{
  @NonNull
  private final List<AdapterIndexAndPosition> mIndexAndPositions;
  @NonNull
  private final List<AdapterIndexAndViewType> mIndexAndViewTypes;

  public DefaultPositionConverter(@NonNull List<RecyclerView.Adapter<RecyclerView.ViewHolder>> adapters)
  {
    Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> pair = makeDataSet(adapters);
    mIndexAndPositions = pair.first;
    mIndexAndViewTypes = pair.second;
  }

  @NonNull
  private static Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> makeDataSet(@NonNull List<RecyclerView.Adapter<RecyclerView.ViewHolder>> adapters)
  {
    List<AdapterIndexAndPosition> indexAndPositions = new ArrayList<>();
    List<AdapterIndexAndViewType> indexAndViewTypes = new ArrayList<>();
    for (int j = 0; j < adapters.size(); j++)
    {
      RecyclerView.Adapter<RecyclerView.ViewHolder> each = adapters.get(j);
      int itemCount = each.getItemCount();
      for (int i = 0; i < itemCount ; i++)
      {
        AdapterIndexAndPosition positionItem = new AdapterIndexAndPositionImpl(j, i);
        indexAndPositions.add(positionItem);
        int relItemViewType = each.getItemViewType(i);

        AdapterIndexAndViewType typeItem = new AdapterIndexAndViewTypeImpl(j, relItemViewType);
        if (!indexAndViewTypes.contains(typeItem))
          indexAndViewTypes.add(typeItem);
      }
    }

    return new Pair<>(indexAndPositions, indexAndViewTypes);
  }

  @NonNull
  @Override
  protected List<AdapterIndexAndViewType> getIndexAndViewTypeItems()
  {
    return mIndexAndViewTypes;
  }

  @NonNull
  @Override
  protected List<AdapterIndexAndPosition> getIndexAndPositionItems()
  {
    return mIndexAndPositions;
  }
}
