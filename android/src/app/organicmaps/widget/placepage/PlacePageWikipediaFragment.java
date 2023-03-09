package app.organicmaps.widget.placepage;

import android.os.Bundle;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Metadata;
import app.organicmaps.util.Utils;

public class PlacePageWikipediaFragment extends Fragment implements Observer<MapObject>
{
  private View mFrame;
  private View mWiki;

  private TextView mPlaceDescriptionView;

  private PlacePageViewModel mViewModel;

  private MapObject mMapObject;

  private int mDescriptionMaxLength;

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_wikipedia_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mDescriptionMaxLength = getResources().getInteger(R.integer.place_page_description_max_length);

    mFrame = view;

    mPlaceDescriptionView = view.findViewById(R.id.poi_description);
    View placeDescriptionMoreBtn = view.findViewById(R.id.more_btn);
    placeDescriptionMoreBtn.setOnClickListener(v -> showDescriptionScreen());
    mPlaceDescriptionView.setOnClickListener(v -> showDescriptionScreen());
    mWiki = view.findViewById(R.id.ll__place_wiki);
  }

  private void showDescriptionScreen()
  {
    PlaceDescriptionActivity.start(requireContext(), mMapObject.getDescription());
  }

  private Spanned getShortDescription()
  {
    String htmlDescription = mMapObject.getDescription();
    final int paragraphStart = htmlDescription.indexOf("<p>");
    final int paragraphEnd = htmlDescription.indexOf("</p>");
    if (paragraphStart == 0 && paragraphEnd != -1)
      htmlDescription = htmlDescription.substring(3, paragraphEnd);

    Spanned description = Utils.fromHtml(htmlDescription);
    if (description.length() > mDescriptionMaxLength)
    {
      description = (Spanned) new SpannableStringBuilder(description)
          .insert(mDescriptionMaxLength - 3, "...")
          .subSequence(0, mDescriptionMaxLength);
    }

    return description;
  }

  private void updateViews()
  {
    mPlaceDescriptionView.setText(getShortDescription());
    mPlaceDescriptionView.setOnLongClickListener((v) -> {
      PlacePageUtils.copyToClipboard(requireContext(), mFrame, mPlaceDescriptionView.getText()
                                                                                    .toString());
      return true;
    });
    final String wikipediaLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA);
    mWiki.setOnClickListener((v) -> Utils.openUrl(requireContext(), wikipediaLink));
    mWiki.setOnLongClickListener((v) -> {
      PlacePageUtils.copyToClipboard(requireContext(), mFrame, wikipediaLink);
      return true;
    });
  }

  @Override
  public void onResume()
  {
    super.onResume();
    mViewModel.getMapObject().observe(requireActivity(), this);
  }

  @Override
  public void onPause()
  {
    super.onPause();
    mViewModel.getMapObject().removeObserver(this);
  }

  @Override
  public void onChanged(MapObject mapObject)
  {
    mMapObject = mapObject;
    updateViews();
  }
}
