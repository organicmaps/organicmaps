package com.mapswithme.maps.ugc;

import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmFragment;

import java.util.ArrayList;
import java.util.List;

public class UGCEditorFragment extends BaseMwmFragment
{
  @NonNull
  private final UGCRatingAdapter mUGCRatingAdapter = new UGCRatingAdapter();

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    View root = inflater.inflate(R.layout.fragment_ugc_editor, container, false);
    RecyclerView rvRatingView = (RecyclerView) root.findViewById(R.id.rv__ratings);
    rvRatingView.setLayoutManager(new LinearLayoutManager(getContext()));
    rvRatingView.getLayoutManager().setAutoMeasureEnabled(true);
    rvRatingView.setNestedScrollingEnabled(false);
    rvRatingView.setHasFixedSize(false);
    rvRatingView.setAdapter(mUGCRatingAdapter);

    //TODO: use parcelable instead of seriliazable
    UGC ugc = (UGC) getActivity().getIntent().getSerializableExtra(UGCEditorActivity.EXTRA_UGC);
    List<UGC.Rating> avgRatings = new ArrayList<>(ugc.getRatings());
    for (UGC.Rating rating: avgRatings)
      rating.setValue(getActivity().getIntent().getIntExtra(UGCEditorActivity.EXTRA_AVG_RATING, 3));
    mUGCRatingAdapter.setItems(avgRatings);

    View submit = root.findViewById(R.id.submit);
    submit.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        getActivity().finish();
      }
    });
    return root;


  }
}
