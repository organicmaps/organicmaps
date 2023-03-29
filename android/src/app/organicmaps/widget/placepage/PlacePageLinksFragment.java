package app.organicmaps.widget.placepage;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.Framework;
import app.organicmaps.R;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Metadata;
import app.organicmaps.util.Utils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;

public class PlacePageLinksFragment extends Fragment implements Observer<MapObject>
{
  static final List<Metadata.MetadataType> supportedLinks = Arrays.asList(
      Metadata.MetadataType.FMD_WEBSITE,
      Metadata.MetadataType.FMD_EMAIL,
      Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS,
      Metadata.MetadataType.FMD_CONTACT_FACEBOOK,
      Metadata.MetadataType.FMD_CONTACT_INSTAGRAM,
      Metadata.MetadataType.FMD_CONTACT_TWITTER,
      Metadata.MetadataType.FMD_CONTACT_VK,
      Metadata.MetadataType.FMD_CONTACT_LINE);

  private View mFrame;
  private View mFacebookPage;
  private TextView mTvFacebookPage;
  private View mInstagramPage;
  private TextView mTvInstagramPage;
  private View mTwitterPage;
  private TextView mTvTwitterPage;
  private View mVkPage;
  private TextView mTvVkPage;
  private View mLinePage;
  private TextView mTvLinePage;

  private View mWebsite;
  private TextView mTvWebsite;
  private View mEmail;
  private TextView mTvEmail;
  private View mWikimedia;
  private TextView mTvWikimedia;

  private PlacePageViewModel mViewModel;
  private MapObject mMapObject;

  private static void refreshMetadataOrHide(String metadata, View metaLayout, TextView metaTv)
  {
    if (!TextUtils.isEmpty(metadata))
    {
      metaLayout.setVisibility(VISIBLE);
      if (metaTv != null)
        metaTv.setText(metadata);
    }
    else
      metaLayout.setVisibility(GONE);
  }

  public static boolean hasLinkAvailable(MapObject mapObject)
  {
    for (Metadata.MetadataType type: supportedLinks) {
      final String metadata = type == Metadata.MetadataType.FMD_WEBSITE ? getWebsiteUrl(mapObject) : mapObject.getMetadata(type);
      if (!TextUtils.isEmpty(metadata))
        return true;
    }
    return false;
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    return inflater.inflate(R.layout.place_page_links_fragment, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mFrame = view;

    mWebsite = mFrame.findViewById(R.id.ll__place_website);
    mTvWebsite = mFrame.findViewById(R.id.tv__place_website);
    mWebsite.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_WEBSITE));
    mWebsite.setOnLongClickListener((v) -> copyUrl(mWebsite, Metadata.MetadataType.FMD_WEBSITE));

    mEmail = mFrame.findViewById(R.id.ll__place_email);
    mTvEmail = mFrame.findViewById(R.id.tv__place_email);
    mEmail.setOnClickListener((v) -> Utils.sendTo(requireContext(), getLink(Metadata.MetadataType.FMD_EMAIL)));
    mEmail.setOnLongClickListener((v) -> copyUrl(mEmail, Metadata.MetadataType.FMD_EMAIL));

