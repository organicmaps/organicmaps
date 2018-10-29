package com.mapswithme.maps.adapter;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;

import java.util.ArrayList;
import java.util.List;

public final class RepeatablePairPositionConverter extends RecyclerCompositeAdapter.AbstractAdapterPositionConverter
{
  private static final int FIRST_ADAPTER_INDEX = 0;
  private static final int SECOND_ADAPTER_INDEX = 1;

  @NonNull
  private final List<AdapterIndexAndPosition> mIndexAndPositions;
  @NonNull
  private final List<AdapterIndexAndViewType> mIndexAndViewTypes;

  public RepeatablePairPositionConverter(@NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> first,
                                         @NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> second)
  {
    Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> pair = mixDataSet(first,
                                                                                         second);
    mIndexAndPositions = pair.first;
    mIndexAndViewTypes = pair.second;
  }

  @NonNull
  private static Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> mixDataSet(
      @NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> first,
      @NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> second)

  {
    List<AdapterIndexAndPosition> indexAndPositions = new ArrayList<>();
    List<AdapterIndexAndViewType> indexAndViewTypes = new ArrayList<>();

    int secondAdapterCount = second.getItemCount();
    int firstAdapterCount = first.getItemCount();
    if (secondAdapterCount != firstAdapterCount)
      throw new IllegalArgumentException("firstAdapterCount different from secondAdapterCount");


    for (int i = 0; i < secondAdapterCount; i++)
    {
      indexAndPositions.add(new AdapterIndexAndPositionImpl(FIRST_ADAPTER_INDEX, i));
      indexAndPositions.add(new AdapterIndexAndPositionImpl(SECOND_ADAPTER_INDEX, i));

      AdapterIndexAndViewType viewTypeFirst = new AdapterIndexAndViewTypeImpl(FIRST_ADAPTER_INDEX, first.getItemViewType(i));
      AdapterIndexAndViewType viewTypeSecond = new AdapterIndexAndViewTypeImpl(SECOND_ADAPTER_INDEX, second.getItemViewType(i));

      if (!indexAndViewTypes.contains(viewTypeFirst))
        indexAndViewTypes.add(viewTypeFirst);

      if (!indexAndViewTypes.contains(viewTypeSecond))
        indexAndViewTypes.add(viewTypeSecond);
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
