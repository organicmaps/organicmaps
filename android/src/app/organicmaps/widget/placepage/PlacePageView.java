package app.organicmaps.widget.placepage;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.location.Location;
import android.os.Bundle;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.Framework;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.api.Const;
import app.organicmaps.api.ParsedMwmRequest;
import app.organicmaps.bookmarks.data.BookmarkManager;
import app.organicmaps.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.bookmarks.data.MapObject;
import app.organicmaps.bookmarks.data.Metadata;
import app.organicmaps.bookmarks.data.RoadWarningMarkType;
import app.organicmaps.downloader.CountryItem;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.downloader.MapManager;
import app.organicmaps.editor.Editor;
import app.organicmaps.editor.OpeningHours;
import app.organicmaps.editor.data.Timetable;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.search.Popularity;
import app.organicmaps.settings.RoadType;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.concurrency.UiThread;
import app.organicmaps.widget.ArrowView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;

public class PlacePageView extends Fragment implements View.OnClickListener,
                                                       View.OnLongClickListener,
                                                       PlacePageButtons.PlacePageButtonClickListener,
                                                       LocationListener,
                                                       Observer<MapObject>

{
  private static final String TAG = PlacePageView.class.getSimpleName();

  private static final String PREF_COORDINATES_FORMAT = "coordinates_format";
  private static final String BOOKMARK_FRAGMENT_TAG = "BOOKMARK_FRAGMENT_TAG";
  private static final String WIKIPEDIA_FRAGMENT_TAG = "WIKIPEDIA_FRAGMENT_TAG";
  private static final String PHONE_FRAGMENT_TAG = "PHONE_FRAGMENT_TAG";
  private static final String OPENING_HOURS_FRAGMENT_TAG = "OPENING_HOURS_FRAGMENT_TAG";

  private static final List<CoordinatesFormat> visibleCoordsFormat =
      Arrays.asList(CoordinatesFormat.LatLonDMS,
                    CoordinatesFormat.LatLonDecimal,
                    CoordinatesFormat.OLCFull,
                    CoordinatesFormat.OSMLink);
  private View mFrame;
  // Preview.
  private ViewGroup mPreview;
  private Toolbar mToolbar;
  private TextView mTvTitle;
  private TextView mTvSecondaryTitle;
  private TextView mTvSubtitle;
  private ArrowView mAvDirection;
  private TextView mTvDistance;
  private TextView mTvAddress;
  // Details.
  private ViewGroup mDetails;
  private View mWebsite;
  private TextView mTvWebsite;
  private TextView mTvLatlon;
  //Social links
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
  private View mWifi;
  private TextView mTvWiFi;
  private View mEmail;
  private TextView mTvEmail;
  private View mWikimedia;
  private TextView mTvWikimedia;
  private View mOperator;
  private TextView mTvOperator;
  private View mLevel;
  private TextView mTvLevel;
  private View mCuisine;
  private TextView mTvCuisine;
  private View mEntrance;
  private TextView mTvEntrance;
  private View mEditPlace;
  private View mAddOrganisation;
  private View mAddPlace;
  private View mEditTopSpace;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPopularityView;
  // Data
  private CoordinatesFormat mCoordsFormat = CoordinatesFormat.LatLonDecimal;
  // Downloader`s stuff
  private DownloaderStatusIcon mDownloaderIcon;
  private TextView mDownloaderInfo;
  private int mStorageCallbackSlot;
  @Nullable
  private CountryItem mCurrentCountry;
  private final Runnable mDownloaderDeferredDetachProc = new Runnable()
  {
    @Override
    public void run()
    {
      detachCountry();
    }
  };
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback()
  {
    @Override
    public void onStatusChanged(List<MapManager.StorageCallbackData> data)
    {
      if (mCurrentCountry == null)
        return;

      for (MapManager.StorageCallbackData item : data)
        if (mCurrentCountry.id.equals(item.countryId))
        {
          updateDownloader();
          return;
        }
    }

    @Override
    public void onProgress(String countryId, long localSize, long remoteSize)
    {
      if (mCurrentCountry != null && mCurrentCountry.id.equals(countryId))
        updateDownloader();
    }
  };
  @NonNull
  private final View.OnClickListener mDownloadClickListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.warn3gAndDownload(requireActivity(), mCurrentCountry.id, null);
    }
  };
  @NonNull
  private final View.OnClickListener mCancelDownloadListener = new View.OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.nativeCancel(mCurrentCountry.id);
    }
  };

  private PlacePageViewListener mPlacePageViewListener;

  private PlacePageViewModel viewModel;
  private MapObject mMapObject;

  @NonNull
  private static PlacePageButtons.ButtonType toPlacePageButton(@NonNull RoadWarningMarkType type)
  {
    switch (type)
    {
      case DIRTY:
        return PlacePageButtons.ButtonType.ROUTE_AVOID_UNPAVED;
      case FERRY:
        return PlacePageButtons.ButtonType.ROUTE_AVOID_FERRY;
      case TOLL:
        return PlacePageButtons.ButtonType.ROUTE_AVOID_TOLL;
      default:
        throw new AssertionError("Unsupported road warning type: " + type);
    }
  }

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

  private static boolean isInvalidDownloaderStatus(int status)
  {
    return (status != CountryItem.STATUS_DOWNLOADABLE &&
            status != CountryItem.STATUS_ENQUEUED &&
            status != CountryItem.STATUS_FAILED &&
            status != CountryItem.STATUS_PARTLY &&
            status != CountryItem.STATUS_PROGRESS &&
            status != CountryItem.STATUS_APPLYING);
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.place_page, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mCoordsFormat = CoordinatesFormat.fromId(
        MwmApplication.prefs(requireContext()).getInt(
            PREF_COORDINATES_FORMAT, CoordinatesFormat.LatLonDecimal.getId()));

    mFrame = view;
    mFrame.setOnClickListener((v) -> mPlacePageViewListener.onPlacePageRequestToggleState());

    mPreview = mFrame.findViewById(R.id.pp__preview);
    mFrame.addOnLayoutChangeListener((v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
      final int oldHeight = oldBottom - oldTop;
      final int newHeight = bottom - top;
      if (oldHeight != newHeight)
        mPlacePageViewListener.onPlacePageContentChanged(mPreview.getHeight(), newHeight);
    });

    mTvTitle = mPreview.findViewById(R.id.tv__title);
    mTvTitle.setOnLongClickListener(this);
    mTvTitle.setOnClickListener(this);
    mTvSecondaryTitle = mPreview.findViewById(R.id.tv__secondary_title);
    mTvSecondaryTitle.setOnLongClickListener(this);
    mTvSecondaryTitle.setOnClickListener(this);
    mToolbar = mFrame.findViewById(R.id.toolbar);
    mTvSubtitle = mPreview.findViewById(R.id.tv__subtitle);

    View directionFrame = mPreview.findViewById(R.id.direction_frame);
    mTvDistance = mPreview.findViewById(R.id.tv__straight_distance);
    mAvDirection = mPreview.findViewById(R.id.av__direction);
    UiUtils.hide(mTvDistance);
    UiUtils.hide(mAvDirection);
    directionFrame.setOnClickListener(this);

    mTvAddress = mPreview.findViewById(R.id.tv__address);
    mTvAddress.setOnLongClickListener(this);
    mTvAddress.setOnClickListener(this);

    mDetails = mFrame.findViewById(R.id.pp__details_frame);
    RelativeLayout address = mFrame.findViewById(R.id.ll__place_name);
    mWebsite = mFrame.findViewById(R.id.ll__place_website);
    mWebsite.setOnClickListener(this);
    mTvWebsite = mFrame.findViewById(R.id.tv__place_website);
    // Wikimedia Commons link
    mWikimedia = mFrame.findViewById(R.id.ll__place_wikimedia);
    mTvWikimedia = mFrame.findViewById(R.id.tv__place_wikimedia);
    mWikimedia.setOnClickListener(this);
    //Social links
    mFacebookPage = mFrame.findViewById(R.id.ll__place_facebook);
    mFacebookPage.setOnClickListener(this);
    mFacebookPage.setOnLongClickListener(this);
    mTvFacebookPage = mFrame.findViewById(R.id.tv__place_facebook_page);

    mInstagramPage = mFrame.findViewById(R.id.ll__place_instagram);
    mInstagramPage.setOnClickListener(this);
    mInstagramPage.setOnLongClickListener(this);
    mTvInstagramPage = mFrame.findViewById(R.id.tv__place_instagram_page);

    mTwitterPage = mFrame.findViewById(R.id.ll__place_twitter);
    mTwitterPage.setOnClickListener(this);
    mTwitterPage.setOnLongClickListener(this);
    mTvTwitterPage = mFrame.findViewById(R.id.tv__place_twitter_page);

    mVkPage = mFrame.findViewById(R.id.ll__place_vk);
    mVkPage.setOnClickListener(this);
    mVkPage.setOnLongClickListener(this);
    mTvVkPage = mFrame.findViewById(R.id.tv__place_vk_page);

    mLinePage = mFrame.findViewById(R.id.ll__place_line);
    mLinePage.setOnClickListener(this);
    mLinePage.setOnLongClickListener(this);
    mTvLinePage = mFrame.findViewById(R.id.tv__place_line_page);
    LinearLayout latlon = mFrame.findViewById(R.id.ll__place_latlon);
    latlon.setOnClickListener(this);
    mTvLatlon = mFrame.findViewById(R.id.tv__place_latlon);
    mWifi = mFrame.findViewById(R.id.ll__place_wifi);
    mTvWiFi = mFrame.findViewById(R.id.tv__place_wifi);
    mEmail = mFrame.findViewById(R.id.ll__place_email);
    mEmail.setOnClickListener(this);
    mTvEmail = mFrame.findViewById(R.id.tv__place_email);
    mOperator = mFrame.findViewById(R.id.ll__place_operator);
    mOperator.setOnClickListener(this);
    mTvOperator = mFrame.findViewById(R.id.tv__place_operator);
    mLevel = mFrame.findViewById(R.id.ll__place_level);
    mTvLevel = mFrame.findViewById(R.id.tv__place_level);
    mCuisine = mFrame.findViewById(R.id.ll__place_cuisine);
    mTvCuisine = mFrame.findViewById(R.id.tv__place_cuisine);
    mEntrance = mFrame.findViewById(R.id.ll__place_entrance);
    mTvEntrance = mEntrance.findViewById(R.id.tv__place_entrance);
    mEditPlace = mFrame.findViewById(R.id.ll__place_editor);
    mEditPlace.setOnClickListener(this);
    mAddOrganisation = mFrame.findViewById(R.id.ll__add_organisation);
    mAddOrganisation.setOnClickListener(this);
    mAddPlace = mFrame.findViewById(R.id.ll__place_add);
    mAddPlace.setOnClickListener(this);
    mEditTopSpace = mFrame.findViewById(R.id.edit_top_space);
    latlon.setOnLongClickListener(this);
    address.setOnLongClickListener(this);
    mWebsite.setOnLongClickListener(this);
    mWikimedia.setOnLongClickListener(this);
    mEmail.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mLevel.setOnLongClickListener(this);

    mDownloaderIcon = new DownloaderStatusIcon(mPreview.findViewById(R.id.downloader_status_frame));

    mDownloaderInfo = mPreview.findViewById(R.id.tv__downloader_details);

    viewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    viewModel.getMapObject().observe(requireActivity(), this);
    mMapObject = viewModel.getMapObject().getValue();

    LocationHelper.INSTANCE.addListener(this);
  }

  @Override
  public void onAttach(@NonNull Context context)
  {
    super.onAttach(context);
    mPlacePageViewListener = (MwmActivity) context;
  }

  @Override
  public void onDestroy()
  {
    super.onDestroy();
    viewModel.getMapObject().removeObserver(this);
    LocationHelper.INSTANCE.removeListener(this);
  }

  @Override
  public void onPlacePageButtonClick(PlacePageButtons.ButtonType item)
  {
    switch (item)
    {
      case BOOKMARK_SAVE:
      case BOOKMARK_DELETE:
        onBookmarkBtnClicked();
        break;

      case SHARE:
        onShareBtnClicked();
        break;

      case BACK:
        onBackBtnClicked();
        break;

      case ROUTE_FROM:
        onRouteFromBtnClicked();
        break;

      case ROUTE_TO:
        onRouteToBtnClicked();
        break;

      case ROUTE_ADD:
        onRouteAddBtnClicked();
        break;

      case ROUTE_REMOVE:
        onRouteRemoveBtnClicked();
        break;

      case ROUTE_AVOID_TOLL:
        onAvoidTollBtnClicked();
        break;

      case ROUTE_AVOID_UNPAVED:
        onAvoidUnpavedBtnClicked();
        break;

      case ROUTE_AVOID_FERRY:
        onAvoidFerryBtnClicked();
        break;
    }
  }

  private void onBookmarkBtnClicked()
  {
    // No need to call setMapObject here as the native methods will reopen the place page
    if (MapObject.isOfType(MapObject.BOOKMARK, mMapObject))
      Framework.nativeDeleteBookmarkFromMapObject();
    else
      BookmarkManager.INSTANCE.addNewBookmark(mMapObject.getLat(), mMapObject.getLon());
  }

  private void onShareBtnClicked()
  {
    SharingUtils.shareMapObject(requireContext(), mMapObject);
  }

  private void onBackBtnClicked()
  {
    final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
    if (request != null && request.isPickPointMode())
    {
      final Intent result = new Intent();
      result.putExtra(Const.EXTRA_POINT_LAT, mMapObject.getLat())
            .putExtra(Const.EXTRA_POINT_LON, mMapObject.getLon())
            .putExtra(Const.EXTRA_POINT_NAME, mMapObject.getTitle())
            .putExtra(Const.EXTRA_POINT_ID, mMapObject.getApiId())
            .putExtra(Const.EXTRA_ZOOM_LEVEL, Framework.nativeGetDrawScale());
      requireActivity().setResult(Activity.RESULT_OK, result);
      ParsedMwmRequest.setCurrentRequest(null);
    }
    requireActivity().finish();
  }

  private void onRouteFromBtnClicked()
  {
    RoutingController controller = RoutingController.get();
    if (!controller.isPlanning())
    {
      controller.prepare(mMapObject, null);
      mPlacePageViewListener.onPlacePageRequestClose();
    }
    else if (controller.setStartPoint(mMapObject))
    {
      mPlacePageViewListener.onPlacePageRequestClose();
    }
  }

  private void onRouteToBtnClicked()
  {
    if (RoutingController.get().isPlanning())
    {
      RoutingController.get().setEndPoint(mMapObject);
      mPlacePageViewListener.onPlacePageRequestClose();
    }
    else
    {
      ((MwmActivity) requireActivity()).startLocationToPoint(mMapObject);
    }
  }

  private void onRouteAddBtnClicked()
  {
    RoutingController.get().addStop(mMapObject);
  }

  private void onRouteRemoveBtnClicked()
  {
    RoutingController.get().removeStop(mMapObject);
  }

  private void onAvoidUnpavedBtnClicked()
  {
    onAvoidBtnClicked(RoadType.Dirty);
  }

  private void onAvoidFerryBtnClicked()
  {
    onAvoidBtnClicked(RoadType.Ferry);
  }

  private void onAvoidTollBtnClicked()
  {
    onAvoidBtnClicked(RoadType.Toll);
  }

  private void onAvoidBtnClicked(@NonNull RoadType roadType)
  {
    mPlacePageViewListener.onPlacePageRequestToggleRouteSettings(roadType);
  }

  private void setCurrentCountry()
  {
    if (mCurrentCountry != null)
      throw new AssertionError("country should be detached before!");
    String country = MapManager.nativeGetSelectedCountry();
    if (country != null && !RoutingController.get().isNavigating())
      attachCountry(country);
  }

  private void refreshViews()
  {
    refreshPreview();
    refreshDetails();
    refreshViewsInternal();
    updateBookmarkView();
    updatePhoneView();
  }

  private void refreshViewsInternal()
  {
    final Location loc = LocationHelper.INSTANCE.getSavedLocation();
    boolean showBackButton = false;
    boolean showRoutingButton = true;
    switch (mMapObject.getMapObjectType())
    {
      case MapObject.BOOKMARK:
      case MapObject.POI:
      case MapObject.SEARCH:
        refreshDistanceToObject(loc);
        break;
      case MapObject.API_POINT:
        refreshDistanceToObject(loc);
        showBackButton = true;
        break;
      case MapObject.MY_POSITION:
        refreshMyPosition(loc);
        showRoutingButton = false;
        break;
    }
    updateButtons(showBackButton, showRoutingButton);
  }

  private <T extends Fragment> void updateViewFragment(Class<T> controllerClass, String fragmentTag, @IdRes int containerId, boolean enabled)
  {
    final FragmentManager fManager = getChildFragmentManager();
    final T fragment = (T) fManager.findFragmentByTag(fragmentTag);
    if (enabled && fragment == null)
    {
      fManager.beginTransaction()
              .add(containerId, controllerClass, null, fragmentTag)
              .commit();
    }
    else if (!enabled && fragment != null)
    {
      fManager.beginTransaction()
              .remove(fragment)
              .commit();
    }
  }

  private void updateOpeningHoursView()
  {
    final String ohStr = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
    final boolean isEmptyTT = (timetables == null || timetables.length == 0);
    updateViewFragment(PlacePageOpeningHoursFragment.class, OPENING_HOURS_FRAGMENT_TAG, R.id.place_page_opening_hours_fragment, !isEmptyTT);
  }

  private void updatePhoneView()
  {
    updateViewFragment(PlacePagePhoneFragment.class, PHONE_FRAGMENT_TAG, R.id.place_page_phone_fragment, mMapObject.hasPhoneNumber());
  }

  private void updateBookmarkView()
  {
    updateViewFragment(PlacePageBookmarkFragment.class, BOOKMARK_FRAGMENT_TAG, R.id.place_page_bookmark_fragment, mMapObject.getMapObjectType() == MapObject.BOOKMARK);
  }

  private void updateWikipediaView()
  {
    updateViewFragment(PlacePageWikipediaFragment.class, WIKIPEDIA_FRAGMENT_TAG, R.id.place_page_wikipedia_fragment, !TextUtils.isEmpty(mMapObject.getDescription()));
  }

  private void setTextAndColorizeSubtitle()
  {
    String text = mMapObject.getSubtitle();
    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, text);
    if (!TextUtils.isEmpty(text))
    {
      SpannableStringBuilder sb = new SpannableStringBuilder(text);
      int start = text.indexOf("★");
      int end = text.lastIndexOf("★") + 1;
      if (start > -1)
      {
        sb.setSpan(new ForegroundColorSpan(getResources().getColor(R.color.base_yellow)),
                   start, end, Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
      }
      mTvSubtitle.setText(sb);
    }
  }

  private void refreshPreview()
  {
    UiUtils.setTextAndHideIfEmpty(mTvTitle, mMapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSecondaryTitle, mMapObject.getSecondaryTitle());
    boolean isPopular = mMapObject.getPopularity().getType() == Popularity.Type.POPULAR;
    if (mPopularityView != null)
      UiUtils.showIf(isPopular, mPopularityView);
    if (mToolbar != null)
      mToolbar.setTitle(mMapObject.getTitle());
    setTextAndColorizeSubtitle();
    UiUtils.setTextAndHideIfEmpty(mTvAddress, mMapObject.getAddress());
  }

  private void refreshDetails()
  {
    refreshLatLon();

    String website = mMapObject.getMetadata(Metadata.MetadataType.FMD_WEBSITE);
    String url = mMapObject.getMetadata(Metadata.MetadataType.FMD_URL);
    refreshMetadataOrHide(TextUtils.isEmpty(website) ? url : website, mWebsite, mTvWebsite);
    String wikimedia_commons = mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS);
    String wikimedia_commons_text = TextUtils.isEmpty(wikimedia_commons) ? "" : getResources().getString(R.string.wikimedia_commons);
    refreshMetadataOrHide(wikimedia_commons_text, mWikimedia, mTvWikimedia);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR), mOperator, mTvOperator);
    /// @todo I don't like it when we take all data from mapObject, but for cuisines, we should
    /// go into JNI Framework and rely on some "active object".
    refreshMetadataOrHide(Framework.nativeGetActiveObjectFormattedCuisine(), mCuisine, mTvCuisine);
    refreshWiFi();
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    refreshSocialLinks();
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_LEVEL), mLevel, mTvLevel);

