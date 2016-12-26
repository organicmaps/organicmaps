package com.mapswithme.maps.search;

import android.os.Parcel;
import android.os.Parcelable;
import android.support.annotation.NonNull;

import static com.mapswithme.maps.search.HotelsFilter.Op.FIELD_RATING;

public class HotelsFilter implements Parcelable
{
  // *NOTE* keep this in sync with JNI counterpart.
  public final static int TYPE_AND = 0;
  public final static int TYPE_OR = 1;
  public final static int TYPE_OP = 2;

  public final int mType;

  protected HotelsFilter(int type)
  {
    mType = type;
  }

  public static class And extends HotelsFilter
  {
    @NonNull
    public final HotelsFilter mLhs;
    @NonNull
    public final HotelsFilter mRhs;

    public And(@NonNull HotelsFilter lhs, @NonNull HotelsFilter rhs)
    {
      super(TYPE_AND);
      mLhs = lhs;
      mRhs = rhs;
    }

    public And(Parcel source)
    {
      super(TYPE_AND);
      mLhs = source.readParcelable(HotelsFilter.class.getClassLoader());
      mRhs = source.readParcelable(HotelsFilter.class.getClassLoader());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeParcelable(mLhs, flags);
      dest.writeParcelable(mRhs, flags);
    }
  }

  public static class Or extends HotelsFilter
  {
    @NonNull
    public final HotelsFilter mLhs;
    @NonNull
    public final HotelsFilter mRhs;

    public Or(@NonNull HotelsFilter lhs, @NonNull HotelsFilter rhs)
    {
      super(TYPE_OR);
      mLhs = lhs;
      mRhs = rhs;
    }

    public Or(Parcel source)
    {
      super(TYPE_OR);
      mLhs = source.readParcelable(HotelsFilter.class.getClassLoader());
      mRhs = source.readParcelable(HotelsFilter.class.getClassLoader());
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeParcelable(mLhs, flags);
      dest.writeParcelable(mRhs, flags);
    }
  }

  public static class Op extends HotelsFilter
  {
    // *NOTE* keep this in sync with JNI counterpart.
    public final static int FIELD_RATING = 0;
    public final static int FIELD_PRICE_RATE = 1;

    // *NOTE* keep this in sync with JNI counterpart.
    public final static int OP_LT = 0;
    public final static int OP_LE = 1;
    public final static int OP_GT = 2;
    public final static int OP_GE = 3;
    public final static int OP_EQ = 4;

    public final int mField;
    public final int mOp;

    protected Op(int field, int op)
    {
      super(TYPE_OP);
      mField = field;
      mOp = op;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeInt(mField);
      dest.writeInt(mOp);
    }
  }

  public static class RatingFilter extends Op
  {
    public final float mValue;

    public RatingFilter(int op, float value)
    {
      super(FIELD_RATING, op);
      mValue = value;
    }

    public RatingFilter(Parcel source)
    {
      super(FIELD_RATING, source.readInt());
      mValue = source.readFloat();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeFloat(mValue);
    }
  }

  public static class PriceRateFilter extends Op
  {
    public final int mValue;

    public PriceRateFilter(int op, int value)
    {
      super(FIELD_PRICE_RATE, op);
      mValue = value;
    }

    public PriceRateFilter(Parcel source)
    {
      super(FIELD_PRICE_RATE, source.readInt());
      mValue = source.readInt();
    }

    @Override
    public void writeToParcel(Parcel dest, int flags)
    {
      super.writeToParcel(dest, flags);
      dest.writeInt(mValue);
    }
  }

  protected HotelsFilter(Parcel in)
  {
    mType = in.readInt();
  }

  @Override
  public void writeToParcel(Parcel dest, int flags)
  {
    dest.writeInt(mType);
  }

  @Override
  public int describeContents()
  {
    return 0;
  }

  private static HotelsFilter readFromParcel(Parcel source)
  {
    int type = source.readInt();
    if (type == TYPE_AND)
      return new And(source);

    if (type == TYPE_OR)
      return new Or(source);

    int field = source.readInt();
    if (field == FIELD_RATING)
      return new RatingFilter(source);

    return new PriceRateFilter(source);
  }

  public static final Creator<HotelsFilter> CREATOR = new Creator<HotelsFilter>()
  {
    @Override
    public HotelsFilter createFromParcel(Parcel in)
    {
      return readFromParcel(in);
    }

    @Override
    public HotelsFilter[] newArray(int size)
    {
      return new HotelsFilter[size];
    }
  };
}
