package com.mapswithme.maps.ugc.routes;

import android.support.annotation.NonNull;
import android.support.v7.widget.RecyclerView;
import android.util.Pair;
import android.view.ViewGroup;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public class RecyclerCompositeAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder>
{
  private final List<RecyclerView.Adapter<? extends RecyclerView.ViewHolder>> mAdapters = new ArrayList<>();
  private final AdapterIndexConverter mIndexConverter;

  public RecyclerCompositeAdapter(AdapterIndexConverter indexConverter,
                                  RecyclerView.Adapter<? extends RecyclerView.ViewHolder>... adapters)
  {
    mIndexConverter = indexConverter;
    mAdapters.addAll(Arrays.asList(adapters));
  }

  @Override
  public int getItemCount()
  {
    int total = 0;
    for (RecyclerView.Adapter<? extends RecyclerView.ViewHolder> each : mAdapters)
    {
      total += each.getItemCount();
    }

    return total;
  }

  @Override
  public RecyclerView.ViewHolder onCreateViewHolder(ViewGroup parent, int viewType)
  {
    AdapterIndexAndViewType entity = mIndexConverter.getRelativeViewType(viewType);
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(entity.getIndex());
    return adapter.onCreateViewHolder(parent, entity.getRelativeViewType());
  }

  @Override
  public int getItemViewType(int position)
  {
    AdapterIndexAndPosition index = mIndexConverter.getRelativePosition(position);
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(index.getIndex());

    int relativeViewType = adapter.getItemViewType(index.getRelativePosition());
    AdapterIndexAndViewTypeImpl type = new AdapterIndexAndViewTypeImpl(index.getIndex(), relativeViewType);
    int absoluteViewType = mIndexConverter.toAbsoluteViewType(type);

    return absoluteViewType;
  }

  @Override
  public void onBindViewHolder(RecyclerView.ViewHolder holder, int position)
  {
    AdapterIndexAndPosition index = mIndexConverter.getRelativePosition(position);
    RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter = mAdapters.get(index.getIndex());
    bindViewHolder(holder, index.getRelativePosition(), adapter);
  }

  private <Holder extends RecyclerView.ViewHolder> void bindViewHolder(Holder holder, int position,
                                                                       RecyclerView.Adapter<? extends RecyclerView.ViewHolder> adapter)
  {
    ((RecyclerView.Adapter<Holder>)adapter).onBindViewHolder(holder, position);
  }

  public static abstract class AbstractAdapterIndexConverter implements AdapterIndexConverter
  {
    @Override
    public AdapterIndexAndPosition getRelativePosition(int absPosition)
    {
      return getIndexAndPositions().get(absPosition);
    }

    @Override
    public AdapterIndexAndViewType getRelativeViewType(int absViewType)
    {
      return getIndexAndViewTypes().get(absViewType);
    }

    @Override
    public int toAbsoluteViewType(AdapterIndexAndViewType type)
    {
      int indexOf = getIndexAndViewTypes().indexOf(type);
      if (indexOf < 0)
      {
        throw new IllegalArgumentException("Item " + type + " not found in list : " + Arrays.toString(getIndexAndViewTypes().toArray()));
      }
      return indexOf;
    }

    protected abstract List<AdapterIndexAndViewType> getIndexAndViewTypes();
    protected abstract List<AdapterIndexAndPosition> getIndexAndPositions();
  }

  public static final class RepeatablePairIndexConverter extends AbstractAdapterIndexConverter
  {
    private static final int FIRST_ADAPTER_INDEX = 0;
    private static final int SECOND_ADAPTER_INDEX = 1;

    private final List<AdapterIndexAndPosition> mIndexAndPositions;
    private final List<AdapterIndexAndViewType> mIndexAndViewTypes;

    public RepeatablePairIndexConverter(@NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> first,
                                        @NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> second)
    {
      Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> pair = mixDataSet(first,
                                                                                           second);
      mIndexAndPositions = pair.first;
      mIndexAndViewTypes = pair.second;
    }

    @NonNull
    private Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> mixDataSet(
        @NonNull RecyclerView.Adapter<? extends RecyclerView.ViewHolder> first,
        @NonNull RecyclerView.Adapter<?extends RecyclerView.ViewHolder> second)

    {
      List<AdapterIndexAndPosition> indexAndPositions = new ArrayList<>();
      List<AdapterIndexAndViewType> indexAndViewTypes = new ArrayList<>();

      int tagsListsCount = second.getItemCount();
      int categoriesListCount = first.getItemCount();
      if (tagsListsCount != categoriesListCount)
        throw new IllegalArgumentException("tagsListsCount different from categoriesListCount");


      for (int i = 0; i < tagsListsCount; i++)
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

    @Override
    protected List<AdapterIndexAndViewType> getIndexAndViewTypes()
    {
      return mIndexAndViewTypes;
    }

    @Override
    protected List<AdapterIndexAndPosition> getIndexAndPositions()
    {
      return mIndexAndPositions;
    }
  }

  public static final class DefaultIndexConverter extends AbstractAdapterIndexConverter
  {
    private final List<AdapterIndexAndPosition> mIndexAndPositions;
    private final List<AdapterIndexAndViewType> mIndexAndViewTypes;

    public DefaultIndexConverter(List<RecyclerView.Adapter<RecyclerView.ViewHolder>> adapters)
    {
      Pair<List<AdapterIndexAndPosition>, List<AdapterIndexAndViewType>> pair = makeDataSet(adapters);
      mIndexAndPositions = pair.first;
      mIndexAndViewTypes = pair.second;
    }

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

    @Override
    protected List<AdapterIndexAndViewType> getIndexAndViewTypes()
    {
      return mIndexAndViewTypes;
    }

    @Override
    protected List<AdapterIndexAndPosition> getIndexAndPositions()
    {
      return mIndexAndPositions;
    }
  }

  static final class AdapterIndexAndPositionImpl implements AdapterIndexAndPosition
  {
    private final int mIndex;
    private final int mRelativePosition;

    private AdapterIndexAndPositionImpl(int index, int relativePosition)
    {
      mIndex = index;
      mRelativePosition = relativePosition;
    }

    @Override
    public int getRelativePosition()
    {
      return mRelativePosition;
    }

    @Override
    public int getIndex()
    {
      return mIndex;
    }

    @Override
    public String toString()
    {
      final StringBuilder sb = new StringBuilder("AdapterIndexAndPositionImpl{");
      sb.append("mIndex=").append(mIndex);
      sb.append(", mRelativePosition=").append(mRelativePosition);
      sb.append('}');
      return sb.toString();
    }
  }

  static final class AdapterIndexAndViewTypeImpl implements AdapterIndexAndViewType
  {
    private final int mIndex;
    private final int mViewType;

    private AdapterIndexAndViewTypeImpl(int index, int viewType)
    {
      mIndex = index;
      mViewType = viewType;
    }

    @Override
    public int getRelativeViewType()
    {
      return mViewType;
    }

    @Override
    public int getIndex()
    {
      return mIndex;
    }

    @Override
    public boolean equals(Object o)
    {
      if (this == o) return true;
      if (o == null || getClass() != o.getClass()) return false;

      AdapterIndexAndViewTypeImpl that = (AdapterIndexAndViewTypeImpl) o;

      if (mIndex != that.mIndex) return false;
      return mViewType == that.mViewType;
    }

    @Override
    public int hashCode()
    {
      int result = mIndex;
      result = 31 * result + mViewType;
      return result;
    }

    @Override
    public String toString()
    {
      final StringBuilder sb = new StringBuilder("AdapterIndexAndViewTypeImpl{");
      sb.append("mIndex=").append(mIndex);
      sb.append(", mViewType=").append(mViewType);
      sb.append('}');
      return sb.toString();
    }
  }


}
