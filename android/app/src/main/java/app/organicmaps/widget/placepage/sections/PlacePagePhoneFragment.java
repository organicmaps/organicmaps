package app.organicmaps.widget.placepage.sections;

import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.R;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.widget.placepage.PlacePageViewModel;

public class PlacePagePhoneFragment extends Fragment implements Observer<MapObject>
{
  private PlacePhoneAdapter mPhoneAdapter;

  private PlacePageViewModel mViewModel;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_phone_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    RecyclerView phoneRecycler = view.findViewById(R.id.rw__phone);
    mPhoneAdapter = new PlacePhoneAdapter();
    phoneRecycler.setAdapter(mPhoneAdapter);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject != null)
      mPhoneAdapter.refreshPhones(mapObject.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
  }
}
