package com.mapswithme.maps.ugc;

import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.view.View;

import com.mapswithme.maps.R;

public class UgcAverageRatingController implements View.OnClickListener
{
  @Nullable
  private final OnUgcRatingChangedListener mListener;

  public UgcAverageRatingController(@NonNull View view, @Nullable OnUgcRatingChangedListener listener)
  {
    mListener = listener;
    view.findViewById(R.id.iv__horrible).setOnClickListener(this);
    view.findViewById(R.id.iv__bad).setOnClickListener(this);
    view.findViewById(R.id.iv__normal).setOnClickListener(this);
    view.findViewById(R.id.iv__good).setOnClickListener(this);
    view.findViewById(R.id.iv__excellent).setOnClickListener(this);

  }

  @Override
  public void onClick(View v)
  {
    if (mListener == null)
      return;

    switch (v.getId()){
      case R.id.iv__horrible:
        mListener.onRatingChanged(UGC.RATING_HORRIBLE);
        break;
      case R.id.iv__bad:
        mListener.onRatingChanged(UGC.RATING_BAD);
        break;
      case R.id.iv__normal:
        mListener.onRatingChanged(UGC.RATING_NORMAL);
        break;
      case R.id.iv__good:
        mListener.onRatingChanged(UGC.RATING_GOOD);
        break;
      case R.id.iv__excellent:
        mListener.onRatingChanged(UGC.RATING_EXCELLENT);
        break;
      default:
        throw new AssertionError("Unknown rating view:");
    }
  }

  public interface OnUgcRatingChangedListener
  {
    void onRatingChanged(@UGC.UGCRating int rating);
  }
}