//    showTaxiOffer(mapObject);

    if (RoutingController.get().isNavigating() || RoutingController.get().isPlanning())
    {
      UiUtils.hide(mEditPlace, mAddOrganisation, mAddPlace, mEditTopSpace);
    }
    else
    {
      UiUtils.showIf(Editor.nativeShouldShowEditPlace(), mEditPlace);
      UiUtils.showIf(Editor.nativeShouldShowAddBusiness(), mAddOrganisation);
      UiUtils.showIf(Editor.nativeShouldShowAddPlace(), mAddPlace);
      UiUtils.showIf(UiUtils.isVisible(mEditPlace)
                     || UiUtils.isVisible(mAddOrganisation)
                     || UiUtils.isVisible(mAddPlace), mEditTopSpace);
    }
    updateOpeningHoursView();
    updateWikipediaView();
  }

  private void refreshWiFi()
  {
    final String inet = mMapObject.getMetadata(Metadata.MetadataType.FMD_INTERNET);
    if (!TextUtils.isEmpty(inet))
    {
      mWifi.setVisibility(VISIBLE);
      /// @todo Better (but harder) to wrap C++ osm::Internet into Java, instead of comparing with "no".
      mTvWiFi.setText(TextUtils.equals(inet, "no") ? R.string.no_available : R.string.yes_available);
    }
    else
      mWifi.setVisibility(GONE);
  }

  private void refreshSocialLinks()
  {
    final String facebookPageLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
    refreshSocialPageLink(mFacebookPage, mTvFacebookPage, facebookPageLink, "facebook.com");
    final String instagramPageLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
    refreshSocialPageLink(mInstagramPage, mTvInstagramPage, instagramPageLink, "instagram.com");
    final String twitterPageLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
    refreshSocialPageLink(mTwitterPage, mTvTwitterPage, twitterPageLink, "twitter.com");
    final String vkPageLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
    refreshSocialPageLink(mVkPage, mTvVkPage, vkPageLink, "vk.com");
    final String linePageLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
    refreshLinePageLink(mLinePage, mTvLinePage, linePageLink);
  }

  private void refreshSocialPageLink(View view, TextView tvSocialPage, String socialPage, String webDomain)
  {
    if (TextUtils.isEmpty(socialPage))
    {
      view.setVisibility(GONE);
    }
    else
    {
      view.setVisibility(VISIBLE);
      if (socialPage.indexOf('/') >= 0)
        tvSocialPage.setText("https://" + webDomain + "/" + socialPage);
      else
        tvSocialPage.setText("@" + socialPage);
    }
  }

  // Tag `contact:line` could contain urls from domains: line.me, liff.line.me, page.line.me, etc.
  // And `socialPage` should not be prepended with domain, but only with "https://" protocol.
  private void refreshLinePageLink(View view, TextView tvSocialPage, String socialPage)
  {
    if (TextUtils.isEmpty(socialPage))
    {
      view.setVisibility(GONE);
    }
    else
    {
      view.setVisibility(VISIBLE);
      if (socialPage.indexOf('/') >= 0)
        tvSocialPage.setText("https://" + socialPage);
      else
        tvSocialPage.setText("@" + socialPage);
    }
  }

  private void updateBookmarkButton()
  {
    final List<PlacePageButtons.ButtonType> currentButtons = viewModel.getCurrentButtons()
                                                                      .getValue();
    PlacePageButtons.ButtonType oldType = PlacePageButtons.ButtonType.BOOKMARK_DELETE;
    PlacePageButtons.ButtonType newType = PlacePageButtons.ButtonType.BOOKMARK_SAVE;
    if (mMapObject.getMapObjectType() == MapObject.BOOKMARK)
    {
      oldType = PlacePageButtons.ButtonType.BOOKMARK_SAVE;
      newType = PlacePageButtons.ButtonType.BOOKMARK_DELETE;
    }
    if (currentButtons != null)
    {
      final List<PlacePageButtons.ButtonType> newList = new ArrayList<>(currentButtons);
      final int index = newList.indexOf(oldType);
      if (index >= 0)
      {
        newList.set(index, newType);
        viewModel.setCurrentButtons(newList);
      }
    }
  }

  private void updateButtons(boolean showBackButton, boolean showRoutingButton)
  {
    List<PlacePageButtons.ButtonType> buttons = new ArrayList<>();
    if (mMapObject.getRoadWarningMarkType() != RoadWarningMarkType.UNKNOWN)
    {
      RoadWarningMarkType markType = mMapObject.getRoadWarningMarkType();
      PlacePageButtons.ButtonType roadType = toPlacePageButton(markType);
      buttons.add(roadType);
    }
    else if (RoutingController.get().isRoutePoint(mMapObject))
    {
      buttons.add(PlacePageButtons.ButtonType.ROUTE_REMOVE);
    }
    else
    {
      final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
      if (showBackButton || (request != null && request.isPickPointMode()))
        buttons.add(PlacePageButtons.ButtonType.BACK);

      boolean needToShowRoutingButtons = RoutingController.get().isPlanning() || showRoutingButton;

      if (needToShowRoutingButtons)
        buttons.add(PlacePageButtons.ButtonType.ROUTE_FROM);

      // If we can show the add route button, put it in the place of the bookmark button
      // And move the bookmark button at the end
      if (needToShowRoutingButtons && RoutingController.get().isStopPointAllowed())
        buttons.add(PlacePageButtons.ButtonType.ROUTE_ADD);
      else
        buttons.add(mMapObject.getMapObjectType() == MapObject.BOOKMARK
                    ? PlacePageButtons.ButtonType.BOOKMARK_DELETE
                    : PlacePageButtons.ButtonType.BOOKMARK_SAVE);

      if (needToShowRoutingButtons)
      {
        buttons.add(PlacePageButtons.ButtonType.ROUTE_TO);
        if (RoutingController.get().isStopPointAllowed())
          buttons.add(mMapObject.getMapObjectType() == MapObject.BOOKMARK
                      ? PlacePageButtons.ButtonType.BOOKMARK_DELETE
                      : PlacePageButtons.ButtonType.BOOKMARK_SAVE);
      }
      buttons.add(PlacePageButtons.ButtonType.SHARE);
    }
    viewModel.setCurrentButtons(buttons);
  }

  private void refreshMyPosition(Location l)
  {
    UiUtils.hide(mTvDistance);
    UiUtils.hide(mAvDirection);

    if (l == null)
      return;

    final StringBuilder builder = new StringBuilder();
    if (l.hasAltitude())
    {
      double altitude = l.getAltitude();
      builder.append(altitude >= 0 ? "▲" : "▼");
      builder.append(Framework.nativeFormatAltitude(altitude));
    }
    if (l.hasSpeed())
      builder.append("   ")
             .append(Framework.nativeFormatSpeed(l.getSpeed()));
    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, builder.toString());

    mMapObject.setLat(l.getLatitude());
    mMapObject.setLon(l.getLongitude());
    refreshLatLon();
  }

  private void refreshDistanceToObject(Location l)
  {
    UiUtils.showIf(l != null, mTvDistance);
    if (l == null)
      return;

    double lat = mMapObject.getLat();
    double lon = mMapObject.getLon();
    DistanceAndAzimut distanceAndAzimuth =
        Framework.nativeGetDistanceAndAzimuthFromLatLon(lat, lon, l.getLatitude(), l.getLongitude(), 0.0);
    mTvDistance.setText(distanceAndAzimuth.getDistance());
  }

  private void refreshLatLon()
  {
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    final String latLon = Framework.nativeFormatLatLon(lat, lon, mCoordsFormat.getId());
    mTvLatlon.setText(latLon);
  }

  private void addOrganisation()
  {
    ((MwmActivity) requireActivity()).showPositionChooserForEditor(true, false);
  }

  private void addPlace()
  {
    ((MwmActivity) requireActivity()).showPositionChooserForEditor(false, true);
  }

  /// @todo
  /// - Why ll__place_editor and ll__place_latlon check if (mMapObject == null)
  /// - Unify urls processing: fb, twitter, instagram, .. add prefix here while
  /// wiki, website, wikimedia, ... already have full url. Better to make it in the same way and in Core.

  @Override
  public void onClick(View v)
  {
    final Context context = requireContext();
    final int id = v.getId();
    if (id == R.id.tv__title || id == R.id.tv__secondary_title || id == R.id.tv__address)
    {
      // A workaround to make single taps toggle the bottom sheet.
      mPlacePageViewListener.onPlacePageRequestToggleState();
    }
    else if (id == R.id.ll__place_editor)
      ((MwmActivity) requireActivity()).showEditor();
    else if (id == R.id.ll__add_organisation)
      addOrganisation();
    else if (id == R.id.ll__place_add)
      addPlace();
    else if (id == R.id.ll__place_latlon)
    {
      final int formatIndex = visibleCoordsFormat.indexOf(mCoordsFormat);
      mCoordsFormat = visibleCoordsFormat.get((formatIndex + 1) % visibleCoordsFormat.size());
      MwmApplication.prefs(context)
                    .edit()
                    .putInt(PREF_COORDINATES_FORMAT, mCoordsFormat.getId())
                    .apply();
      refreshLatLon();
    }
    else if (id == R.id.ll__place_website)
      Utils.openUrl(context, mTvWebsite.getText().toString());
    else if (id == R.id.ll__place_wikimedia)
      Utils.openUrl(context, mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));
    else if (id == R.id.ll__place_facebook)
    {
      final String facebookPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
      Utils.openUrl(context, "https://m.facebook.com/" + facebookPage);
    }
    else if (id == R.id.ll__place_instagram)
    {
      final String instagramPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
      Utils.openUrl(context, "https://instagram.com/" + instagramPage);
    }
    else if (id == R.id.ll__place_twitter)
    {
      final String twitterPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
      Utils.openUrl(context, "https://mobile.twitter.com/" + twitterPage);
    }
    else if (id == R.id.ll__place_vk)
    {
      final String vkPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
      Utils.openUrl(context, "https://vk.com/" + vkPage);
    }
    else if (id == R.id.ll__place_line)
    {
      final String linePage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
      if (linePage.indexOf('/') >= 0)
        Utils.openUrl(context, "https://" + linePage);
      else
        Utils.openUrl(context, "https://line.me/R/ti/p/@" + linePage);
    }
    else if (id == R.id.direction_frame)
      showBigDirection();
    else if (id == R.id.ll__place_email)
      Utils.sendTo(context, mTvEmail.getText().toString());
  }

  private void showBigDirection()
  {
    final FragmentManager fragmentManager = requireActivity().getSupportFragmentManager();
    final DirectionFragment fragment = (DirectionFragment) fragmentManager.getFragmentFactory()
                                                                          .instantiate(getContext().getClassLoader(), DirectionFragment.class.getName());
    fragment.setMapObject(mMapObject);
    fragment.show(fragmentManager, null);
  }

  /// @todo Unify urls processing (fb, twitter, instagram, ...).
  /// onLongClick behaviour differs even from onClick function several lines above.

  @Override
  public boolean onLongClick(View v)
  {
    final List<String> items = new ArrayList<>();
    final int id = v.getId();
    if (id == R.id.tv__title)
      items.add(mTvTitle.getText().toString());
    else if (id == R.id.tv__secondary_title)
      items.add(mTvSecondaryTitle.getText().toString());
    else if (id == R.id.tv__address)
      items.add(mTvAddress.getText().toString());
    else if (id == R.id.ll__place_latlon)
    {
      final double lat = mMapObject.getLat();
      final double lon = mMapObject.getLon();
      for (CoordinatesFormat format : visibleCoordsFormat)
        items.add(Framework.nativeFormatLatLon(lat, lon, format.getId()));
    }
    else if (id == R.id.ll__place_website)
      items.add(mTvWebsite.getText().toString());
    else if (id == R.id.ll__place_wikimedia)
      items.add(mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));
    else if (id == R.id.ll__place_facebook)
    {
      final String facebookPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
      if (facebookPage.indexOf('/') == -1)
        items.add(facebookPage); // Show username along with URL.
      items.add("https://m.facebook.com/" + facebookPage);
    }
    else if (id == R.id.ll__place_instagram)
    {
      final String instagramPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
      if (instagramPage.indexOf('/') == -1)
        items.add(instagramPage); // Show username along with URL.
      items.add("https://instagram.com/" + instagramPage);
    }
    else if (id == R.id.ll__place_twitter)
    {
      final String twitterPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
      if (twitterPage.indexOf('/') == -1)
        items.add(twitterPage); // Show username along with URL.
      items.add("https://mobile.twitter.com/" + twitterPage);
    }
    else if (id == R.id.ll__place_vk)
    {
      final String vkPage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
      if (vkPage.indexOf('/') == -1)
        items.add(vkPage); // Show username along with URL.
      items.add("https://vk.com/" + vkPage);
    }
    else if (id == R.id.ll__place_line)
    {
      final String linePage = mMapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
      if (linePage.indexOf('/') >= 0)
        items.add("https://" + linePage);
      else
      {
        items.add(linePage); // Show username along with URL.
        items.add("https://line.me/R/ti/p/@" + linePage);
      }
    }
    else if (id == R.id.ll__place_email)
      items.add(mTvEmail.getText().toString());
    else if (id == R.id.ll__place_operator)
      items.add(mTvOperator.getText().toString());
    else if (id == R.id.ll__place_level)
      items.add(mTvLevel.getText().toString());

    final Context context = requireContext();
    if (items.size() == 1)
      PlacePageUtils.copyToClipboard(context, mFrame, items.get(0));
    else
      PlacePageUtils.showCopyPopup(context, v, mFrame, items);

    return true;
  }

  private void updateDownloader(CountryItem country)
  {
    if (isInvalidDownloaderStatus(country.status))
    {
      if (mStorageCallbackSlot != 0)
        UiThread.runLater(mDownloaderDeferredDetachProc);
      return;
    }

    mDownloaderIcon.update(country);

    StringBuilder sb = new StringBuilder(StringUtils.getFileSizeString(requireContext(), country.totalSize));
    if (country.isExpandable())
      sb.append(StringUtils.formatUsingUsLocale("  •  %s: %d", requireContext().getString(R.string.downloader_status_maps),
                                                country.totalChildCount));

    mDownloaderInfo.setText(sb.toString());
  }

  private void updateDownloader()
  {
    if (mCurrentCountry == null)
      return;

    mCurrentCountry.update();
    updateDownloader(mCurrentCountry);
  }

  private void attachCountry(String country)
  {
    CountryItem map = CountryItem.fill(country);
    if (isInvalidDownloaderStatus(map.status))
      return;

    mCurrentCountry = map;
    if (mStorageCallbackSlot == 0)
      mStorageCallbackSlot = MapManager.nativeSubscribe(mStorageCallback);

    mDownloaderIcon.setOnIconClickListener(mDownloadClickListener)
                   .setOnCancelClickListener(mCancelDownloadListener);
    mDownloaderIcon.show(true);
    UiUtils.show(mDownloaderInfo);
    updateDownloader(mCurrentCountry);
  }

  private void detachCountry()
  {
    if (mStorageCallbackSlot == 0)
      return;

    MapManager.nativeUnsubscribe(mStorageCallbackSlot);
    mStorageCallbackSlot = 0;
    mCurrentCountry = null;
    mDownloaderIcon.setOnIconClickListener(null)
                   .setOnCancelClickListener(null);
    mDownloaderIcon.show(false);
    UiUtils.hide(mDownloaderInfo);
  }

  @Override
  public void onChanged(MapObject mapObject)
  {
    mMapObject = mapObject;

    detachCountry();
    setCurrentCountry();
    updateBookmarkButton();
    refreshViews();
    // In case the place page has already some data, make sure to call the onPlacePageContentChanged callback
    // to catch cases where the new data has the exact same height as the previous one (eg 2 address nodes)
    if (mFrame.getHeight() > 0)
      mPlacePageViewListener.onPlacePageContentChanged(mPreview.getHeight(), mFrame.getHeight());
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    if (MapObject.isOfType(MapObject.MY_POSITION, mMapObject))
      refreshMyPosition(location);
    else
      refreshDistanceToObject(location);
  }

  @Override
  public void onCompassUpdated(double north)
  {
    if (MapObject.isOfType(MapObject.MY_POSITION, mMapObject))
      return;

    final Location location = LocationHelper.INSTANCE.getSavedLocation();
    if (location == null)
    {
      UiUtils.hide(mAvDirection);
      return;
    }

    final double azimuth = Framework.nativeGetDistanceAndAzimuthFromLatLon(mMapObject.getLat(),
                                                                           mMapObject.getLon(),
                                                                           location.getLatitude(),
                                                                           location.getLongitude(),
                                                                           north)
                                    .getAzimuth();
    UiUtils.showIf(azimuth >= 0, mAvDirection);
    if (azimuth >= 0)
    {
      mAvDirection.setAzimuth(azimuth);
    }
  }

  public interface PlacePageViewListener
  {
    // Called when the content has actually changed and we are ready to compute the peek height
    void onPlacePageContentChanged(int previewHeight, int frameHeight);

    void onPlacePageRequestClose();

    void onPlacePageRequestToggleState();

    void onPlacePageRequestToggleRouteSettings(@NonNull RoadType roadType);
  }
}
