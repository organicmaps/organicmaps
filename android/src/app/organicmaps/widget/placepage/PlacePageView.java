package app.organicmaps.widget.placepage;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.res.Resources;
import android.location.Location;
import android.os.Bundle;
import android.text.Html;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.util.Linkify;
import android.util.Base64;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import androidx.recyclerview.widget.RecyclerView;
import app.organicmaps.Framework;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.api.Const;
import app.organicmaps.api.ParsedMwmRequest;
import app.organicmaps.bookmarks.data.Bookmark;
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
import app.organicmaps.editor.data.TimeFormatUtils;
import app.organicmaps.editor.data.Timespan;
import app.organicmaps.editor.data.Timetable;
import app.organicmaps.location.LocationHelper;
import app.organicmaps.location.LocationListener;
import app.organicmaps.routing.RoutingController;
import app.organicmaps.search.Popularity;
import app.organicmaps.settings.RoadType;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.StringUtils;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.concurrency.UiThread;
import app.organicmaps.util.log.Logger;
import app.organicmaps.widget.ArrowView;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;

public class PlacePageView extends Fragment implements View.OnClickListener,
                                                       View.OnLongClickListener,
                                                       EditBookmarkFragment.EditBookmarkListener,
                                                       PlacePageButtons.PlacePageButtonClickListener,
                                                       LocationListener,
                                                       Observer<MapObject>

