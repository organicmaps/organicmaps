package com.mapswithme.maps.widget.placepage;

import android.app.Activity;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.res.Resources;
import android.content.res.TypedArray;
import android.graphics.Color;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.os.Build;
import android.text.Html;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.util.Linkify;
import android.util.AttributeSet;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.WebView;
import android.widget.GridView;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.RelativeLayout;
import android.widget.TextView;

import androidx.annotation.ColorInt;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.core.widget.NestedScrollViewClickFixed;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import com.mapswithme.maps.Framework;
import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.ads.LocalAdInfo;
import com.mapswithme.maps.api.ParsedMwmRequest;
import com.mapswithme.maps.base.Detachable;
import com.mapswithme.maps.bookmarks.data.Bookmark;
import com.mapswithme.maps.bookmarks.data.BookmarkManager;
import com.mapswithme.maps.bookmarks.data.DistanceAndAzimut;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.bookmarks.data.RoadWarningMarkType;
import com.mapswithme.maps.downloader.CountryItem;
import com.mapswithme.maps.downloader.DownloaderStatusIcon;
import com.mapswithme.maps.downloader.MapManager;
import com.mapswithme.maps.editor.Editor;
import com.mapswithme.maps.editor.OpeningHours;
import com.mapswithme.maps.editor.data.TimeFormatUtils;
import com.mapswithme.maps.editor.data.Timetable;
import com.mapswithme.maps.gallery.Constants;
import com.mapswithme.maps.gallery.FullScreenGalleryActivity;
import com.mapswithme.maps.gallery.GalleryActivity;
import com.mapswithme.maps.location.LocationHelper;
import com.mapswithme.maps.metrics.UserActionsLogger;
import com.mapswithme.maps.promo.CatalogPromoController;
import com.mapswithme.maps.promo.PromoCityGallery;
import com.mapswithme.maps.promo.PromoEntity;
import com.mapswithme.maps.review.Review;
import com.mapswithme.maps.routing.RoutingController;
import com.mapswithme.maps.search.FilterUtils;
import com.mapswithme.maps.search.HotelsFilter;
import com.mapswithme.maps.search.Popularity;
import com.mapswithme.maps.settings.RoadType;
import com.mapswithme.maps.taxi.TaxiType;
import com.mapswithme.maps.ugc.Impress;
import com.mapswithme.maps.ugc.UGCController;
import com.mapswithme.maps.widget.ArrowView;
import com.mapswithme.maps.widget.LineCountTextView;
import com.mapswithme.maps.widget.RatingView;
import com.mapswithme.maps.widget.recycler.ItemDecoratorFactory;
import com.mapswithme.maps.widget.recycler.RecyclerClickListener;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.Graphics;
import com.mapswithme.util.NetworkPolicy;
import com.mapswithme.util.SponsoredLinks;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.ThemeUtils;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.concurrency.UiThread;
import com.mapswithme.util.log.Logger;
import com.mapswithme.util.log.LoggerFactory;
import com.mapswithme.util.sharing.ShareOption;
import com.mapswithme.util.statistics.AlohaHelper;
import com.mapswithme.util.statistics.Statistics;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

import static com.mapswithme.maps.widget.placepage.PlacePageButtons.Item.BOOKING;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_HOTEL_DESCRIPTION_LAND;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_HOTEL_FACILITIES;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_HOTEL_GALLERY_OPEN;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_HOTEL_REVIEWS_LAND;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_ACTION;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_DETAILS;
import static com.mapswithme.util.statistics.Statistics.EventName.PP_SPONSORED_OPENTABLE;

public class PlacePageView extends NestedScrollViewClickFixed
    implements View.OnClickListener,
               View.OnLongClickListener,
               Sponsored.OnPriceReceivedListener,
               Sponsored.OnHotelInfoReceivedListener,
               LineCountTextView.OnLineCountCalculatedListener,
               RecyclerClickListener,
               NearbyAdapter.OnItemClickListener,
               EditBookmarkFragment.EditBookmarkListener,
               Detachable<Activity>

