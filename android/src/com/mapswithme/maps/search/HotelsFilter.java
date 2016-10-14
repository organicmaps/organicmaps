package com.mapswithme.maps.search;

import android.support.annotation.NonNull;

public class HotelsFilter
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
  }

  public static class RatingFilter extends Op
  {
    public final float mValue;

    public RatingFilter(int op, float value)
    {
      super(FIELD_RATING, op);
      mValue = value;
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
  }
}