    mWikimedia = mFrame.findViewById(R.id.ll__place_wikimedia);
    mTvWikimedia = mFrame.findViewById(R.id.tv__place_wikimedia);
    mWikimedia.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));
    mWikimedia.setOnLongClickListener((v) -> copyUrl(mWikimedia, Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));

    mFacebookPage = mFrame.findViewById(R.id.ll__place_facebook);
    mTvFacebookPage = mFrame.findViewById(R.id.tv__place_facebook_page);
    mFacebookPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_FACEBOOK));
    mFacebookPage.setOnLongClickListener((v) -> copyUrl(mFacebookPage, Metadata.MetadataType.FMD_CONTACT_FACEBOOK));

    mInstagramPage = mFrame.findViewById(R.id.ll__place_instagram);
    mTvInstagramPage = mFrame.findViewById(R.id.tv__place_instagram_page);
    mInstagramPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM));
    mInstagramPage.setOnLongClickListener((v) -> copyUrl(mInstagramPage, Metadata.MetadataType.FMD_CONTACT_INSTAGRAM));

    mTwitterPage = mFrame.findViewById(R.id.ll__place_twitter);
    mTvTwitterPage = mFrame.findViewById(R.id.tv__place_twitter_page);
    mTwitterPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_TWITTER));
    mTwitterPage.setOnLongClickListener((v) -> copyUrl(mTwitterPage, Metadata.MetadataType.FMD_CONTACT_TWITTER));

    mVkPage = mFrame.findViewById(R.id.ll__place_vk);
    mTvVkPage = mFrame.findViewById(R.id.tv__place_vk_page);
    mVkPage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_VK));
    mVkPage.setOnLongClickListener((v) -> copyUrl(mVkPage, Metadata.MetadataType.FMD_CONTACT_VK));

    mLinePage = mFrame.findViewById(R.id.ll__place_line);
    mTvLinePage = mFrame.findViewById(R.id.tv__place_line_page);
    mLinePage.setOnClickListener((v) -> openUrl(Metadata.MetadataType.FMD_CONTACT_LINE));
    mLinePage.setOnLongClickListener((v) -> copyUrl(mLinePage, Metadata.MetadataType.FMD_CONTACT_LINE));
  }

  private boolean isSocialUsername(Metadata.MetadataType type)
  {
    return !mMapObject.getMetadata(type).contains("/");
  }

  private void openUrl(Metadata.MetadataType type)
  {
    final String url = getLink(type);
    if (!TextUtils.isEmpty(url))
      Utils.openUrl(requireContext(), url);
  }

  private String getLink(Metadata.MetadataType type)
  {
    final String metadata = mMapObject.getMetadata(type);
    if (TextUtils.isEmpty(metadata))
      return "";

    switch (type)
    {
      case FMD_WEBSITE:
        return getWebsiteUrl(mMapObject);
      case FMD_CONTACT_FACEBOOK:
      case FMD_CONTACT_INSTAGRAM:
      case FMD_CONTACT_TWITTER:
      case FMD_CONTACT_VK:
      case FMD_CONTACT_LINE:
        return Framework.nativeGetPoiContactUrl(type.toInt());
      default:
        return metadata;
    }
  }

  private boolean copyUrl(View view, Metadata.MetadataType type)
  {
    final String url = getLink(type);
    final List<String> items = new ArrayList<>();
    items.add(url);

    final String metadata = type == Metadata.MetadataType.FMD_WEBSITE ? getWebsiteUrl(mMapObject) : mMapObject.getMetadata(type);
    // Add user names for social media if available
    if (!metadata.equals(url) && isSocialUsername(type))
      items.add(metadata);

    if (items.size() == 1)
      PlacePageUtils.copyToClipboard(requireContext(), mFrame, items.get(0));
    else
      PlacePageUtils.showCopyPopup(requireContext(), view, mFrame, items);
    return true;
  }

  private void refreshSocialPageLink(@NonNull MapObject mapObject, View view, TextView tvSocialPage, Metadata.MetadataType metaType)
  {
    final String socialPage = mapObject.getMetadata(metaType);
    refreshMetadataOrHide(socialPage, view, tvSocialPage);
  }

  private static String getWebsiteUrl(MapObject mapObject)
  {
    String website = mapObject.getMetadata(Metadata.MetadataType.FMD_WEBSITE);
    String url = mapObject.getMetadata(Metadata.MetadataType.FMD_URL);
    return TextUtils.isEmpty(website) ? url : website;
  }

  private void refreshLinks()
  {
    refreshMetadataOrHide(getWebsiteUrl(mMapObject), mWebsite, mTvWebsite);
    String wikimedia_commons = mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS);
    String wikimedia_commons_text = TextUtils.isEmpty(wikimedia_commons) ? "" : getResources().getString(R.string.wikimedia_commons);
    refreshMetadataOrHide(wikimedia_commons_text, mWikimedia, mTvWikimedia);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);

    refreshSocialPageLink(mMapObject, mFacebookPage, mTvFacebookPage, Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
    refreshSocialPageLink(mMapObject, mInstagramPage, mTvInstagramPage, Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
    refreshSocialPageLink(mMapObject, mTwitterPage, mTvTwitterPage, Metadata.MetadataType.FMD_CONTACT_TWITTER);
    refreshSocialPageLink(mMapObject, mVkPage, mTvVkPage, Metadata.MetadataType.FMD_CONTACT_VK);
    refreshSocialPageLink(mMapObject, mLinePage, mTvLinePage, Metadata.MetadataType.FMD_CONTACT_LINE);
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
    refreshLinks();
  }
}