{
  private static final Logger LOGGER = LoggerFactory.INSTANCE.getLogger(LoggerFactory.Type.MISC);
  private static final String TAG = PlacePageView.class.getSimpleName();
  private static final String PREF_USE_DMS = "use_dms";
  private static final String DISCOUNT_PREFIX = "-";
  private static final String DISCOUNT_SUFFIX = "%";

  private boolean mIsDocked;
  private boolean mIsFloating;

  // Preview.
  private ViewGroup mPreview;
  private Toolbar mToolbar;
  private TextView mTvTitle;
  private TextView mTvSecondaryTitle;
  private TextView mTvSubtitle;
  private ArrowView mAvDirection;
  private TextView mTvDistance;
  private TextView mTvAddress;
  private View mPreviewRatingInfo;
  private RatingView mRatingView;
  private TextView mTvSponsoredPrice;
  @SuppressWarnings("NullableProblems")
  @NonNull
  private RatingView mHotelDiscount;
  private View mPhone;
  private TextView mTvPhone;
  private View mWebsite;
  private TextView mTvWebsite;
  private TextView mTvLatlon;
  private View mOpeningHours;
  private TextView mFullOpeningHours;
  private TextView mTodayOpeningHours;
  private View mWifi;
  private View mEmail;
  private TextView mTvEmail;
  private View mOperator;
  private TextView mTvOperator;
  private View mCuisine;
  private TextView mTvCuisine;
  private View mWiki;
  private View mEntrance;
  private TextView mTvEntrance;
  private View mTaxiShadow;
  private View mTaxiDivider;
  private View mTaxi;
  private View mEditPlace;
  private View mAddOrganisation;
  private View mAddPlace;
  private View mLocalAd;
  private TextView mTvLocalAd;
  private View mEditTopSpace;
  // Bookmark
  private View mBookmarkFrame;
  private WebView mWvBookmarkNote;
  private TextView mTvBookmarkNote;
  private boolean mBookmarkSet;
  // Place page buttons
  private PlacePageButtons mButtons;
  private ImageView mBookmarkButtonIcon;
  // Hotel
  private View mHotelDescription;
  private LineCountTextView mTvHotelDescription;
  private View mHotelMoreDescription;
  private View mHotelFacilities;
  private View mHotelMoreFacilities;
  private View mHotelGallery;
  private RecyclerView mRvHotelGallery;
  private View mHotelNearby;
  private View mHotelReview;
  private TextView mHotelRating;
  private TextView mHotelRatingBase;
  private View mHotelMore;
  @Nullable
  private View mBookmarkButtonFrame;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPlaceDescriptionContainer;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private TextView mPlaceDescriptionView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPlaceDescriptionMoreBtn;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mPopularityView;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private UGCController mUgcController;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private CatalogPromoController mCatalogPromoController;

  // Data
  @Nullable
  private MapObject mMapObject;
  @Nullable
  private Sponsored mSponsored;
  private String mSponsoredPrice;
  private boolean mIsLatLonDms;
  @NonNull
  private final FacilitiesAdapter mFacilitiesAdapter = new FacilitiesAdapter();
  @NonNull
  private final com.mapswithme.maps.widget.placepage.GalleryAdapter mGalleryAdapter;
  @NonNull
  private final NearbyAdapter mNearbyAdapter = new NearbyAdapter(this);
  @NonNull
  private final ReviewAdapter mReviewAdapter = new ReviewAdapter();

  // Downloader`s stuff
  private DownloaderStatusIcon mDownloaderIcon;
  private TextView mDownloaderInfo;
  private int mStorageCallbackSlot;
  @Nullable
  private CountryItem mCurrentCountry;
  private boolean mScrollable = true;

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

  private final Runnable mDownloaderDeferredDetachProc = new Runnable()
  {
    @Override
    public void run()
    {
      detachCountry();
    }
  };
  @NonNull
  private final EditBookmarkClickListener mEditBookmarkClickListener = new EditBookmarkClickListener();

  @NonNull
  private OnClickListener mDownloadClickListener = new OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.warn3gAndDownload(getActivity(), mCurrentCountry.id, this::onDownloadClick);
    }

    private void onDownloadClick()
    {
      String scenario = mCurrentCountry.isExpandable() ? "download_group" : "download";
      Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_ACTION,
                                     Statistics.params()
                                               .add(Statistics.EventParam.ACTION, "download")
                                               .add(Statistics.EventParam.FROM, "placepage")
                                               .add("is_auto", "false")
                                               .add("scenario", scenario));
    }
  };

  @NonNull
  private OnClickListener mCancelDownloadListener = new OnClickListener()
  {
    @Override
    public void onClick(View v)
    {
      MapManager.nativeCancel(mCurrentCountry.id);
      Statistics.INSTANCE.trackEvent(Statistics.EventName.DOWNLOADER_CANCEL,
                                     Statistics.params()
                                               .add(Statistics.EventParam.FROM, "placepage"));
    }
  };

  @Nullable
  private Closable mClosable;

  @Nullable
  private RoutingModeListener mRoutingModeListener;

  @SuppressWarnings("NullableProblems")
  @NonNull
  private View mCatalogPromoTitleView;

  void setScrollable(boolean scrollable)
  {
    mScrollable = scrollable;
  }

  void addClosable(@NonNull Closable closable)
  {
    mClosable = closable;
  }

  @Override
  public boolean onTouchEvent(MotionEvent ev)
  {
    switch (ev.getAction())
    {
      case MotionEvent.ACTION_DOWN:
        return mScrollable && super.onTouchEvent(ev);
      default:
        return super.onTouchEvent(ev);
    }
  }

  @Override
  public boolean onInterceptTouchEvent(MotionEvent event)
  {
    return mScrollable && super.onInterceptTouchEvent(event);
  }

  @Override
  public void attach(@NonNull Activity object)
  {
    mCatalogPromoController.attach(object);
  }

  @Override
  public void detach()
  {
    mCatalogPromoController.detach();
  }

  public interface SetMapObjectListener
  {
    void onSetMapObjectComplete(@NonNull NetworkPolicy policy, boolean isSameObject);
  }

  public PlacePageView(Context context)
  {
    this(context, null, 0);
  }

  public PlacePageView(Context context, AttributeSet attrs)
  {
    this(context, attrs, 0);
  }

  public PlacePageView(Context context, AttributeSet attrs, int defStyleAttr)
  {
    super(context, attrs);
    mIsLatLonDms = MwmApplication.prefs().getBoolean(PREF_USE_DMS, false);
    mGalleryAdapter = new com.mapswithme.maps.widget.placepage.GalleryAdapter(context);
    init(attrs, defStyleAttr);
  }

  @Override
  protected void onFinishInflate()
  {
    super.onFinishInflate();
    mPreview = findViewById(R.id.pp__preview);
    mTvTitle = mPreview.findViewById(R.id.tv__title);
    mPopularityView = findViewById(R.id.popular_rating_view);
    mTvSecondaryTitle = mPreview.findViewById(R.id.tv__secondary_title);
    mToolbar = findViewById(R.id.toolbar);
    mTvSubtitle = mPreview.findViewById(R.id.tv__subtitle);

    View directionFrame = mPreview.findViewById(R.id.direction_frame);
    mTvDistance = mPreview.findViewById(R.id.tv__straight_distance);
    mAvDirection = mPreview.findViewById(R.id.av__direction);
    directionFrame.setOnClickListener(this);

    mTvAddress = mPreview.findViewById(R.id.tv__address);
    mPreview.findViewById(R.id.search_hotels_btn).setOnClickListener(this);

    mPreviewRatingInfo = mPreview.findViewById(R.id.preview_rating_info);
    mRatingView = mPreviewRatingInfo.findViewById(R.id.rating_view);
    mTvSponsoredPrice = mPreviewRatingInfo.findViewById(R.id.tv__hotel_price);
    mHotelDiscount = mPreviewRatingInfo.findViewById(R.id.discount_in_percents);

    RelativeLayout address = findViewById(R.id.ll__place_name);
    mPhone = findViewById(R.id.ll__place_phone);
    mPhone.setOnClickListener(this);
    mTvPhone = findViewById(R.id.tv__place_phone);
    mWebsite = findViewById(R.id.ll__place_website);
    mWebsite.setOnClickListener(this);
    mTvWebsite = findViewById(R.id.tv__place_website);
    LinearLayout latlon = findViewById(R.id.ll__place_latlon);
    latlon.setOnClickListener(this);
    mTvLatlon = findViewById(R.id.tv__place_latlon);
    mOpeningHours = findViewById(R.id.ll__place_schedule);
    mFullOpeningHours = findViewById(R.id.opening_hours);
    mTodayOpeningHours = findViewById(R.id.today_opening_hours);
    mWifi = findViewById(R.id.ll__place_wifi);
    mEmail = findViewById(R.id.ll__place_email);
    mEmail.setOnClickListener(this);
    mTvEmail = findViewById(R.id.tv__place_email);
    mOperator = findViewById(R.id.ll__place_operator);
    mOperator.setOnClickListener(this);
    mTvOperator = findViewById(R.id.tv__place_operator);
    mCuisine = findViewById(R.id.ll__place_cuisine);
    mTvCuisine = findViewById(R.id.tv__place_cuisine);
    mWiki = findViewById(R.id.ll__place_wiki);
    mWiki.setOnClickListener(this);
    mEntrance = findViewById(R.id.ll__place_entrance);
    mTvEntrance = mEntrance.findViewById(R.id.tv__place_entrance);
    mTaxiShadow = findViewById(R.id.place_page_taxi_shadow);
    mTaxiDivider = findViewById(R.id.place_page_taxi_divider);
    mTaxi = findViewById(R.id.ll__place_page_taxi);
    TextView orderTaxi = mTaxi.findViewById(R.id.tv__place_page_order_taxi);
    orderTaxi.setOnClickListener(this);
    mEditPlace = findViewById(R.id.ll__place_editor);
    mEditPlace.setOnClickListener(this);
    mAddOrganisation = findViewById(R.id.ll__add_organisation);
    mAddOrganisation.setOnClickListener(this);
    mAddPlace = findViewById(R.id.ll__place_add);
    mAddPlace.setOnClickListener(this);
    mLocalAd = findViewById(R.id.ll__local_ad);
    mLocalAd.setOnClickListener(this);
    mTvLocalAd = mLocalAd.findViewById(R.id.tv__local_ad);
    mEditTopSpace = findViewById(R.id.edit_top_space);
    latlon.setOnLongClickListener(this);
    address.setOnLongClickListener(this);
    mPhone.setOnLongClickListener(this);
    mWebsite.setOnLongClickListener(this);
    mOpeningHours.setOnLongClickListener(this);
    mEmail.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mWiki.setOnLongClickListener(this);

    mBookmarkFrame = findViewById(R.id.bookmark_frame);
    mWvBookmarkNote = mBookmarkFrame.findViewById(R.id.wv__bookmark_notes);
    mWvBookmarkNote.getSettings().setJavaScriptEnabled(false);
    mTvBookmarkNote = mBookmarkFrame.findViewById(R.id.tv__bookmark_notes);
    initEditMapObjectBtn();

    mHotelMore = findViewById(R.id.ll__more);
    mHotelMore.setOnClickListener(this);

    initHotelDescriptionView();
    initHotelFacilitiesView();
    initHotelGalleryView();
    initHotelNearbyView();
    initHotelRatingView();

    mCatalogPromoController = new CatalogPromoController(this);

    mUgcController = new UGCController(this);

    mDownloaderIcon = new DownloaderStatusIcon(mPreview.findViewById(R.id.downloader_status_frame));

    mDownloaderInfo = mPreview.findViewById(R.id.tv__downloader_details);

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP)
      setElevation(UiUtils.dimen(R.dimen.placepage_elevation));

    if (UiUtils.isLandscape(getContext()))
      setBackgroundResource(0);

    Sponsored.setPriceListener(this);
    Sponsored.setInfoListener(this);

    initPlaceDescriptionView();
  }

  public void initButtons(@NonNull ViewGroup buttons)
  {
    mButtons = new PlacePageButtons(this, buttons, new PlacePageButtons.ItemListener()
    {
      public void onPrepareVisibleView(@NonNull PlacePageButtons.PlacePageButton item,
                                       @NonNull View frame, @NonNull ImageView icon,
                                       @NonNull TextView title)
      {
        int color;

        switch (item.getType())
        {
          case BOOKING:
          case BOOKING_SEARCH:
          case OPENTABLE:
            // ---------------------------------------------------------------------------------------
            // Warning: the following code is autogenerated.
            // Do NOT change it manually.
            // %PartnersExtender.Switch
          case PARTNER1:
          case PARTNER3:
          case PARTNER18:
          case PARTNER19:
          case PARTNER20:
            // /%PartnersExtender.Switch
            // End of autogenerated code.
            // ---------------------------------------------------------------------------------------
            frame.setBackgroundResource(item.getBackgroundResource());
            color = Color.WHITE;
            break;

          case PARTNER2:
            frame.setBackgroundResource(item.getBackgroundResource());
            color = Color.BLACK;
            break;

          case BOOKMARK:
            mBookmarkButtonIcon = icon;
            mBookmarkButtonFrame = frame;
            updateBookmarkButton();
            color = ThemeUtils.getColor(getContext(), R.attr.iconTint);
            break;

          default:
            color = ThemeUtils.getColor(getContext(), R.attr.iconTint);
            icon.setColorFilter(color);
            break;
        }

        title.setTextColor(color);
      }

      @Override
      public void onItemClick(PlacePageButtons.PlacePageButton item)
      {
        switch (item.getType())
        {
          case BOOKMARK:
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

          case BOOKING:
          case OPENTABLE:
            // -----------------------------------------------------------------------------------------
            // Warning: the following code is autogenerated.
            // Do NOT change it manually.
            // %PartnersExtender.SwitchClick
          case PARTNER1:
          case PARTNER2:
          case PARTNER3:
          case PARTNER18:
          case PARTNER19:
          case PARTNER20:
            // /%PartnersExtender.SwitchClick
            // End of autogenerated code.
            // -----------------------------------------------------------------------------------------
            onSponsoredClick(true /* book */, false);
            break;

          case BOOKING_SEARCH:
            onBookingSearchBtnClicked();
            break;

          case CALL:
            onCallBtnClicked();
            break;
        }
      }
    });
  }

  private void onBookmarkBtnClicked()
  {
    if (mMapObject == null)
    {
      LOGGER.e(TAG, "Bookmark cannot be managed, mMapObject is null!");
      return;
    }

    Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_BOOKMARK);
    AlohaHelper.logClick(AlohaHelper.PP_BOOKMARK);
    toggleIsBookmark(mMapObject);
  }

  private void onShareBtnClicked()
  {
    if (mMapObject == null)
    {
      LOGGER.e(TAG, "A map object cannot be shared, it's null!");
      return;
    }
    Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_SHARE);
    AlohaHelper.logClick(AlohaHelper.PP_SHARE);
    ShareOption.AnyShareOption.ANY.shareMapObject(getActivity(), mMapObject, mSponsored);
  }

  private void onBackBtnClicked()
  {
    if (mMapObject == null)
    {
      LOGGER.e(TAG, "A mwm request cannot be handled, mMapObject is null!");
      getActivity().finish();
      return;
    }

    if (ParsedMwmRequest.hasRequest())
    {
      ParsedMwmRequest request = ParsedMwmRequest.getCurrentRequest();
      if (ParsedMwmRequest.isPickPointMode())
        request.setPointData(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getTitle(), "");

      request.sendResponseAndFinish(getActivity(), true);
    }
    else
      getActivity().finish();
  }

  private void onRouteFromBtnClicked()
  {
    RoutingController controller = RoutingController.get();
    if (!controller.isPlanning())
    {
      controller.prepare(mMapObject, null);
      close();
    }
    else if (controller.setStartPoint(mMapObject))
    {
      close();
    }
  }

  private void onRouteToBtnClicked()
  {
    if (RoutingController.get().isPlanning())
    {
      RoutingController.get().setEndPoint(mMapObject);
      close();
    }
    else
    {
      getActivity().startLocationToPoint(getMapObject(), true);
    }
  }

  private void onRouteAddBtnClicked()
  {
    if (mMapObject != null)
      RoutingController.get().addStop(mMapObject);
  }

  private void onRouteRemoveBtnClicked()
  {
    if (mMapObject != null)
      RoutingController.get().removeStop(mMapObject);
  }

  private void onCallBtnClicked()
  {
    Utils.callPhone(getContext(), mTvPhone.getText().toString());
  }

  private void onBookingSearchBtnClicked()
  {
    if (mMapObject != null && !TextUtils.isEmpty(mMapObject.getBookingSearchUrl()))
    {
      Statistics.INSTANCE.trackBookingSearchEvent(mMapObject);
      Utils.openUrl(getContext(), mMapObject.getBookingSearchUrl());
    }
  }

  public void setRoutingModeListener(@Nullable RoutingModeListener routingModeListener)
  {
    mRoutingModeListener = routingModeListener;
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
    if (mRoutingModeListener == null)
      return;

    mRoutingModeListener.toggleRouteSettings(roadType);
    Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_DRIVING_OPTIONS_ACTION,
                                   Statistics.params().add(Statistics.EventParam.TYPE,
                                                           roadType.name()));
  }

  private void initPlaceDescriptionView()
  {
    mPlaceDescriptionContainer = findViewById(R.id.poi_description_container);
    mPlaceDescriptionView = findViewById(R.id.poi_description);
    mPlaceDescriptionMoreBtn = findViewById(R.id.more_btn);
    mPlaceDescriptionMoreBtn.setOnClickListener(v -> showDescriptionScreen());
  }

  private void showDescriptionScreen()
  {
    Context context = mPlaceDescriptionContainer.getContext();
    String description = Objects.requireNonNull(mMapObject).getDescription();
    PlaceDescriptionActivity.start(context, description, Statistics.ParamValue.WIKIPEDIA);
  }

  private void initEditMapObjectBtn()
  {
    boolean isEditSupported = isEditableMapObject();
    View editBookmarkBtn = mBookmarkFrame.findViewById(R.id.tv__bookmark_edit);
    UiUtils.showIf(isEditSupported, editBookmarkBtn);
    editBookmarkBtn.setOnClickListener(isEditSupported ? mEditBookmarkClickListener : null);
  }

  public boolean isEditableMapObject()
  {
    boolean isBookmark = MapObject.isOfType(MapObject.BOOKMARK, mMapObject);
    if (isBookmark)
    {
      long id = Utils.<Bookmark>castTo(mMapObject).getBookmarkId();
      return BookmarkManager.INSTANCE.isEditableBookmark(id);
    }
    return true;
  }

  private void initHotelRatingView()
  {
    mHotelReview = findViewById(R.id.ll__place_hotel_rating);
    RecyclerView rvHotelReview = findViewById(R.id.rv__place_hotel_review);
    rvHotelReview.setLayoutManager(new LinearLayoutManager(getContext()));
    rvHotelReview.getLayoutManager().setAutoMeasureEnabled(true);
    rvHotelReview.setNestedScrollingEnabled(false);
    rvHotelReview.setHasFixedSize(false);
    rvHotelReview.setAdapter(mReviewAdapter);
    mHotelRating = findViewById(R.id.tv__place_hotel_rating);
    mHotelRatingBase = findViewById(R.id.tv__place_hotel_rating_base);
    View hotelMoreReviews = findViewById(R.id.tv__place_hotel_reviews_more);
    hotelMoreReviews.setOnClickListener(this);
  }

  private void initHotelNearbyView()
  {
    mHotelNearby = findViewById(R.id.ll__place_hotel_nearby);
    GridView gvHotelNearby = findViewById(R.id.gv__place_hotel_nearby);
    gvHotelNearby.setAdapter(mNearbyAdapter);
  }

  private void initHotelGalleryView()
  {
    mHotelGallery = findViewById(R.id.ll__place_hotel_gallery);
    mRvHotelGallery = findViewById(R.id.rv__place_hotel_gallery);
    mRvHotelGallery.setNestedScrollingEnabled(false);
    LinearLayoutManager layoutManager = new LinearLayoutManager(getContext(),
                                                                LinearLayoutManager.HORIZONTAL,
                                                                false);
    mRvHotelGallery.setLayoutManager(layoutManager);
    RecyclerView.ItemDecoration decor = ItemDecoratorFactory
        .createHotelGalleryDecorator(getContext(), LinearLayoutManager.HORIZONTAL);
    mRvHotelGallery.addItemDecoration(decor);
    mGalleryAdapter.setListener(this);
    mRvHotelGallery.setAdapter(mGalleryAdapter);
  }

  private void initHotelFacilitiesView()
  {
    mHotelFacilities = findViewById(R.id.ll__place_hotel_facilities);
    RecyclerView rvHotelFacilities = findViewById(R.id.rv__place_hotel_facilities);
    rvHotelFacilities.setLayoutManager(new GridLayoutManager(getContext(), 2));
    rvHotelFacilities.getLayoutManager().setAutoMeasureEnabled(true);
    rvHotelFacilities.setNestedScrollingEnabled(false);
    rvHotelFacilities.setHasFixedSize(false);
    mHotelMoreFacilities = findViewById(R.id.tv__place_hotel_facilities_more);
    rvHotelFacilities.setAdapter(mFacilitiesAdapter);
    mHotelMoreFacilities.setOnClickListener(this);
  }

  private void initHotelDescriptionView()
  {
    mHotelDescription = findViewById(R.id.ll__place_hotel_description);
    mTvHotelDescription = findViewById(R.id.tv__place_hotel_details);
    mHotelMoreDescription = findViewById(R.id.tv__place_hotel_more);
    View hotelMoreDescriptionOnWeb = findViewById(R.id.tv__place_hotel_more_on_web);
    mTvHotelDescription.setListener(this);
    mHotelMoreDescription.setOnClickListener(this);
    hotelMoreDescriptionOnWeb.setOnClickListener(this);
  }

  @Override
  public void onPriceReceived(@NonNull HotelPriceInfo priceInfo)
  {
    if (mSponsored == null || !TextUtils.equals(priceInfo.getId(), mSponsored.getId()))
      return;

    String price = makePrice(getContext(), priceInfo);
    if (price != null)
      mSponsoredPrice = price;

    if (mMapObject == null)
    {
      LOGGER.e(TAG, "A sponsored info cannot be updated, mMapObject is null!");
      return;
    }
    refreshPreview(mMapObject, priceInfo);
  }

  @Nullable
  private static String makePrice(@NonNull Context context, @NonNull HotelPriceInfo priceInfo)
  {
    if (TextUtils.isEmpty(priceInfo.getPrice()) || TextUtils.isEmpty(priceInfo.getCurrency()))
      return null;

    String text = Utils.formatCurrencyString(priceInfo.getPrice(), priceInfo.getCurrency());
    return context.getString(R.string.place_page_starting_from, text);
  }

  @Override
  public void onHotelInfoReceived(@NonNull String id, @NonNull Sponsored.HotelInfo info)
  {
    if (mSponsored == null || !TextUtils.equals(id, mSponsored.getId()))
      return;

    updateHotelDetails(info);
    updateHotelFacilities(info);
    updateHotelGallery(info);
    updateHotelNearby(info);
    updateHotelRating(info);
  }

  private void updateHotelRating(@NonNull Sponsored.HotelInfo info)
  {
    if (info.mReviews == null || info.mReviews.length == 0)
    {
      UiUtils.hide(mHotelReview);
    }
    else
    {
      UiUtils.show(mHotelReview);
      mReviewAdapter.setItems(new ArrayList<>(Arrays.asList(info.mReviews)));
      //noinspection ConstantConditions
      mHotelRating.setText(mSponsored.getRating());
      int reviewsCount = (int) info.mReviewsAmount;
      String text = getResources().getQuantityString(
          R.plurals.placepage_summary_rating_description, reviewsCount, reviewsCount);
      mHotelRatingBase.setText(text);
      TextView previewReviewCountView = mPreviewRatingInfo.findViewById(R.id.tv__review_count);
      previewReviewCountView.setText(text);
    }
  }

  private void updateHotelNearby(@NonNull Sponsored.HotelInfo info)
  {
    if (info.mNearby == null || info.mNearby.length == 0)
    {
      UiUtils.hide(mHotelNearby);
    }
    else
    {
      UiUtils.show(mHotelNearby);
      mNearbyAdapter.setItems(Arrays.asList(info.mNearby));
    }
  }

  private void updateHotelGallery(@NonNull Sponsored.HotelInfo info)
  {
    if (info.mPhotos == null || info.mPhotos.length == 0)
    {
      UiUtils.hide(mHotelGallery);
    }
    else
    {
      UiUtils.show(mHotelGallery);
      mGalleryAdapter.setItems(new ArrayList<>(Arrays.asList(info.mPhotos)));
      mRvHotelGallery.scrollToPosition(0);
    }
  }

  private void updateHotelFacilities(@NonNull Sponsored.HotelInfo info)
  {
    if (info.mFacilities == null || info.mFacilities.length == 0)
    {
      UiUtils.hide(mHotelFacilities);
    }
    else
    {
      UiUtils.show(mHotelFacilities);
      mFacilitiesAdapter.setShowAll(false);
      mFacilitiesAdapter.setItems(Arrays.asList(info.mFacilities));
      mHotelMoreFacilities.setVisibility(info.mFacilities.length > FacilitiesAdapter.MAX_COUNT
                                         ? VISIBLE : GONE);
    }
  }

  private void updateHotelDetails(@NonNull Sponsored.HotelInfo info)
  {
    mTvHotelDescription.setMaxLines(getResources().getInteger(R.integer.pp_hotel_description_lines));
    refreshMetadataOrHide(info.mDescription, mHotelDescription, mTvHotelDescription);
    mHotelMoreDescription.setVisibility(GONE);
  }

  private void clearHotelViews()
  {
    mTvHotelDescription.setText("");
    mHotelMoreDescription.setVisibility(GONE);
    mFacilitiesAdapter.setItems(Collections.emptyList());
    mHotelMoreFacilities.setVisibility(GONE);
    mGalleryAdapter.setItems(new ArrayList<>());
    mNearbyAdapter.setItems(Collections.emptyList());
    mReviewAdapter.setItems(new ArrayList<Review>());
    mHotelRating.setText("");
    mHotelRatingBase.setText("");
    mTvSponsoredPrice.setText("");
    mGalleryAdapter.setItems(new ArrayList<>());
  }

  @Override
  public void onLineCountCalculated(boolean grater)
  {
    UiUtils.showIf(grater, mHotelMoreDescription);
  }

  @Override
  public void onItemClick(View v, int position)
  {
    if (mMapObject == null || mSponsored == null)
    {
      LOGGER.e(TAG, "A photo gallery cannot be started, mMapObject/mSponsored is null!");
      return;
    }

    Statistics.INSTANCE.trackHotelEvent(PP_HOTEL_GALLERY_OPEN, mSponsored, mMapObject);

    if (position == com.mapswithme.maps.widget.placepage.GalleryAdapter.MAX_COUNT - 1
        && mGalleryAdapter.getItems().size() > com.mapswithme.maps.widget.placepage.GalleryAdapter.MAX_COUNT)
    {
      GalleryActivity.start(getContext(), mGalleryAdapter.getItems(), mMapObject.getTitle());
    }
    else
    {
      FullScreenGalleryActivity.start(getContext(), mGalleryAdapter.getItems(), position);
    }
  }

  @Override
  public void onItemClick(@NonNull Sponsored.NearbyObject item)
  {
//  TODO go to selected object on map
  }

  private void onSponsoredClick(final boolean book, final boolean isDetails)
  {
    Utils.checkConnection(
        getActivity(), R.string.common_check_internet_connection_dialog, new Utils.Proc<Boolean>()
        {
          @Override
          public void invoke(@NonNull Boolean result)
          {
            if (!result)
              return;

            Sponsored info = mSponsored;
            if (info == null)
              return;

            Utils.PartnerAppOpenMode partnerAppOpenMode = Utils.PartnerAppOpenMode.None;

            switch (info.getType())
            {
              case Sponsored.TYPE_BOOKING:
                if (mMapObject == null)
                  break;

                if (book)
                {
                  partnerAppOpenMode = Utils.PartnerAppOpenMode.Direct;
                  UserActionsLogger.logBookingBookClicked();
                  Statistics.INSTANCE.trackBookHotelEvent(info, mMapObject);
                }
                else if (isDetails)
                {
                  UserActionsLogger.logBookingDetailsClicked();
                  Statistics.INSTANCE.trackHotelEvent(PP_SPONSORED_DETAILS, info, mMapObject);
                }
                else
                {
                  UserActionsLogger.logBookingMoreClicked();
                  Statistics.INSTANCE.trackHotelEvent(PP_HOTEL_DESCRIPTION_LAND, info, mMapObject);
                }
                break;
              case Sponsored.TYPE_OPENTABLE:
                if (mMapObject != null)
                  Statistics.INSTANCE.trackRestaurantEvent(PP_SPONSORED_OPENTABLE, info,
                                                           mMapObject);
                break;
              case Sponsored.TYPE_PARTNER:
                if (mMapObject != null && !info.getPartnerName().isEmpty())
                  Statistics.INSTANCE.trackSponsoredObjectEvent(PP_SPONSORED_ACTION, info,
                                                                mMapObject);
                break;
              case Sponsored.TYPE_NONE:
                break;
            }

            try
            {
              if (partnerAppOpenMode != Utils.PartnerAppOpenMode.None)
              {
                SponsoredLinks links = new SponsoredLinks(info.getDeepLink(), info.getUrl());
                String packageName = Sponsored.getPackageName(info.getType());

                Utils.openPartner(getContext(), links, packageName, partnerAppOpenMode);
              }
              else
              {
                if (book)
                  Utils.openUrl(getContext(), info.getUrl());
                else
                  Utils.openUrl(getContext(), isDetails ? info.getDescriptionUrl()
                                                        : info.getMoreUrl());
              }
            }
            catch (ActivityNotFoundException e)
            {
              LOGGER.e(TAG, "Failed to handle click on sponsored: ", e);
              AlohaHelper.logException(e);
            }
          }
        });
  }

  private void init(AttributeSet attrs, int defStyleAttr)
  {
    LayoutInflater.from(getContext()).inflate(R.layout.place_page, this);

    if (isInEditMode())
      return;

    final TypedArray attrArray = getContext().obtainStyledAttributes(attrs, R.styleable.PlacePageView, defStyleAttr, 0);
    final int animationType = attrArray.getInt(R.styleable.PlacePageView_animationType, 0);
    mIsDocked = attrArray.getBoolean(R.styleable.PlacePageView_docked, false);
    mIsFloating = attrArray.getBoolean(R.styleable.PlacePageView_floating, false);
    attrArray.recycle();
  }

  public boolean isDocked()
  {
    return mIsDocked;
  }

  public boolean isFloating()
  {
    return mIsFloating;
  }

  @Nullable
  public MapObject getMapObject()
  {
    return mMapObject;
  }

  /**
   * @param mapObject new MapObject
   * @param listener  listener
   */
  public void setMapObject(@Nullable MapObject mapObject, @Nullable final SetMapObjectListener listener)
  {
    if (MapObject.same(mMapObject, mapObject))
    {
      mMapObject = mapObject;
      NetworkPolicy policy = NetworkPolicy.newInstance(NetworkPolicy.getCurrentNetworkUsageStatus());
      refreshViews(policy);
      if (listener != null)
      {
        listener.onSetMapObjectComplete(policy, true);
      }
      return;
    }

    mMapObject = mapObject;
    mSponsored = (mMapObject == null ? null : Sponsored.nativeGetCurrent());

    if (isNetworkNeeded())
    {
      NetworkPolicy.checkNetworkPolicy(getActivity().getSupportFragmentManager(),
                                       new NetworkPolicy.NetworkPolicyListener()
      {
        @Override
        public void onResult(@NonNull NetworkPolicy policy)
        {
          setMapObjectInternal(policy);
          if (listener != null)
            listener.onSetMapObjectComplete(policy, false);
        }
      });
    }
    else
    {
      NetworkPolicy policy = NetworkPolicy.newInstance(false);
      setMapObjectInternal(policy);
      if (listener != null)
        listener.onSetMapObjectComplete(policy, false);
    }
  }

  private void setMapObjectInternal(@NonNull NetworkPolicy policy)
  {
    detachCountry();
    if (mMapObject != null)
    {
      clearHotelViews();
      processSponsored(policy);
      initEditMapObjectBtn();
      mUgcController.clearViewsFor(mMapObject);

      String country = MapManager.nativeGetSelectedCountry();
      if (country != null && !RoutingController.get().isNavigating())
        attachCountry(country);
    }
    refreshViews(policy);
  }

  private void processSponsored(@NonNull NetworkPolicy policy)
  {
    if (mSponsored == null || mMapObject == null)
      return;

    mSponsored.updateId(mMapObject);
    mSponsoredPrice = mSponsored.getPrice();
    String currencyCode = Utils.getCurrencyCode();

    if (mSponsored.getId() == null || TextUtils.isEmpty(currencyCode))
      return;

    if (mSponsored.getType() != Sponsored.TYPE_BOOKING)
      return;

    Sponsored.requestPrice(mSponsored.getId(), currencyCode, policy);
    Sponsored.requestInfo(mSponsored, Locale.getDefault().toString(), policy);
  }

  private boolean isNetworkNeeded()
  {
    return mMapObject != null && (isSponsored() || mMapObject.getBanners() != null);
  }

  void refreshViews(@NonNull NetworkPolicy policy)
  {
    if (mMapObject == null)
    {
      LOGGER.e(TAG, "A place page views cannot be refreshed, mMapObject is null");
      return;
    }

    refreshPreview(mMapObject, null);
    refreshDetails(mMapObject);
    refreshHotelDetailViews(policy);
    refreshViewsInternal(mMapObject);
    mUgcController.getUGC(mMapObject);
    mCatalogPromoController.updateCatalogPromo(policy, mMapObject);
  }

  private void refreshViewsInternal(@NonNull MapObject mapObject)
  {
    final Location loc = LocationHelper.INSTANCE.getSavedLocation();
    switch (mapObject.getMapObjectType())
    {
      case MapObject.BOOKMARK:
        refreshDistanceToObject(mapObject, loc);
        showBookmarkDetails(mapObject);
        updateBookmarkButton();
        setButtons(mapObject, false, true);
        break;
      case MapObject.POI:
      case MapObject.SEARCH:
        refreshDistanceToObject(mapObject, loc);
        hideBookmarkDetails();
        setButtons(mapObject, false, true);
        setPlaceDescription(mapObject);
        break;
      case MapObject.API_POINT:
        refreshDistanceToObject(mapObject, loc);
        hideBookmarkDetails();
        setButtons(mapObject, true, true);
        break;
      case MapObject.MY_POSITION:
        refreshMyPosition(mapObject, loc);
        hideBookmarkDetails();
        setButtons(mapObject, false, false);
        break;
    }
  }

  private void setPlaceDescription(@NonNull MapObject mapObject)
  {
    if (TextUtils.isEmpty(mapObject.getDescription()))
    {
      UiUtils.hide(mPlaceDescriptionContainer);
      return;
    }

    if (MapObject.isOfType(MapObject.BOOKMARK, mapObject))
    {
      Bookmark bmk = (Bookmark) mapObject;
      if (!TextUtils.isEmpty(bmk.getBookmarkDescription()))
      {
        UiUtils.hide(mPlaceDescriptionContainer);
        return;
      }
    }

    UiUtils.show(mPlaceDescriptionContainer);
    mPlaceDescriptionView.setText(Html.fromHtml(mapObject.getDescription()));
  }

  private void colorizeSubtitle()
  {
    String text = mTvSubtitle.getText().toString();
    if (TextUtils.isEmpty(text))
      return;

    int start = text.indexOf("★");
    if (start > -1)
    {
      SpannableStringBuilder sb = new SpannableStringBuilder(text);
      sb.setSpan(new ForegroundColorSpan(getResources().getColor(R.color.base_yellow)),
                 start, sb.length(), Spanned.SPAN_INCLUSIVE_EXCLUSIVE);

      mTvSubtitle.setText(sb);
    }
  }

  private void refreshPreview(@NonNull MapObject mapObject, @Nullable HotelPriceInfo priceInfo)
  {
    UiUtils.setTextAndHideIfEmpty(mTvTitle, mapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSecondaryTitle, mapObject.getSecondaryTitle());
    boolean isPopular = mapObject.getPopularity().getType() == Popularity.Type.POPULAR;
    UiUtils.showIf(isPopular, mPopularityView);
    if (mToolbar != null)
      mToolbar.setTitle(mapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, mapObject.getSubtitle());
    colorizeSubtitle();
    UiUtils.hide(mAvDirection);
    UiUtils.setTextAndHideIfEmpty(mTvAddress, mapObject.getAddress());
    boolean sponsored = isSponsored();
    UiUtils.showIf(sponsored || mapObject.shouldShowUGC(), mPreviewRatingInfo);
    UiUtils.showIf(sponsored, mHotelDiscount);
    UiUtils.showIf(mapObject.getHotelType() != null, mPreview, R.id.search_hotels_btn);
    if (sponsored)
      refreshSponsoredViews(mapObject, priceInfo);
  }

  private void refreshSponsoredViews(@NonNull MapObject mapObject,
                                     @Nullable HotelPriceInfo priceInfo)
  {
    boolean isPriceEmpty = TextUtils.isEmpty(mSponsoredPrice);
    @SuppressWarnings("ConstantConditions")
    boolean isRatingEmpty = TextUtils.isEmpty(mSponsored.getRating());
    Impress impress = Impress.values()[mSponsored.getImpress()];
    mRatingView.setRating(impress, mSponsored.getRating());
    UiUtils.showIf(!isRatingEmpty, mRatingView);
    mTvSponsoredPrice.setText(mSponsoredPrice);
    UiUtils.showIf(!isPriceEmpty, mTvSponsoredPrice);
    boolean isBookingInfoExist = (!isRatingEmpty || !isPriceEmpty) &&
                                 mSponsored.getType() == Sponsored.TYPE_BOOKING;
    UiUtils.showIf(isBookingInfoExist || mapObject.shouldShowUGC(), mPreviewRatingInfo);
    String discount = getHotelDiscount(priceInfo);
    UiUtils.hideIf(TextUtils.isEmpty(discount), mHotelDiscount);
    mHotelDiscount.setRating(Impress.DISCOUNT, discount);
  }

  @Nullable
  private String getHotelDiscount(@Nullable HotelPriceInfo priceInfo)
  {
    boolean hasPercentsDiscount = priceInfo != null && priceInfo.getDiscount() > 0;
    if (hasPercentsDiscount)
      return DISCOUNT_PREFIX + priceInfo.getDiscount() + DISCOUNT_SUFFIX;

    return priceInfo != null && priceInfo.hasSmartDeal() ? DISCOUNT_SUFFIX : null;
  }

  private boolean isSponsored()
  {
    return mSponsored != null && mSponsored.getType() != Sponsored.TYPE_NONE;
  }

  @Nullable
  public Sponsored getSponsored()
  {
    return mSponsored;
  }

  private void refreshDetails(@NonNull MapObject mapObject)
  {
    refreshLatLon(mapObject);

    if (mSponsored == null || mSponsored.getType() != Sponsored.TYPE_BOOKING)
    {
      String website = mapObject.getMetadata(Metadata.MetadataType.FMD_WEBSITE);
      String url = mapObject.getMetadata(Metadata.MetadataType.FMD_URL);
      refreshMetadataOrHide(TextUtils.isEmpty(website) ? url : website, mWebsite, mTvWebsite);
    }
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER), mPhone, mTvPhone);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_EMAIL), mEmail, mTvEmail);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR), mOperator, mTvOperator);
    refreshMetadataOrHide(Framework.nativeGetActiveObjectFormattedCuisine(), mCuisine, mTvCuisine);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA), mWiki, null);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_INTERNET), mWifi, null);
    refreshMetadataOrHide(mapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    refreshOpeningHours(mapObject);

    showTaxiOffer(mapObject);

    if (RoutingController.get().isNavigating() || RoutingController.get().isPlanning())
    {
      UiUtils.hide(mEditPlace, mAddOrganisation, mAddPlace, mLocalAd, mEditTopSpace);
    }
    else
    {
      UiUtils.showIf(Editor.nativeShouldShowEditPlace(), mEditPlace);
      UiUtils.showIf(Editor.nativeShouldShowAddBusiness(), mAddOrganisation);
      UiUtils.showIf(Editor.nativeShouldShowAddPlace(), mAddPlace);
      UiUtils.showIf(UiUtils.isVisible(mEditPlace)
                     || UiUtils.isVisible(mAddOrganisation)
                     || UiUtils.isVisible(mAddPlace), mEditTopSpace);
      refreshLocalAdInfo(mapObject);
    }
    setPlaceDescription(mapObject);
  }

  private void refreshHotelDetailViews(@NonNull NetworkPolicy policy)
  {
    if (mSponsored == null)
    {
      hideHotelDetailViews();
      return;
    }

    boolean isConnected = ConnectionState.isConnected();
    if (isConnected && policy.canUseNetwork())
      showHotelDetailViews();
    else
      hideHotelDetailViews();

    if (mSponsored.getType() == Sponsored.TYPE_BOOKING)
    {
      UiUtils.hide(mWebsite);
      UiUtils.show(mHotelMore);
    }

    if (mSponsored.getType() != Sponsored.TYPE_BOOKING)
      hideHotelDetailViews();
  }

  private void showTaxiOffer(@NonNull MapObject mapObject)
  {
    List<TaxiType> taxiTypes = mapObject.getReachableByTaxiTypes();

    boolean showTaxiOffer = taxiTypes != null && !taxiTypes.isEmpty() &&
                            LocationHelper.INSTANCE.getMyPosition() != null &&
                            ConnectionState.isConnected()
                            && mapObject.getRoadWarningMarkType() == RoadWarningMarkType.UNKNOWN;
    UiUtils.showIf(showTaxiOffer, mTaxi, mTaxiShadow, mTaxiDivider);

    if (!showTaxiOffer)
      return;

    // At this moment we display only a one taxi provider at the same time.
    TaxiType type = taxiTypes.get(0);
    ImageView logo = mTaxi.findViewById(R.id.iv__place_page_taxi);
    logo.setImageResource(type.getIcon());
    TextView title = mTaxi.findViewById(R.id.tv__place_page_taxi);
    title.setText(type.getTitle());
  }

  private void hideHotelDetailViews()
  {
    UiUtils.hide(mHotelDescription, mHotelFacilities, mHotelGallery, mHotelNearby,
                 mHotelReview, mHotelMore);
  }

  private void showHotelDetailViews()
  {
    UiUtils.show(mHotelDescription, mHotelFacilities, mHotelGallery, mHotelNearby,
                 mHotelReview, mHotelMore);
  }

  private void refreshLocalAdInfo(@NonNull MapObject mapObject)
  {
    LocalAdInfo localAdInfo = mapObject.getLocalAdInfo();
    boolean isLocalAdAvailable = localAdInfo != null && localAdInfo.isAvailable();
    if (isLocalAdAvailable && !TextUtils.isEmpty(localAdInfo.getUrl()) && !localAdInfo.isHidden())
    {
      mTvLocalAd.setText(localAdInfo.isCustomer() ? R.string.view_campaign_button
                                                  : R.string.create_campaign_button);
      UiUtils.show(mLocalAd);
    }
    else
    {
      UiUtils.hide(mLocalAd);
    }
  }

  private void refreshOpeningHours(@NonNull MapObject mapObject)
  {
    final Timetable[] timetables = OpeningHours.nativeTimetablesFromString(mapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS));
    if (timetables == null || timetables.length == 0)
    {
      UiUtils.hide(mOpeningHours);
      return;
    }

    UiUtils.show(mOpeningHours);

    final Resources resources = getResources();
    if (timetables[0].isFullWeek())
    {
      refreshTodayOpeningHours((timetables[0].isFullday ? resources.getString(R.string.twentyfour_seven)
                                                        : resources.getString(R.string.daily) + " " + timetables[0].workingTimespan),
                               ThemeUtils.getColor(getContext(), android.R.attr.textColorPrimary));
      UiUtils.clearTextAndHide(mFullOpeningHours);
      return;
    }

    boolean containsCurrentWeekday = false;
    final int currentDay = Calendar.getInstance().get(Calendar.DAY_OF_WEEK);
    for (Timetable tt : timetables)
    {
      if (tt.containsWeekday(currentDay))
      {
        containsCurrentWeekday = true;
        String workingTime;

        if (tt.isFullday)
        {
          String allDay = resources.getString(R.string.editor_time_allday);
          workingTime = Utils.unCapitalize(allDay);
        }
        else
        {
          workingTime = tt.workingTimespan.toString();
        }

        refreshTodayOpeningHours(resources.getString(R.string.today) + " " + workingTime,
                                 ThemeUtils.getColor(getContext(), android.R.attr.textColorPrimary));

        break;
      }
    }

    UiUtils.setTextAndShow(mFullOpeningHours, TimeFormatUtils.formatTimetables(timetables));
    if (!containsCurrentWeekday)
      refreshTodayOpeningHours(resources.getString(R.string.day_off_today), resources.getColor(R.color.base_red));
  }

  private void refreshTodayOpeningHours(String text, @ColorInt int color)
  {
    UiUtils.setTextAndShow(mTodayOpeningHours, text);
    mTodayOpeningHours.setTextColor(color);
  }

  private void updateBookmarkButton()
  {
    if (mBookmarkButtonIcon == null || mBookmarkButtonFrame == null)
      return;

    if (mBookmarkSet)
      mBookmarkButtonIcon.setImageResource(R.drawable.ic_bookmarks_on);
    else
      mBookmarkButtonIcon.setImageDrawable(Graphics.tint(getContext(), R.drawable.ic_bookmarks_off, R.attr.iconTint));

    boolean isEditable = isEditableMapObject();
    mBookmarkButtonFrame.setEnabled(isEditable);

    if (isEditable)
      return;
    final int resId = PlacePageButtons.Item.BOOKMARK.getIcon().getDisabledStateResId();
    Drawable drawable = Graphics.tint(getContext(), resId, R.attr.iconTintDisabled);
    mBookmarkButtonIcon.setImageDrawable(drawable);
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
      mWvBookmarkNote.loadData(notes, "text/html; charset=utf-8", null);
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

  @NonNull
  private static PlacePageButtons.Item toPlacePageButton(@NonNull RoadWarningMarkType type)
  {
    switch (type)
    {
      case DIRTY:
        return PlacePageButtons.Item.ROUTE_AVOID_UNPAVED;
      case FERRY:
        return PlacePageButtons.Item.ROUTE_AVOID_FERRY;
      case TOLL:
        return PlacePageButtons.Item.ROUTE_AVOID_TOLL;
      default:
        throw new AssertionError("Unsupported road warning type: " + type);
    }
  }

  private void setButtons(@NonNull MapObject mapObject, boolean showBackButton, boolean showRoutingButton)
  {
    List<PlacePageButtons.PlacePageButton> buttons = new ArrayList<>();

    if (mapObject.getRoadWarningMarkType() != RoadWarningMarkType.UNKNOWN)
    {
      RoadWarningMarkType markType = mapObject.getRoadWarningMarkType();
      PlacePageButtons.Item roadType = toPlacePageButton(markType);
      buttons.add(roadType);
      mButtons.setItems(buttons);
      return;
    }

    if (RoutingController.get().isRoutePoint(mapObject))
    {
      buttons.add(PlacePageButtons.Item.ROUTE_REMOVE);
      mButtons.setItems(buttons);
      return;
    }

    if (showBackButton || ParsedMwmRequest.isPickPointMode())
      buttons.add(PlacePageButtons.Item.BACK);

    if (mSponsored != null)
    {
      switch (mSponsored.getType())
      {
        case Sponsored.TYPE_BOOKING:
          buttons.add(BOOKING);
          break;
        case Sponsored.TYPE_OPENTABLE:
          buttons.add(PlacePageButtons.Item.OPENTABLE);
          break;
        case Sponsored.TYPE_PARTNER:
          int partnerIndex = mSponsored.getPartnerIndex();
          if (partnerIndex >= 0 && !mSponsored.getUrl().isEmpty())
            buttons.add(PlacePageButtons.getPartnerItem(partnerIndex));
          break;
        case Sponsored.TYPE_NONE:
          break;
      }
    }

    if (!TextUtils.isEmpty(mapObject.getBookingSearchUrl()))
      buttons.add(PlacePageButtons.Item.BOOKING_SEARCH);

    if (mapObject.hasPhoneNumber())
      buttons.add(PlacePageButtons.Item.CALL);

    buttons.add(PlacePageButtons.Item.BOOKMARK);

    if (RoutingController.get().isPlanning() || showRoutingButton)
    {
      buttons.add(PlacePageButtons.Item.ROUTE_FROM);
      buttons.add(PlacePageButtons.Item.ROUTE_TO);
      if (RoutingController.get().isStopPointAllowed())
        buttons.add(PlacePageButtons.Item.ROUTE_ADD);
    }

    buttons.add(PlacePageButtons.Item.SHARE);

    mButtons.setItems(buttons);
  }

  public void refreshLocation(Location l)
  {
    if (mMapObject == null)
    {
      LOGGER.e(TAG, "A location cannot be refreshed, mMapObject is null!");
      return;
    }

    if (MapObject.isOfType(MapObject.MY_POSITION, mMapObject))
      refreshMyPosition(mMapObject, l);
    else
      refreshDistanceToObject(mMapObject, l);
  }

  private void refreshMyPosition(@NonNull MapObject mapObject, Location l)
  {
    UiUtils.hide(mTvDistance);

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

    mTvDistance.setVisibility(View.VISIBLE);
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
    final String[] latLon = Framework.nativeFormatLatLonToArr(lat, lon, mIsLatLonDms);
    if (latLon.length == 2)
      mTvLatlon.setText(String.format(Locale.US, "%1$s, %2$s", latLon[0], latLon[1]));
  }

  private static void refreshMetadataOrHide(String metadata, View metaLayout, TextView metaTv)
  {
    if (!TextUtils.isEmpty(metadata))
    {
      metaLayout.setVisibility(View.VISIBLE);
      if (metaTv != null)
        metaTv.setText(metadata);
    }
    else
      metaLayout.setVisibility(View.GONE);
  }

  public void refreshAzimuth(double northAzimuth)
  {
    if (mMapObject == null || MapObject.isOfType(MapObject.MY_POSITION, mMapObject))
      return;

    final Location location = LocationHelper.INSTANCE.getSavedLocation();
    if (location == null)
      return;

    final double azimuth = Framework.nativeGetDistanceAndAzimuthFromLatLon(mMapObject.getLat(),
                                                                           mMapObject.getLon(),
                                                                           location.getLatitude(),
                                                                           location.getLongitude(),
                                                                           northAzimuth).getAzimuth();
    if (azimuth >= 0)
    {
      UiUtils.show(mAvDirection);
      mAvDirection.setAzimuth(azimuth);
    }
  }

  private void addOrganisation()
  {
    Statistics.INSTANCE.trackEvent(Statistics.EventName.EDITOR_ADD_CLICK,
                                   Statistics.params()
                                             .add(Statistics.EventParam.FROM, "placepage"));
    getActivity().showPositionChooser(true, false);
  }

  private void addPlace()
  {
    // TODO add statistics
    getActivity().showPositionChooser(false, true);
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
      case R.id.ll__place_editor:
        if (mMapObject == null)
        {
          LOGGER.e(TAG, "Cannot start editor, map object is null!");
          break;
        }
        getActivity().showEditor();
        break;
      case R.id.ll__add_organisation:
        addOrganisation();
        break;
      case R.id.ll__place_add:
        addPlace();
        break;
      case R.id.ll__local_ad:
        if (mMapObject != null)
        {
          LocalAdInfo localAdInfo = mMapObject.getLocalAdInfo();
          if (localAdInfo == null)
            throw new AssertionError("A local ad must be non-null if button is shown!");

          if (!TextUtils.isEmpty(localAdInfo.getUrl()))
          {
            Statistics.INSTANCE.trackPPOwnershipButtonClick(mMapObject);
            Utils.openUrl(getContext(), localAdInfo.getUrl());
          }
        }
        break;
      case R.id.ll__more:
        onSponsoredClick(false /* book */, true /* isMoreDetails */);
        break;
      case R.id.tv__place_hotel_more_on_web:
        onSponsoredClick(false /* book */, false /* isMoreDetails */);
        break;
      case R.id.ll__place_latlon:
        mIsLatLonDms = !mIsLatLonDms;
        MwmApplication.prefs().edit().putBoolean(PREF_USE_DMS, mIsLatLonDms).apply();
        if (mMapObject == null)
        {
          LOGGER.e(TAG, "A LatLon cannot be refreshed, mMapObject is null");
          break;
        }
        refreshLatLon(mMapObject);
        break;
      case R.id.ll__place_phone:
        Utils.callPhone(getContext(), mTvPhone.getText().toString());
        if (mMapObject != null)
          Framework.logLocalAdsEvent(Framework.LocalAdsEventType.LOCAL_ADS_EVENT_CLICKED_PHONE, mMapObject);
        break;
      case R.id.ll__place_website:
        Utils.openUrl(getContext(), mTvWebsite.getText().toString());
        if (mMapObject != null)
          Framework.logLocalAdsEvent(Framework.LocalAdsEventType.LOCAL_ADS_EVENT_CLICKED_WEBSITE, mMapObject);
        break;
      case R.id.ll__place_wiki:
        // TODO: Refactor and use separate getters for Wiki and all other PP meta info too.
        if (mMapObject == null)
        {
          LOGGER.e(TAG, "Cannot follow url, mMapObject is null!");
          break;
        }
        Utils.openUrl(getContext(), mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA));
        break;
      case R.id.direction_frame:
        Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_DIRECTION_ARROW);
        AlohaHelper.logClick(AlohaHelper.PP_DIRECTION_ARROW);
        showBigDirection();
        break;
      case R.id.ll__place_email:
        Utils.sendTo(getContext(), mTvEmail.getText().toString());
        break;
      case R.id.tv__place_hotel_more:
        UiUtils.hide(mHotelMoreDescription);
        mTvHotelDescription.setMaxLines(Integer.MAX_VALUE);
        break;
      case R.id.tv__place_hotel_facilities_more:
        if (mSponsored != null && mMapObject != null)
          Statistics.INSTANCE.trackHotelEvent(PP_HOTEL_FACILITIES, mSponsored, mMapObject);
        UiUtils.hide(mHotelMoreFacilities);
        mFacilitiesAdapter.setShowAll(true);
        break;
      case R.id.tv__place_hotel_reviews_more:
        if (isSponsored())
        {
          //null checking is done in 'isSponsored' method
          //noinspection ConstantConditions
          Utils.openUrl(getContext(), mSponsored.getReviewUrl());
          UserActionsLogger.logBookingReviewsClicked();
          if (mMapObject != null)
            Statistics.INSTANCE.trackHotelEvent(PP_HOTEL_REVIEWS_LAND, mSponsored, mMapObject);
        }
        break;
      case R.id.tv__place_page_order_taxi:
        RoutingController.get().prepare(LocationHelper.INSTANCE.getMyPosition(), mMapObject,
                                        Framework.ROUTER_TYPE_TAXI);
        close();
        if (mMapObject != null)
        {
          List<TaxiType> types = mMapObject.getReachableByTaxiTypes();
          if (types != null && !types.isEmpty())
          {
            String providerName = types.get(0).getProviderName();
            Statistics.INSTANCE.trackTaxiEvent(Statistics.EventName.ROUTING_TAXI_CLICK_IN_PP,
                                               providerName);
          }
        }
        break;
      case R.id.search_hotels_btn:
        if (mMapObject == null)
          break;

        @FilterUtils.RatingDef
        int filterRating = mSponsored != null ? Framework.getFilterRating(mSponsored.getRating())
                           : FilterUtils.RATING_ANY;
        HotelsFilter filter = FilterUtils.createHotelFilter(filterRating,
                                                            mMapObject.getPriceRate(),
                                                            mMapObject.getHotelType());
        getActivity().onSearchSimilarHotels(filter);
        String provider = mSponsored != null && mSponsored.getType() == Sponsored.TYPE_BOOKING
                          ? Statistics.ParamValue.BOOKING_COM : Statistics.ParamValue.OSM;
        Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_HOTEL_SEARCH_SIMILAR,
                                       Statistics.params().add(Statistics.EventParam.PROVIDER,
                                                               provider));
        break;
    }
  }

  private void toggleIsBookmark(@NonNull MapObject mapObject)
  {
    if (MapObject.isOfType(MapObject.BOOKMARK, mapObject))
      setMapObject(Framework.nativeDeleteBookmarkFromMapObject(), null);
    else
      setMapObject(BookmarkManager.INSTANCE.addNewBookmark(mapObject.getLat(), mapObject.getLon()), null);
  }

  private void showBigDirection()
  {
    final DirectionFragment fragment = (DirectionFragment) Fragment.instantiate(getActivity(), DirectionFragment.class
        .getName(), null);
    fragment.setMapObject(mMapObject);
    fragment.show(getActivity().getSupportFragmentManager(), null);
  }

  @Override
  public boolean onLongClick(View v)
  {
    final Object tag = v.getTag();
    final String tagStr = tag == null ? "" : tag.toString();
    AlohaHelper.logLongClick(tagStr);

    final PopupMenu popup = new PopupMenu(getContext(), v);
    final Menu menu = popup.getMenu();
    final List<String> items = new ArrayList<>();
    switch (v.getId())
    {
      case R.id.ll__place_latlon:
        if (mMapObject == null)
        {
          LOGGER.e(TAG, "A long click tap on LatLon cannot be handled, mMapObject is null!");
          break;
        }
        final double lat = mMapObject.getLat();
        final double lon = mMapObject.getLon();
        items.add(Framework.nativeFormatLatLon(lat, lon, false));
        items.add(Framework.nativeFormatLatLon(lat, lon, true));
        break;
      case R.id.ll__place_website:
        items.add(mTvWebsite.getText().toString());
        break;
      case R.id.ll__place_email:
        items.add(mTvEmail.getText().toString());
        break;
      case R.id.ll__place_phone:
        items.add(mTvPhone.getText().toString());
        break;
      case R.id.ll__place_schedule:
        String text = UiUtils.isVisible(mFullOpeningHours)
                      ? mFullOpeningHours.getText().toString()
                      : mTodayOpeningHours.getText().toString();
        items.add(text);
        break;
      case R.id.ll__place_operator:
        items.add(mTvOperator.getText().toString());
        break;
      case R.id.ll__place_wiki:
        if (mMapObject == null)
        {
          LOGGER.e(TAG, "A long click tap on wiki cannot be handled, mMapObject is null!");
          break;
        }
        items.add(mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA));
        break;
    }

    final String copyText = getResources().getString(android.R.string.copy);
    for (int i = 0; i < items.size(); i++)
      menu.add(Menu.NONE, i, i, String.format("%s %s", copyText, items.get(i)));

    popup.setOnMenuItemClickListener(new PopupMenu.OnMenuItemClickListener()
    {
      @Override
      public boolean onMenuItemClick(MenuItem item)
      {
        final int id = item.getItemId();
        final Context ctx = getContext();
        Utils.copyTextToClipboard(ctx, items.get(id));
        Utils.toastShortcut(ctx, ctx.getString(R.string.copied_to_clipboard, items.get(id)));
        Statistics.INSTANCE.trackEvent(Statistics.EventName.PP_METADATA_COPY + ":" + tagStr);
        AlohaHelper.logClick(AlohaHelper.PP_METADATA_COPY + ":" + tagStr);
        return true;
      }
    });

    popup.show();
    return true;
  }

  private void close()
  {
    if (mClosable != null)
      mClosable.closePlacePage();
  }

  void reset()
  {
    resetScroll();
    detachCountry();
  }

  void resetScroll()
  {
    scrollTo(0, 0);
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

  private void updateDownloader(CountryItem country)
  {
    if (isInvalidDownloaderStatus(country.status))
    {
      if (mStorageCallbackSlot != 0)
        UiThread.runLater(mDownloaderDeferredDetachProc);
      return;
    }

    mDownloaderIcon.update(country);

    StringBuilder sb = new StringBuilder(StringUtils.getFileSizeString(country.totalSize));
    if (country.isExpandable())
      sb.append(String.format(Locale.US, "  •  %s: %d", getContext().getString(R.string.downloader_status_maps), country.totalChildCount));

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

  MwmActivity getActivity()
  {
    return (MwmActivity) getContext();
  }

  @Override
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    Bookmark updatedBookmark = BookmarkManager.INSTANCE.updateBookmarkPlacePage(bookmarkId);
    if (updatedBookmark == null)
      return;

    setMapObject(updatedBookmark, null);
    NetworkPolicy policy = NetworkPolicy.newInstance(NetworkPolicy.getCurrentNetworkUsageStatus());
    refreshViews(policy);
  }

  int getPreviewHeight()
  {
    return mPreview.getHeight();
  }

  @NonNull
  public static List<PromoEntity> toEntities(@NonNull PromoCityGallery gallery)
  {
    List<PromoEntity> items = new ArrayList<>();
    for (PromoCityGallery.Item each : gallery.getItems())
    {
      String subtitle = TextUtils.isEmpty(each.getTourCategory()) ? each.getAuthor().getName()
                                                                  : each.getTourCategory();
      PromoEntity item = new PromoEntity(Constants.TYPE_PRODUCT,
                                         each.getName(),
                                         subtitle,
                                         each.getUrl(),
                                         each.getLuxCategory(),
                                         each.getImageUrl());
      items.add(item);
    }
    return items;
  }

  private class EditBookmarkClickListener implements OnClickListener
  {
    @Override
    public void onClick(View v)
    {
      if (mMapObject == null)
      {
        LOGGER.e(TAG, "A bookmark cannot be edited, mMapObject is null!");
        return;
      }
      Bookmark bookmark = (Bookmark) mMapObject;
      EditBookmarkFragment.editBookmark(bookmark.getCategoryId(),
                                        bookmark.getBookmarkId(),
                                        getActivity(),
                                        getActivity().getSupportFragmentManager(),
                                        PlacePageView.this);
    }
  }
}