{
  private static final String TAG = PlacePageView.class.getSimpleName();

  private static final String PREF_COORDINATES_FORMAT = "coordinates_format";
  private static final List<CoordinatesFormat> visibleCoordsFormat =
      Arrays.asList(CoordinatesFormat.LatLonDMS,
                    CoordinatesFormat.LatLonDecimal,
                    CoordinatesFormat.OLCFull,
                    CoordinatesFormat.OSMLink);
  @NonNull
  private final EditBookmarkClickListener mEditBookmarkClickListener = new EditBookmarkClickListener();
  private int mDescriptionMaxLength;
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
  private RecyclerView mPhoneRecycler;
  private PlacePhoneAdapter mPhoneAdapter;
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
  private View mOpeningHours;
  private TextView mTodayLabel;
  private TextView mTodayOpenTime;
  private TextView mTodayNonBusinessTime;
  private RecyclerView mFullWeekOpeningHours;
  private PlaceOpeningHoursAdapter mOpeningHoursAdapter;
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
  private View mWiki;
  private View mEntrance;
  private TextView mTvEntrance;
  private View mEditPlace;
  private View mAddOrganisation;
  private View mAddPlace;
  private View mEditTopSpace;
  // Bookmark
  private View mBookmarkFrame;
  private WebView mWvBookmarkNote;
  private TextView mTvBookmarkNote;
  private boolean mBookmarkSet;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPlaceDescriptionContainer;
  @SuppressWarnings("NotNullFieldNotInitialized")
  @NonNull
  private View mPlaceDescriptionHeaderContainer;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mPlaceDescriptionView;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPlaceDescriptionMoreBtn;
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
    mDescriptionMaxLength = getResources().getInteger(R.integer.place_page_description_max_length);
    mCoordsFormat = CoordinatesFormat.fromId(
        MwmApplication.prefs(requireContext()).getInt(
            PREF_COORDINATES_FORMAT, CoordinatesFormat.LatLonDecimal.getId()));
    LocationHelper.INSTANCE.addListener(this);

    mFrame = view;
    mFrame.setOnClickListener((v) -> mPlacePageViewListener.onPlacePageRequestToggleState());

    mPreview = mFrame.findViewById(R.id.pp__preview);
    mPreview.addOnLayoutChangeListener((v, left, top, right, bottom, oldLeft, oldTop, oldRight, oldBottom) -> {
      final int oldHeight = oldBottom - oldTop;
      final int newHeight = bottom - top;
      if (oldHeight != newHeight)
        mPreview.post(() -> mPlacePageViewListener.onPlacePageHeightChange(newHeight));
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
    mPhoneRecycler = mFrame.findViewById(R.id.rw__phone);
    mPhoneAdapter = new PlacePhoneAdapter();
    mPhoneRecycler.setAdapter(mPhoneAdapter);
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
    mOpeningHours = mFrame.findViewById(R.id.ll__place_schedule);
    mTodayLabel = mFrame.findViewById(R.id.oh_today_label);
    mTodayOpenTime = mFrame.findViewById(R.id.oh_today_open_time);
    mTodayNonBusinessTime = mFrame.findViewById(R.id.oh_nonbusiness_time);
    mFullWeekOpeningHours = mFrame.findViewById(R.id.rw__full_opening_hours);
    mOpeningHoursAdapter = new PlaceOpeningHoursAdapter();
    mFullWeekOpeningHours.setAdapter(mOpeningHoursAdapter);
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
    mWiki = mFrame.findViewById(R.id.ll__place_wiki);
    mWiki.setOnClickListener(this);
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
    mOpeningHours.setOnLongClickListener(this);
    mEmail.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mLevel.setOnLongClickListener(this);
    mWiki.setOnLongClickListener(this);

    mBookmarkFrame = mFrame.findViewById(R.id.bookmark_frame);
    mWvBookmarkNote = mBookmarkFrame.findViewById(R.id.wv__bookmark_notes);
    final WebSettings settings = mWvBookmarkNote.getSettings();
    settings.setJavaScriptEnabled(false);
    settings.setDefaultTextEncodingName("UTF-8");
    mTvBookmarkNote = mBookmarkFrame.findViewById(R.id.tv__bookmark_notes);
    mTvBookmarkNote.setOnLongClickListener(this);
    initEditMapObjectBtn();

    mDownloaderIcon = new DownloaderStatusIcon(mPreview.findViewById(R.id.downloader_status_frame));

    mDownloaderInfo = mPreview.findViewById(R.id.tv__downloader_details);

    // Place Description
    mPlaceDescriptionContainer = mFrame.findViewById(R.id.poi_description_container);
    mPlaceDescriptionView = mFrame.findViewById(R.id.poi_description);
    mPlaceDescriptionView.setOnLongClickListener(this);
    mPlaceDescriptionHeaderContainer = mFrame.findViewById(R.id.pp_description_header_container);
    mPlaceDescriptionMoreBtn = mFrame.findViewById(R.id.more_btn);
    mPlaceDescriptionMoreBtn.setOnClickListener(v -> showDescriptionScreen());

    viewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    viewModel.getMapObject().observe(requireActivity(), this);
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
    final MapObject mapObject = getMapObject();
    if (mapObject == null)
    {
      Logger.e(TAG, "Bookmark cannot be managed, mMapObject is null!");
      return;
    }
    toggleIsBookmark(mapObject);
  }

  private void onShareBtnClicked()
  {
    final MapObject mapObject = getMapObject();
    if (mapObject == null)
    {
      Logger.e(TAG, "A map object cannot be shared, it's null!");
      return;
    }
    SharingUtils.shareMapObject(requireContext(), mapObject);
  }

  private void onBackBtnClicked()
  {
    final MapObject mapObject = getMapObject();
    if (mapObject == null)
    {
      Logger.e(TAG, "A mwm request cannot be handled, mMapObject is null!");
      requireActivity().finish();
      return;
    }

    final ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
    if (request != null && request.isPickPointMode())
    {
      final Intent result = new Intent();
      result.putExtra(Const.EXTRA_POINT_LAT, mapObject.getLat())
            .putExtra(Const.EXTRA_POINT_LON, mapObject.getLon())
            .putExtra(Const.EXTRA_POINT_NAME, mapObject.getTitle())
            .putExtra(Const.EXTRA_POINT_ID, mapObject.getApiId())
            .putExtra(Const.EXTRA_ZOOM_LEVEL, Framework.nativeGetDrawScale());
      requireActivity().setResult(Activity.RESULT_OK, result);
      ParsedMwmRequest.setCurrentRequest(null);
    }
    requireActivity().finish();
  }

  private void onRouteFromBtnClicked()
  {
    final MapObject mapObject = getMapObject();
    RoutingController controller = RoutingController.get();
    if (!controller.isPlanning())
    {
      controller.prepare(mapObject, null);
      mPlacePageViewListener.onPlacePageRequestClose();
    }
    else if (controller.setStartPoint(mapObject))
    {
      mPlacePageViewListener.onPlacePageRequestClose();
    }
  }

  private void onRouteToBtnClicked()
  {
    final MapObject mapObject = getMapObject();
    if (RoutingController.get().isPlanning())
    {
      RoutingController.get().setEndPoint(mapObject);
      mPlacePageViewListener.onPlacePageRequestClose();
    }
    else
    {
      ((MwmActivity) requireActivity()).startLocationToPoint(mapObject);
    }
  }

  private void onRouteAddBtnClicked()
  {
    final MapObject mapObject = getMapObject();
    if (mapObject != null)
      RoutingController.get().addStop(mapObject);
  }

  private void onRouteRemoveBtnClicked()
  {
    final MapObject mapObject = getMapObject();
    if (mapObject != null)
      RoutingController.get().removeStop(mapObject);
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

  private void showDescriptionScreen()
  {
    Context context = mPlaceDescriptionContainer.getContext();
    String description = Objects.requireNonNull(getMapObject()).getDescription();
    PlaceDescriptionActivity.start(context, description);
  }

  private void initEditMapObjectBtn()
  {
    final View editBookmarkBtn = mBookmarkFrame.findViewById(R.id.tv__bookmark_edit);
    editBookmarkBtn.setVisibility(VISIBLE);
    editBookmarkBtn.setOnClickListener(mEditBookmarkClickListener);
  }

  private void onMapObjectChange(MapObject mapObject)
  {
    detachCountry();
    if (mapObject != null)
    {
      initEditMapObjectBtn();
      setCurrentCountry();
    }
    refreshViews();
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
    final MapObject mapObject = getMapObject();
    if (mapObject == null)
    {
      Logger.e(TAG, "A place page views cannot be refreshed, mMapObject is null");
      return;
    }
    refreshPreview(mapObject);
    refreshDetails(mapObject);
    refreshViewsInternal(mapObject);
  }

  private void refreshViewsInternal(@NonNull MapObject mapObject)
  {
    final Location loc = LocationHelper.INSTANCE.getSavedLocation();
    boolean showBackButton = false;
    boolean showRoutingButton = true;
    switch (mapObject.getMapObjectType())
    {
      case MapObject.BOOKMARK:
        refreshDistanceToObject(mapObject, loc);
        showBookmarkDetails(mapObject);
        updateBookmarkButton();
        break;
      case MapObject.POI:
      case MapObject.SEARCH:
        refreshDistanceToObject(mapObject, loc);
        hideBookmarkDetails();
        setPlaceDescription(mapObject);
        break;
      case MapObject.API_POINT:
        refreshDistanceToObject(mapObject, loc);
        hideBookmarkDetails();
        showBackButton = true;
        break;
      case MapObject.MY_POSITION:
        refreshMyPosition(mapObject, loc);
        hideBookmarkDetails();
        showRoutingButton = false;
        break;
    }
    final boolean hasNumber = mapObject.hasPhoneNumber();
    if (hasNumber)
      mPhoneRecycler.setVisibility(VISIBLE);
    else
      mPhoneRecycler.setVisibility(GONE);
    updateButtons(mapObject, showBackButton, showRoutingButton);
  }

  private Spanned getShortDescription(@NonNull MapObject mapObject)
  {
    String htmlDescription = mapObject.getDescription();
    final int paragraphStart = htmlDescription.indexOf("<p>");
    final int paragraphEnd = htmlDescription.indexOf("</p>");
    if (paragraphStart == 0 && paragraphEnd != -1)
      htmlDescription = htmlDescription.substring(3, paragraphEnd);

    Spanned description = Html.fromHtml(htmlDescription);
    if (description.length() > mDescriptionMaxLength)
    {
      description = (Spanned) new SpannableStringBuilder(description)
          .insert(mDescriptionMaxLength - 3, "...")
          .subSequence(0, mDescriptionMaxLength);
    }

    return description;
  }

  private void setPlaceDescription(@NonNull MapObject mapObject)
  {
    boolean isBookmark = MapObject.isOfType(MapObject.BOOKMARK, mapObject);
    if (TextUtils.isEmpty(mapObject.getDescription()) && !isBookmark)
    {
      UiUtils.hide(mPlaceDescriptionContainer, mPlaceDescriptionHeaderContainer);
      return;
    }

    if (isBookmark)
    {
      final Bookmark bmk = (Bookmark) mapObject;
      UiUtils.showIf(!TextUtils.isEmpty(bmk.getBookmarkDescription()), mPlaceDescriptionHeaderContainer);
      UiUtils.hide(mPlaceDescriptionContainer);
      return;
    }
    UiUtils.show(mPlaceDescriptionContainer, mPlaceDescriptionHeaderContainer);
    mPlaceDescriptionView.setText(getShortDescription(mapObject));
  }

  private void setTextAndColorizeSubtitle(@NonNull MapObject mapObject)
  {
    String text = mapObject.getSubtitle();
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

  private void refreshPreview(@NonNull MapObject mapObject)
  {
    UiUtils.setTextAndHideIfEmpty(mTvTitle, mapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSecondaryTitle, mapObject.getSecondaryTitle());
    boolean isPopular = mapObject.getPopularity().getType() == Popularity.Type.POPULAR;
    if (mPopularityView != null)
      UiUtils.showIf(isPopular, mPopularityView);
    if (mToolbar != null)
      mToolbar.setTitle(mapObject.getTitle());
    setTextAndColorizeSubtitle(mapObject);
    UiUtils.setTextAndHideIfEmpty(mTvAddress, mapObject.getAddress());
  }

  private void refreshDetails(@NonNull MapObject mapObject)
  {
    refreshLatLon(mapObject);

    String website = mapObject.getMetadata(Metadata.MetadataType.FMD_WEBSITE);
    String url = mapObject.getMetadata(Metadata.MetadataType.FMD_URL);
    refreshMetadataOrHide(TextUtils.isEmpty(website) ? url : website, mWebsite, mTvWebsite);
    String wikimedia_commons = mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS);
    String wikimedia_commons_text = TextUtils.isEmpty(wikimedia_commons) ? "" : getResources().getString(R.string.wikimedia_commons);
    refreshMetadataOrHide(wikimedia_commons_text, mWikimedia, mTvWikimedia);
    refreshPhoneNumberList(mapObject.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER));
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR), mOperator, mTvOperator);
    /// @todo I don't like it when we take all data from mapObject, but for cuisines, we should
    /// go into JNI Framework and rely on some "active object".
    refreshMetadataOrHide(Framework.nativeGetActiveObjectFormattedCuisine(), mCuisine, mTvCuisine);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA), mWiki, null);
    refreshWiFi(mapObject);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    refreshOpeningHours(mapObject);
    refreshSocialLinks(mapObject);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_LEVEL), mLevel, mTvLevel);

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
    setPlaceDescription(mapObject);
  }

  private void refreshOpeningHours(@NonNull MapObject mapObject)
  {
    final String ohStr = mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
    final boolean isEmptyTT = (timetables == null || timetables.length == 0);
    final int color = ThemeUtils.getColor(getContext(), android.R.attr.textColorPrimary);

    if (isEmptyTT)
    {
      // 'opening_hours' tag wasn't parsed either because it's empty or wrong format.
      if (!ohStr.isEmpty())
      {
        UiUtils.show(mOpeningHours);
        refreshTodayOpeningHours(ohStr, color);
        UiUtils.hide(mTodayNonBusinessTime);
        UiUtils.hide(mFullWeekOpeningHours);
      }
      else
      {
        UiUtils.hide(mOpeningHours);
      }
      return;
    }

    UiUtils.show(mOpeningHours);

    final Resources resources = getResources();

    if (timetables[0].isFullWeek())
    {
      final Timetable tt = timetables[0];
      if (tt.isFullday)
      {
        refreshTodayOpeningHours(resources.getString(R.string.twentyfour_seven), color);
        UiUtils.clearTextAndHide(mTodayNonBusinessTime);
        UiUtils.hide(mTodayNonBusinessTime);
      }
      else
      {
        refreshTodayOpeningHours(resources.getString(R.string.daily), tt.workingTimespan.toWideString(), color);
        refreshTodayNonBusinessTime(tt.closedTimespans);
      }

      UiUtils.hide(mFullWeekOpeningHours);
      return;
    }

    // Show whole week time table.
    int firstDayOfWeek = Calendar.getInstance(Locale.getDefault()).getFirstDayOfWeek();
    mOpeningHoursAdapter.setTimetables(timetables, firstDayOfWeek);
    UiUtils.show(mFullWeekOpeningHours);

    // Show today's open time + non-business time.
    boolean containsCurrentWeekday = false;
    final int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
    for (Timetable tt : timetables)
    {
      if (tt.containsWeekday(currentDay))
      {
        containsCurrentWeekday = true;
        String openTime;

        if (tt.isFullday)
        {
          String allDay = resources.getString(R.string.editor_time_allday);
          openTime = Utils.unCapitalize(allDay);
        }
        else
        {
          openTime = tt.workingTimespan.toWideString();
        }

        refreshTodayOpeningHours(resources.getString(R.string.today), openTime, color);
        refreshTodayNonBusinessTime(tt.closedTimespans);

        break;
      }
    }

    // Show that place is closed today.
    if (!containsCurrentWeekday)
    {
      refreshTodayOpeningHours(resources.getString(R.string.day_off_today), resources.getColor(R.color.base_red));
      UiUtils.hide(mTodayNonBusinessTime);
    }
  }

  private void refreshTodayNonBusinessTime(Timespan[] closedTimespans)
  {
    final String hoursClosedLabel = getResources().getString(R.string.editor_hours_closed);
    if (closedTimespans == null || closedTimespans.length == 0)
      UiUtils.clearTextAndHide(mTodayNonBusinessTime);
    else
      UiUtils.setTextAndShow(mTodayNonBusinessTime, TimeFormatUtils.formatNonBusinessTime(closedTimespans, hoursClosedLabel));
  }

  private void refreshWiFi(@NonNull MapObject mapObject)
  {
    final String inet = mapObject.getMetadata(Metadata.MetadataType.FMD_INTERNET);
    if (!TextUtils.isEmpty(inet))
    {
      mWifi.setVisibility(VISIBLE);
      /// @todo Better (but harder) to wrap C++ osm::Internet into Java, instead of comparing with "no".
      mTvWiFi.setText(TextUtils.equals(inet, "no") ? R.string.no_available : R.string.yes_available);
    }
    else
      mWifi.setVisibility(GONE);
  }

  private void refreshSocialLinks(@NonNull MapObject mapObject)
  {
    final String facebookPageLink = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
    refreshSocialPageLink(mFacebookPage, mTvFacebookPage, facebookPageLink, "facebook.com");
    final String instagramPageLink = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
    refreshSocialPageLink(mInstagramPage, mTvInstagramPage, instagramPageLink, "instagram.com");
    final String twitterPageLink = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
    refreshSocialPageLink(mTwitterPage, mTvTwitterPage, twitterPageLink, "twitter.com");
    final String vkPageLink = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
    refreshSocialPageLink(mVkPage, mTvVkPage, vkPageLink, "vk.com");
    final String linePageLink = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
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

  private void refreshTodayOpeningHours(String label, String openTime, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayLabel, label);
    UiUtils.setTextAndShow(mTodayOpenTime, openTime);

    mTodayLabel.setTextColor(color);
    mTodayOpenTime.setTextColor(color);
  }

  private void refreshTodayOpeningHours(String label, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayLabel, label);
    UiUtils.hide(mTodayOpenTime);

    mTodayLabel.setTextColor(color);
    mTodayOpenTime.setTextColor(color);
  }

  private void refreshPhoneNumberList(String phones)
  {
    mPhoneAdapter.refreshPhones(phones);
  }

  private void updateBookmarkButton()
  {
    final List<PlacePageButtons.ButtonType> currentButtons = viewModel.getCurrentButtons()
                                                                      .getValue();
    PlacePageButtons.ButtonType oldType = PlacePageButtons.ButtonType.BOOKMARK_DELETE;
    PlacePageButtons.ButtonType newType = PlacePageButtons.ButtonType.BOOKMARK_SAVE;
    if (mBookmarkSet)
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

  private void hideBookmarkDetails()
  {
    mBookmarkSet = false;
    UiUtils.hide(mBookmarkFrame);
    updateBookmarkButton();
  }

  private void showBookmarkDetails(@NonNull MapObject mapObject)
  {
    mBookmarkSet = true;
    UiUtils.show(mBookmarkFrame);

    final String notes = ((Bookmark) mapObject).getBookmarkDescription();

    if (TextUtils.isEmpty(notes))
    {
      UiUtils.hide(mTvBookmarkNote, mWvBookmarkNote);
      return;
    }

    if (StringUtils.nativeIsHtml(notes))
    {
      // According to loadData documentation, HTML should be either base64 or percent encoded.
      // Default UTF-8 encoding for all content is set above in WebSettings.
      final String b64encoded = Base64.encodeToString(notes.getBytes(), Base64.DEFAULT);
      mWvBookmarkNote.loadData(b64encoded, Utils.TEXT_HTML, "base64");
      UiUtils.show(mWvBookmarkNote);
      UiUtils.hide(mTvBookmarkNote);
    }
    else
    {
      mTvBookmarkNote.setText(notes);
      Linkify.addLinks(mTvBookmarkNote, Linkify.ALL);
      UiUtils.show(mTvBookmarkNote);
      UiUtils.hide(mWvBookmarkNote);
    }
  }

  private void updateButtons(@NonNull MapObject mapObject, boolean showBackButton, boolean showRoutingButton)
  {
    List<PlacePageButtons.ButtonType> buttons = new ArrayList<>();
    if (mapObject.getRoadWarningMarkType() != RoadWarningMarkType.UNKNOWN)
    {
      RoadWarningMarkType markType = mapObject.getRoadWarningMarkType();
      PlacePageButtons.ButtonType roadType = toPlacePageButton(markType);
      buttons.add(roadType);
    }
    else if (RoutingController.get().isRoutePoint(mapObject))
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

      if (needToShowRoutingButtons && RoutingController.get().isStopPointAllowed())
        buttons.add(PlacePageButtons.ButtonType.ROUTE_ADD);
      else
        buttons.add(mapObject.getMapObjectType() == MapObject.BOOKMARK
                    ? PlacePageButtons.ButtonType.BOOKMARK_DELETE
                    : PlacePageButtons.ButtonType.BOOKMARK_SAVE);

      if (needToShowRoutingButtons)
      {
        buttons.add(PlacePageButtons.ButtonType.ROUTE_TO);
        if (!RoutingController.get().isStopPointAllowed())
          buttons.add(PlacePageButtons.ButtonType.ROUTE_ADD);
        else
          buttons.add(mapObject.getMapObjectType() == MapObject.BOOKMARK
                      ? PlacePageButtons.ButtonType.BOOKMARK_DELETE
                      : PlacePageButtons.ButtonType.BOOKMARK_SAVE);
      }
      buttons.add(PlacePageButtons.ButtonType.SHARE);
    }
    viewModel.setCurrentButtons(buttons);
  }

  private void refreshMyPosition(@NonNull MapObject mapObject, Location l)
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

    mapObject.setLat(l.getLatitude());
    mapObject.setLon(l.getLongitude());
    refreshLatLon(mapObject);
  }

  private void refreshDistanceToObject(@NonNull MapObject mapObject, Location l)
  {
    UiUtils.showIf(l != null, mTvDistance);
    if (l == null)
      return;

    double lat = mapObject.getLat();
    double lon = mapObject.getLon();
    DistanceAndAzimut distanceAndAzimuth =
        Framework.nativeGetDistanceAndAzimuthFromLatLon(lat, lon, l.getLatitude(), l.getLongitude(), 0.0);
    mTvDistance.setText(distanceAndAzimuth.getDistance());
  }

  private void refreshLatLon(@NonNull MapObject mapObject)
  {
    final double lat = mapObject.getLat();
    final double lon = mapObject.getLon();
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
    final MapObject mapObject = getMapObject();
    final Context context = requireContext();
    final int id = v.getId();
    if (id == R.id.tv__title || id == R.id.tv__secondary_title || id == R.id.tv__address)
    {
      // A workaround to make single taps toggle the bottom sheet.
      mPlacePageViewListener.onPlacePageRequestToggleState();
    }
    else if (id == R.id.ll__place_editor)
    {
      if (mapObject == null)
      {
        Logger.e(TAG, "Cannot start editor, map object is null!");
        return;
      }
      ((MwmActivity) requireActivity()).showEditor();
    }
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
      if (mapObject == null)
      {
        Logger.e(TAG, "A LatLon cannot be refreshed, mMapObject is null");
        return;
      }
      refreshLatLon(mapObject);
    }
    else if (id == R.id.ll__place_website)
      Utils.openUrl(context, mTvWebsite.getText().toString());
    else if (id == R.id.ll__place_wikimedia)
      Utils.openUrl(context, mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));
    else if (id == R.id.ll__place_facebook)
    {
      final String facebookPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
      Utils.openUrl(context, "https://m.facebook.com/" + facebookPage);
    }
    else if (id == R.id.ll__place_instagram)
    {
      final String instagramPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
      Utils.openUrl(context, "https://instagram.com/" + instagramPage);
    }
    else if (id == R.id.ll__place_twitter)
    {
      final String twitterPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
      Utils.openUrl(context, "https://mobile.twitter.com/" + twitterPage);
    }
    else if (id == R.id.ll__place_vk)
    {
      final String vkPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
      Utils.openUrl(context, "https://vk.com/" + vkPage);
    }
    else if (id == R.id.ll__place_line)
    {
      final String linePage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
      if (linePage.indexOf('/') >= 0)
        Utils.openUrl(context, "https://" + linePage);
      else
        Utils.openUrl(context, "https://line.me/R/ti/p/@" + linePage);
    }
    else if (id == R.id.ll__place_wiki)
      Utils.openUrl(context, mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA));
    else if (id == R.id.direction_frame)
      showBigDirection();
    else if (id == R.id.ll__place_email)
      Utils.sendTo(context, mTvEmail.getText().toString());
  }

  private void toggleIsBookmark(@NonNull MapObject mapObject)
  {
    if (MapObject.isOfType(MapObject.BOOKMARK, mapObject))
      viewModel.setMapObject(Framework.nativeDeleteBookmarkFromMapObject());
    else
      viewModel.setMapObject(BookmarkManager.INSTANCE.addNewBookmark(mapObject.getLat(), mapObject.getLon()));
  }

  private void showBigDirection()
  {
    final FragmentManager fragmentManager = requireActivity().getSupportFragmentManager();
    final DirectionFragment fragment = (DirectionFragment) fragmentManager.getFragmentFactory()
                                                                          .instantiate(getContext().getClassLoader(), DirectionFragment.class.getName());
    fragment.setMapObject(getMapObject());
    fragment.show(fragmentManager, null);
  }

  /// @todo Unify urls processing (fb, twitter, instagram, ...).
  /// onLongClick behaviour differs even from onClick function several lines above.

  @Override
  public boolean onLongClick(View v)
  {
    final MapObject mapObject = getMapObject();
    final List<String> items = new ArrayList<>();
    final int id = v.getId();
    if (id == R.id.tv__title)
      items.add(mTvTitle.getText().toString());
    else if (id == R.id.tv__secondary_title)
      items.add(mTvSecondaryTitle.getText().toString());
    else if (id == R.id.tv__address)
      items.add(mTvAddress.getText().toString());
    else if (id == R.id.tv__bookmark_notes)
      items.add(mTvBookmarkNote.getText().toString());
    else if (id == R.id.poi_description)
      items.add(mPlaceDescriptionView.getText().toString());
    else if (id == R.id.ll__place_latlon)
    {
      if (mapObject != null)
      {
        final double lat = mapObject.getLat();
        final double lon = mapObject.getLon();
        for (CoordinatesFormat format : visibleCoordsFormat)
          items.add(Framework.nativeFormatLatLon(lat, lon, format.getId()));
      }
      else
        Logger.e(TAG, "A long click tap on LatLon cannot be handled, mMapObject is null!");
    }
    else if (id == R.id.ll__place_website)
      items.add(mTvWebsite.getText().toString());
    else if (id == R.id.ll__place_wikimedia)
      items.add(mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIMEDIA_COMMONS));
    else if (id == R.id.ll__place_facebook)
    {
      final String facebookPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_FACEBOOK);
      if (facebookPage.indexOf('/') == -1)
        items.add(facebookPage); // Show username along with URL.
      items.add("https://m.facebook.com/" + facebookPage);
    }
    else if (id == R.id.ll__place_instagram)
    {
      final String instagramPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_INSTAGRAM);
      if (instagramPage.indexOf('/') == -1)
        items.add(instagramPage); // Show username along with URL.
      items.add("https://instagram.com/" + instagramPage);
    }
    else if (id == R.id.ll__place_twitter)
    {
      final String twitterPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_TWITTER);
      if (twitterPage.indexOf('/') == -1)
        items.add(twitterPage); // Show username along with URL.
      items.add("https://mobile.twitter.com/" + twitterPage);
    }
    else if (id == R.id.ll__place_vk)
    {
      final String vkPage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_VK);
      if (vkPage.indexOf('/') == -1)
        items.add(vkPage); // Show username along with URL.
      items.add("https://vk.com/" + vkPage);
    }
    else if (id == R.id.ll__place_line)
    {
      final String linePage = mapObject.getMetadata(Metadata.MetadataType.FMD_CONTACT_LINE);
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
    else if (id == R.id.ll__place_schedule)
    {
      final String ohStr = mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
      final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(ohStr);
      items.add(TimeFormatUtils.formatTimetables(getResources(), ohStr, timetables));
    }
    else if (id == R.id.ll__place_operator)
      items.add(mTvOperator.getText().toString());
    else if (id == R.id.ll__place_wiki)
      items.add(mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA));
    else if (id == R.id.ll__place_level)
      items.add(mTvLevel.getText().toString());

    final Context ctx = getContext();
    if (items.size() == 1)
    {
      Utils.copyTextToClipboard(ctx, items.get(0));
      Utils.showSnackbarAbove(mDetails,
                              mFrame.getRootView().findViewById(R.id.pp_buttons_layout),
                              ctx.getString(R.string.copied_to_clipboard, items.get(0)));
    }
    else
    {
      final PopupMenu popup = new PopupMenu(getContext(), v);
      final Menu menu = popup.getMenu();
      final String copyText = getResources().getString(android.R.string.copy);

      for (int i = 0; i < items.size(); i++)
        menu.add(Menu.NONE, i, i, String.format("%s %s", copyText, items.get(i)));

      popup.setOnMenuItemClickListener(item -> {
        final int itemId = item.getItemId();
        Utils.copyTextToClipboard(ctx, items.get(itemId));
        Utils.showSnackbarAbove(mDetails,
                                mFrame.getRootView().findViewById(R.id.pp_buttons_layout),
                                ctx.getString(R.string.copied_to_clipboard, items.get(itemId)));
        return true;
      });
      popup.show();
    }

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
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    Bookmark updatedBookmark = BookmarkManager.INSTANCE.updateBookmarkPlacePage(bookmarkId);
    if (updatedBookmark == null)
      return;

    viewModel.setMapObject(updatedBookmark);
    refreshViews();
    mPlacePageViewListener.onPlacePageHeightChange(getPreviewHeight());
  }

  private int getPreviewHeight()
  {
    return mPreview.getHeight();
  }

  @Override
  public void onChanged(MapObject mapObject)
  {
    onMapObjectChange(mapObject);
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    final MapObject mapObject = getMapObject();
    if (mapObject == null)
    {
      // TODO: This method is constantly called even when nothing is selected on the map.
      //Logger.e(TAG, "A location cannot be refreshed, mMapObject is null!");
      return;
    }

    if (MapObject.isOfType(MapObject.MY_POSITION, mapObject))
      refreshMyPosition(mapObject, location);
    else
      refreshDistanceToObject(mapObject, location);
  }

  @Override
  public void onCompassUpdated(double north)
  {
    final MapObject mapObject = getMapObject();
    if (mapObject == null || MapObject.isOfType(MapObject.MY_POSITION, mapObject))
      return;

    final Location location = LocationHelper.INSTANCE.getSavedLocation();
    if (location == null)
    {
      UiUtils.hide(mAvDirection);
      return;
    }

    final double azimuth = Framework.nativeGetDistanceAndAzimuthFromLatLon(mapObject.getLat(),
                                                                           mapObject.getLon(),
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

  @Nullable
  private MapObject getMapObject()
  {
    if (viewModel != null)
      return viewModel.getMapObject().getValue();
    return null;
  }

  public interface PlacePageViewListener
  {
    void onPlacePageHeightChange(int previewHeight);

    void onPlacePageRequestClose();

    void onPlacePageRequestToggleState();

    void onPlacePageRequestToggleRouteSettings(@NonNull RoadType roadType);
  }

  private class EditBookmarkClickListener implements View.OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      final MapObject mapObject = getMapObject();
      if (mapObject == null)
      {
        Logger.e(TAG, "A bookmark cannot be edited, mMapObject is null!");
        return;
      }
      Bookmark bookmark = (Bookmark) mapObject;
      EditBookmarkFragment.editBookmark(bookmark.getCategoryId(),
                                        bookmark.getBookmarkId(),
                                        requireActivity(),
                                        requireActivity().getSupportFragmentManager(),
                                        PlacePageView.this);
    }
  }
}
