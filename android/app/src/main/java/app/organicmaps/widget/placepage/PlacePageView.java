package app.organicmaps.widget.placepage;

import static android.view.View.GONE;
import static android.view.View.VISIBLE;
import static app.organicmaps.sdk.util.Utils.getLocalizedFeatureType;
import static app.organicmaps.sdk.util.Utils.getTagValueLocalized;

import android.content.Context;
import android.content.res.Resources;
import android.graphics.Canvas;
import android.graphics.Color;
import android.graphics.ColorFilter;
import android.graphics.Outline;
import android.graphics.Paint;
import android.graphics.Path;
import android.graphics.PixelFormat;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Typeface;
import android.graphics.drawable.Drawable;
import android.location.Location;
import android.net.Uri;
import android.os.Bundle;
import android.text.SpannableStringBuilder;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.style.ForegroundColorSpan;
import android.text.style.StyleSpan;
import android.text.style.UnderlineSpan;
import android.util.TypedValue;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewOutlineProvider;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ListView;
import android.widget.PopupWindow;
import android.widget.RelativeLayout;
import android.widget.TextView;
import androidx.activity.result.ActivityResultLauncher;
import androidx.annotation.IdRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.widget.Toolbar;
import androidx.core.content.ContextCompat;
import androidx.core.graphics.ColorUtils;
import androidx.core.widget.TextViewCompat;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentActivity;
import androidx.fragment.app.FragmentFactory;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;
import app.organicmaps.MwmActivity;
import app.organicmaps.MwmApplication;
import app.organicmaps.R;
import app.organicmaps.bookmarks.BookmarksSharingHelper;
import app.organicmaps.bookmarks.ChooseBookmarkCategoryFragment;
import app.organicmaps.downloader.DownloaderStatusIcon;
import app.organicmaps.downloader.MapManagerHelper;
import app.organicmaps.sdk.Framework;
import app.organicmaps.sdk.bookmarks.data.Bookmark;
import app.organicmaps.sdk.bookmarks.data.BookmarkCategory;
import app.organicmaps.sdk.bookmarks.data.BookmarkManager;
import app.organicmaps.sdk.bookmarks.data.BookmarkSharingResult;
import app.organicmaps.sdk.bookmarks.data.DistanceAndAzimut;
import app.organicmaps.sdk.bookmarks.data.FileType;
import app.organicmaps.sdk.bookmarks.data.Icon;
import app.organicmaps.sdk.bookmarks.data.MapObject;
import app.organicmaps.sdk.bookmarks.data.Metadata;
import app.organicmaps.sdk.bookmarks.data.PredefinedColors;
import app.organicmaps.sdk.bookmarks.data.Track;
import app.organicmaps.sdk.bookmarks.data.TrackRecording;
import app.organicmaps.sdk.downloader.CountryItem;
import app.organicmaps.sdk.downloader.MapManager;
import app.organicmaps.sdk.editor.Editor;
import app.organicmaps.sdk.location.LocationCompatExtractor;
import app.organicmaps.sdk.location.LocationListener;
import app.organicmaps.sdk.location.SensorListener;
import app.organicmaps.sdk.routing.RoutingController;
import app.organicmaps.sdk.util.StringUtils;
import app.organicmaps.sdk.util.concurrency.UiThread;
import app.organicmaps.sdk.widget.placepage.CoordinatesFormat;
import app.organicmaps.sdk.widget.placepage.RouteInfo;
import app.organicmaps.util.SharingUtils;
import app.organicmaps.util.ThemeUtils;
import app.organicmaps.util.UiUtils;
import app.organicmaps.util.Utils;
import app.organicmaps.util.bottomsheet.MenuBottomSheetFragment;
import app.organicmaps.util.bottomsheet.MenuBottomSheetItem;
import app.organicmaps.utils.Graphics;
import app.organicmaps.widget.ArrowView;
import app.organicmaps.widget.colorpicker.TrackColorPickerFragment;
import app.organicmaps.widget.colorpicker.TrackColorPickerViewModel;
import app.organicmaps.widget.placepage.sections.PlacePageLinksFragment;
import app.organicmaps.widget.placepage.sections.PlacePageNotesFragment;
import app.organicmaps.widget.placepage.sections.PlacePageOpeningHoursFragment;
import app.organicmaps.widget.placepage.sections.PlacePagePhoneFragment;
import app.organicmaps.widget.placepage.sections.PlacePageProductsFragment;
import app.organicmaps.widget.placepage.sections.PlacePageTrackFragment;
import app.organicmaps.widget.placepage.sections.PlacePageTrackRecordingFragment;
import app.organicmaps.widget.placepage.sections.PlacePageWikipediaFragment;
import com.google.android.material.button.MaterialButton;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

public class PlacePageView extends Fragment
    implements View.OnClickListener, View.OnLongClickListener, LocationListener, SensorListener, Observer<MapObject>,
               ChooseBookmarkCategoryFragment.Listener, EditBookmarkFragment.EditBookmarkListener,
               MenuBottomSheetFragment.MenuBottomSheetInterface, BookmarkManager.BookmarksSharingListener,
               BookmarkColorDialogFragment.OnBookmarkColorChangeListener,
               TrackColorPickerFragment.OnTrackColorChangeListener

{
  private static final String PREF_COORDINATES_FORMAT = "coordinates_format";
  private static final String NOTES_FRAGMENT_TAG = "NOTES_FRAGMENT_TAG";
  private static final String TRACK_FRAGMENT_TAG = "TRACK_FRAGMENT_TAG";
  private static final String TRACK_RECORDING_FRAGMENT_TAG = "TRACK_RECORDING_FRAGMENT_TAG";
  private static final String PRODUCTS_FRAGMENT_TAG = "PRODUCTS_FRAGMENT_TAG";
  private static final String WIKIPEDIA_FRAGMENT_TAG = "WIKIPEDIA_FRAGMENT_TAG";
  private static final String PHONE_FRAGMENT_TAG = "PHONE_FRAGMENT_TAG";
  private static final String OPENING_HOURS_FRAGMENT_TAG = "OPENING_HOURS_FRAGMENT_TAG";
  private static final String LINKS_FRAGMENT_TAG = "LINKS_FRAGMENT_TAG";
  private static final String TRACK_SHARE_MENU_ID = "TRACK_SHARE_MENU_ID";

  private static final List<CoordinatesFormat> visibleCoordsFormat =
      Arrays.asList(CoordinatesFormat.LatLonDMS, CoordinatesFormat.LatLonDecimal, CoordinatesFormat.OLCFull,
                    CoordinatesFormat.UTM, CoordinatesFormat.MGRS, CoordinatesFormat.OSMLink);
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
  private TextView mTvLatlon;
  private View mWifi;
  private TextView mTvWiFi;
  private View mOperator;
  private TextView mTvOperator;
  private View mNetwork;
  private TextView mTvNetwork;
  private View mLevel;
  private TextView mTvLevel;
  private View mAtm;
  private TextView mTvAtm;
  private View mCapacity;
  private TextView mTvCapacity;
  private View mWheelchair;
  private TextView mTvWheelchair;
  private View mDriveThrough;
  private TextView mTvDriveThrough;
  private View mSelfService;
  private TextView mTvSelfService;
  private View mCuisine;
  private TextView mTvCuisine;
  private View mOutdoorSeating;
  private TextView mTvOutdoorSeating;
  private View mEntrance;
  private TextView mTvEntrance;
  private View mRouteRef;
  private TextView mTvRouteRef;
  private ImageView mIvRouteRef;
  @Nullable
  private RouteInfo[] mRoutes;
  @Nullable
  private PopupWindow mRoutesPopup;
  private View mEditPlace;
  private View mAddOrganisation;
  private View mAddPlace;
  private View mEditTopSpace;
  private View mDetailsTopSpace;
  private ImageView mColorIcon;
  private TextView mTvCategory;
  private ImageView mEditBookmark;
  private View mOsmDescriptionContainer;
  private TextView mTvOsmDescription;
  private MaterialButton mShareButton;
  private MaterialButton closeButton;

  @Nullable
  private Observer<String> mTrackRecordingObserver;

  // Data
  private CoordinatesFormat mCoordsFormat = CoordinatesFormat.LatLonDecimal;
  // Downloader`s stuff
  private DownloaderStatusIcon mDownloaderIcon;
  private TextView mDownloaderInfo;
  private ActivityResultLauncher<SharingUtils.SharingIntent> shareLauncher;
  private int mStorageCallbackSlot;
  @Nullable
  private CountryItem mCurrentCountry;
  private final MapManager.StorageCallback mStorageCallback = new MapManager.StorageCallback() {
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
  private PlacePageViewListener mPlacePageViewListener;

  private PlacePageViewModel mViewModel;
  private MapObject mMapObject;

  private static void refreshMetadataOrHide(@Nullable String metadata, @NonNull View metaLayout,
                                            @NonNull TextView metaTv)
  {
    if (!TextUtils.isEmpty(metadata))
    {
      metaLayout.setVisibility(VISIBLE);
      metaTv.setText(metadata);
    }
    else
      metaLayout.setVisibility(GONE);
  }

  private static void refreshMetadataOrHide(@Nullable CharSequence metadata, @NonNull View metaLayout,
                                            @NonNull TextView metaTv)
  {
    if (!TextUtils.isEmpty(metadata))
    {
      metaLayout.setVisibility(VISIBLE);
      metaTv.setText(metadata);
    }
    else
      metaLayout.setVisibility(GONE);
  }

  private static boolean isInvalidDownloaderStatus(int status)
  {
    return (status != CountryItem.STATUS_DOWNLOADABLE && status != CountryItem.STATUS_ENQUEUED
            && status != CountryItem.STATUS_FAILED && status != CountryItem.STATUS_PARTLY
            && status != CountryItem.STATUS_PROGRESS && status != CountryItem.STATUS_APPLYING);
  }

  @Nullable
  @Override
  public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState)
  {
    mViewModel = new ViewModelProvider(requireActivity()).get(PlacePageViewModel.class);
    shareLauncher = SharingUtils.RegisterLauncher(this);
    return inflater.inflate(R.layout.place_page, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    mCoordsFormat =
        CoordinatesFormat.fromId(MwmApplication.prefs(requireContext())
                                     .getInt(PREF_COORDINATES_FORMAT, CoordinatesFormat.LatLonDecimal.getId()));

    Fragment parentFragment = getParentFragment();
    mPlacePageViewListener = (PlacePageViewListener) parentFragment;

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

    mColorIcon = mFrame.findViewById(R.id.item_icon);
    mTvCategory = mFrame.findViewById(R.id.tv__category);
    mEditBookmark = mFrame.findViewById(R.id.edit_Bookmark);
    mColorIcon.setOnClickListener(this);
    mTvCategory.setOnClickListener(this);
    mEditBookmark.setOnClickListener(this);

    mOsmDescriptionContainer = mFrame.findViewById(R.id.osm_description_container);
    mTvOsmDescription = mFrame.findViewById(R.id.tv__osm_description);
    mTvOsmDescription.setOnLongClickListener(this);

    mShareButton = mPreview.findViewById(R.id.share_button);
    mShareButton.setOnClickListener(this::shareClickListener);

    closeButton = mPreview.findViewById(R.id.close_button);
    closeButton.setOnClickListener((v) -> mPlacePageViewListener.onPlacePageRequestClose());

    RelativeLayout address = mFrame.findViewById(R.id.ll__place_name);

    LinearLayout latlon = mFrame.findViewById(R.id.ll__place_latlon);
    latlon.setOnClickListener(this);
    LinearLayout openIn = mFrame.findViewById(R.id.ll__place_open_in);
    openIn.setOnClickListener(this);
    openIn.setOnLongClickListener(this);
    openIn.setVisibility(VISIBLE);
    mTvLatlon = mFrame.findViewById(R.id.tv__place_latlon);
    mWifi = mFrame.findViewById(R.id.ll__place_wifi);
    mTvWiFi = mFrame.findViewById(R.id.tv__place_wifi);
    mOutdoorSeating = mFrame.findViewById(R.id.ll__place_outdoor_seating);
    mTvOutdoorSeating = mFrame.findViewById(R.id.tv__place_outdoor_seating);
    mOperator = mFrame.findViewById(R.id.ll__place_operator);
    mOperator.setOnClickListener(this);
    mTvOperator = mFrame.findViewById(R.id.tv__place_operator);
    mNetwork = mFrame.findViewById(R.id.ll__place_network);
    mTvNetwork = mFrame.findViewById(R.id.tv__place_network);
    mLevel = mFrame.findViewById(R.id.ll__place_level);
    mTvLevel = mFrame.findViewById(R.id.tv__place_level);
    mAtm = mFrame.findViewById(R.id.ll__place_atm);
    mTvAtm = mFrame.findViewById(R.id.tv__place_atm);
    mCapacity = mFrame.findViewById(R.id.ll__place_capacity);
    mTvCapacity = mFrame.findViewById(R.id.tv__place_capacity);
    mWheelchair = mFrame.findViewById(R.id.ll__place_wheelchair);
    mTvWheelchair = mFrame.findViewById(R.id.tv__place_wheelchair);
    mDriveThrough = mFrame.findViewById(R.id.ll__place_drive_through);
    mTvDriveThrough = mFrame.findViewById(R.id.tv__place_drive_through);
    mSelfService = mFrame.findViewById(R.id.ll__place_self_service);
    mTvSelfService = mFrame.findViewById(R.id.tv__place_self_service);
    mCuisine = mFrame.findViewById(R.id.ll__place_cuisine);
    mTvCuisine = mFrame.findViewById(R.id.tv__place_cuisine);
    mEntrance = mFrame.findViewById(R.id.ll__place_entrance);
    mTvEntrance = mEntrance.findViewById(R.id.tv__place_entrance);
    mRouteRef = mFrame.findViewById(R.id.ll__place_route_ref);
    mFrame.findViewById(R.id.ll__place_route_ref_content).setOnClickListener(this);
    mTvRouteRef = mFrame.findViewById(R.id.tv__place_route_ref);
    mIvRouteRef = mFrame.findViewById(R.id.iv__place_route_ref);
    mEditPlace = mFrame.findViewById(R.id.ll__place_editor);
    mEditPlace.setOnClickListener(this);
    mAddOrganisation = mFrame.findViewById(R.id.ll__add_organisation);
    mAddOrganisation.setOnClickListener(this);
    mAddPlace = mFrame.findViewById(R.id.ll__place_add);
    mAddPlace.setOnClickListener(this);
    mEditTopSpace = mFrame.findViewById(R.id.edit_top_space);
    mDetailsTopSpace = mFrame.findViewById(R.id.details_top_space);
    latlon.setOnLongClickListener(this);
    address.setOnLongClickListener(this);
    mOperator.setOnLongClickListener(this);
    mNetwork.setOnLongClickListener(this);
    mLevel.setOnLongClickListener(this);
    mAtm.setOnLongClickListener(this);
    mCapacity.setOnLongClickListener(this);
    mWheelchair.setOnLongClickListener(this);
    mDriveThrough.setOnLongClickListener(this);
    mSelfService.setOnLongClickListener(this);
    mOutdoorSeating.setOnLongClickListener(this);

    mDownloaderIcon = new DownloaderStatusIcon(mPreview.findViewById(R.id.downloader_status_frame));

    mDownloaderInfo = mPreview.findViewById(R.id.tv__downloader_details);
  }

  @Override
  public void onStart()
  {
    super.onStart();
    mViewModel.getMapObject().observe(requireActivity(), this);
    BookmarkManager.INSTANCE.addSharingListener(this);
    MwmApplication.from(requireContext()).getLocationHelper().addListener(this);
    MwmApplication.from(requireContext()).getSensorHelper().addListener(this);
  }

  @Override
  public void onStop()
  {
    super.onStop();
    if (mRoutesPopup != null && mRoutesPopup.isShowing())
      mRoutesPopup.dismiss();
    mViewModel.getMapObject().removeObserver(this);
    BookmarkManager.INSTANCE.removeSharingListener(this);
    MwmApplication.from(requireContext()).getLocationHelper().removeListener(this);
    MwmApplication.from(requireContext()).getSensorHelper().removeListener(this);
    detachCountry();
  }

  private void setCurrentCountry()
  {
    if (mCurrentCountry != null)
      return;
    String country = MapManager.nativeGetSelectedCountry();
    if (country != null && !RoutingController.get().isNavigating())
      attachCountry(country);
  }

  private void refreshViews()
  {
    refreshPreview();
    refreshDetails();
    final Location loc = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
    if (mMapObject.isMyPosition())
      refreshMyPosition(loc);
    else
      refreshDistanceToObject(loc);
  }

  private <T extends Fragment> void updateViewFragment(Class<T> controllerClass, String fragmentTag,
                                                       @IdRes int containerId, boolean enabled)
  {
    final FragmentManager fm = getChildFragmentManager();
    final Fragment fragment = fm.findFragmentByTag(fragmentTag);
    if (enabled && fragment == null)
    {
      fm.beginTransaction().setReorderingAllowed(true).add(containerId, controllerClass, null, fragmentTag).commit();
    }
    else if (!enabled && fragment != null)
    {
      fm.beginTransaction().setReorderingAllowed(true).remove(fragment).commit();
    }
  }

  private void updateLinksView()
  {
    updateViewFragment(PlacePageLinksFragment.class, LINKS_FRAGMENT_TAG, R.id.place_page_links_fragment, true);
  }

  private void updateOpeningHoursView()
  {
    final String ohStr = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS);
    updateViewFragment(PlacePageOpeningHoursFragment.class, OPENING_HOURS_FRAGMENT_TAG,
                       R.id.place_page_opening_hours_fragment, !TextUtils.isEmpty(ohStr));
  }

  private void updatePhoneView()
  {
    updateViewFragment(PlacePagePhoneFragment.class, PHONE_FRAGMENT_TAG, R.id.place_page_phone_fragment,
                       mMapObject.hasPhoneNumber());
  }

  private void updateNotesView()
  {
    updateViewFragment(PlacePageNotesFragment.class, NOTES_FRAGMENT_TAG, R.id.place_page_notes_fragment,
                       mMapObject.isBookmark() || mMapObject.isTrack());
  }

  private void updateTrackView()
  {
    updateViewFragment(PlacePageTrackFragment.class, TRACK_FRAGMENT_TAG, R.id.place_page_track_fragment,
                       mMapObject.isTrack());
  }

  private void updateTrackRecordingView()
  {
    updateViewFragment(PlacePageTrackRecordingFragment.class, TRACK_RECORDING_FRAGMENT_TAG,
                       R.id.place_page_track_fragment, mMapObject.isTrackRecording());
  }

  private boolean hasWikipediaEntry()
  {
    final String wikipediaLink = mMapObject.getMetadata(Metadata.MetadataType.FMD_WIKIPEDIA);
    final String wikiArticle = mMapObject.getWikiArticle();
    return !TextUtils.isEmpty(wikipediaLink) || !TextUtils.isEmpty(wikiArticle);
  }

  private void updateWikipediaView()
  {
    updateViewFragment(PlacePageWikipediaFragment.class, WIKIPEDIA_FRAGMENT_TAG, R.id.place_page_wikipedia_fragment,
                       hasWikipediaEntry());
  }

  private boolean hasProductsEntry()
  {
    return Framework.nativeShouldShowProducts();
  }

  private void updateProductsView()
  {
    var hasProductsEntry = hasProductsEntry();

    updateViewFragment(PlacePageProductsFragment.class, PRODUCTS_FRAGMENT_TAG, R.id.place_page_products_fragment,
                       hasProductsEntry);
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
        sb.setSpan(new ForegroundColorSpan(ContextCompat.getColor(requireContext(), R.color.base_yellow)), start, end,
                   Spanned.SPAN_INCLUSIVE_EXCLUSIVE);
      }
      mTvSubtitle.setText(sb);
    }
    if (mMapObject.isTrack())
      UiUtils.hide(mTvSubtitle);
  }

  private void refreshPreview()
  {
    UiUtils.hideIf(mMapObject.isTrackRecording(), closeButton);
    UiUtils.setTextAndHideIfEmpty(mTvTitle, mMapObject.getTitle());
    UiUtils.setTextAndHideIfEmpty(mTvSecondaryTitle, mMapObject.getSecondaryTitle());
    if (mToolbar != null)
      mToolbar.setTitle(mMapObject.getTitle());
    setTextAndColorizeSubtitle();
    UiUtils.setTextAndHideIfEmpty(mTvAddress, mMapObject.getAddress());

    refreshCategoryPreview();

    final String osmDescription = mMapObject.getOsmDescription();
    if (osmDescription.isEmpty())
      mOsmDescriptionContainer.setVisibility(GONE);
    else
    {
      mTvOsmDescription.setText(osmDescription);
      mOsmDescriptionContainer.setVisibility(VISIBLE);
    }
    if (mMapObject.isTrack())
    {
      UiUtils.hide(mAvDirection, mTvDistance);
    }
    else if (mMapObject.isTrackRecording())
    {
      TrackRecording trackRecording = (TrackRecording) mMapObject;
      final var liveData = trackRecording.getTrackRecordingPPDescription();
      if (mTrackRecordingObserver != null)
        liveData.removeObserver(mTrackRecordingObserver);
      mTrackRecordingObserver = s -> UiUtils.setTextAndHideIfEmpty(mTvTitle, s);
      liveData.observe(getViewLifecycleOwner(), mTrackRecordingObserver);
      UiUtils.hide(mAvDirection, mTvDistance);
    }
  }

  void refreshCategoryPreview()
  {
    View categoryContainer = mFrame.findViewById(R.id.category_container);
    boolean showCategory;
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      Drawable circle = Graphics.drawCircle(track.getColor(), R.dimen.place_page_icon_background_size,
                                            requireContext().getResources());
      mColorIcon.setImageDrawable(circle);
      showCategory = !track.isRelationTrack();
      if (showCategory)
        mTvCategory.setText(BookmarkManager.INSTANCE.getCategoryById(track.getCategoryId()).getName());
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      Icon icon = bookmark.getIcon();
      if (icon != null)
      {
        Drawable circle = Graphics.drawCircleAndImage(icon.argb(), R.dimen.place_page_icon_background_size,
                                                      icon.getResId(), R.dimen.place_page_icon_size, requireContext());
        mColorIcon.setImageDrawable(circle);
        mTvCategory.setText(BookmarkManager.INSTANCE.getCategoryById(bookmark.getCategoryId()).getName());
      }
      showCategory = true;
    }
    else
      showCategory = false;
    UiUtils.showIf(showCategory, categoryContainer);
  }

  void showColorDialog()
  {
    final FragmentManager manager = getChildFragmentManager();

    if (mMapObject.isTrack())
    {
      final Track track = (Track) mMapObject;
      final Bundle args = new Bundle();
      args.putInt(TrackColorPickerViewModel.EXTRA_INITIAL_COLOR, track.getColor());
      final TrackColorPickerFragment fragment = new TrackColorPickerFragment();
      fragment.setArguments(args);
      fragment.show(manager, null);
    }
    else if (mMapObject.isBookmark())
    {
      final Bundle args = new Bundle();
      final Bookmark bookmark = (Bookmark) mMapObject;
      args.putInt(BookmarkColorDialogFragment.ICON_COLOR, bookmark.getIcon().getColor());
      args.putInt(BookmarkColorDialogFragment.ICON_RES, bookmark.getIcon().getResId());
      final BookmarkColorDialogFragment dialogFragment = new BookmarkColorDialogFragment();
      dialogFragment.setArguments(args);
      dialogFragment.show(manager, null);
    }
  }

  @Override
  public void onTrackColorSet(int color)
  {
    if (mMapObject == null || !mMapObject.isTrack())
      return;
    final Track track = (Track) mMapObject;
    if (track.getColor() == color)
      return;
    track.setColor(color);
    Drawable circle =
        Graphics.drawCircle(color, R.dimen.place_page_icon_background_size, requireContext().getResources());
    mColorIcon.setImageDrawable(circle);
  }

  @Override
  public void onBookmarkColorSet(int colorPos)
  {
    if (mMapObject == null || !mMapObject.isBookmark())
      return;
    final Bookmark bookmark = (Bookmark) mMapObject;
    int from = bookmark.getIcon().argb();
    int to = PredefinedColors.getColor(colorPos);
    if (from == to)
      return;
    bookmark.setIconColor(to);
    Drawable circle =
        Graphics.drawCircleAndImage(to, R.dimen.place_page_icon_background_size, bookmark.getIcon().getResId(),
                                    R.dimen.place_page_icon_size, requireContext());
    mColorIcon.setImageDrawable(circle);
  }

  private void showCategoryList()
  {
    final Bundle args = new Bundle();
    final List<BookmarkCategory> categories = BookmarkManager.INSTANCE.getCategories();
    final FragmentManager manager = getChildFragmentManager();
    String className = ChooseBookmarkCategoryFragment.class.getName();
    final FragmentFactory factory = manager.getFragmentFactory();
    final ChooseBookmarkCategoryFragment frag =
        (ChooseBookmarkCategoryFragment) factory.instantiate(getContext().getClassLoader(), className);
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      BookmarkCategory currentCategory = BookmarkManager.INSTANCE.getCategoryById(track.getCategoryId());
      final int index = categories.indexOf(currentCategory);
      args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_POSITION, index);
      frag.setArguments(args);
      frag.show(manager, null);
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      BookmarkCategory currentCategory = BookmarkManager.INSTANCE.getCategoryById(bookmark.getCategoryId());
      final int index = categories.indexOf(currentCategory);
      args.putInt(ChooseBookmarkCategoryFragment.CATEGORY_POSITION, index);
      frag.setArguments(args);
      frag.show(manager, null);
    }
  }

  @Override
  public void onCategoryChanged(@NonNull BookmarkCategory newCategory)
  {
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      BookmarkCategory previousCategory = BookmarkManager.INSTANCE.getCategoryById(track.getCategoryId());
      if (previousCategory == newCategory)
        return;
      track.setCategoryId(newCategory.getId());
      mTvCategory.setText(newCategory.getName());
      track.setCategoryId(newCategory.getId());
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      BookmarkCategory previousCategory = BookmarkManager.INSTANCE.getCategoryById(bookmark.getCategoryId());
      if (previousCategory == newCategory)
        return;
      mTvCategory.setText(newCategory.getName());
      bookmark.setCategoryId(newCategory.getId());
    }
  }

  void showBookmarkEditFragment()
  {
    if (mMapObject.isTrack())
    {
      Track track = (Track) mMapObject;
      final FragmentActivity activity = requireActivity();
      EditBookmarkFragment.editTrack(track.getCategoryId(), track.getTrackId(), activity, getChildFragmentManager(),
                                     PlacePageView.this);
    }
    else if (mMapObject.isBookmark())
    {
      Bookmark bookmark = (Bookmark) mMapObject;
      final FragmentActivity activity = requireActivity();
      EditBookmarkFragment.editBookmark(bookmark.getCategoryId(), bookmark.getBookmarkId(), activity,
                                        getChildFragmentManager(), PlacePageView.this);
    }
  }

  @Override
  public void onBookmarkSaved(long bookmarkId, boolean movedFromCategory)
  {
    if (mMapObject.isTrack())
      BookmarkManager.INSTANCE.updateTrackPlacePage();
    else if (mMapObject.isBookmark())
      BookmarkManager.INSTANCE.updateBookmarkPlacePage(bookmarkId);
  }

  private void refreshDetails()
  {
    refreshLatLon();

    final String operator = mMapObject.getMetadata(Metadata.MetadataType.FMD_OPERATOR);
    refreshMetadataOrHide(!TextUtils.isEmpty(operator) ? getString(R.string.operator, operator) : "", mOperator,
                          mTvOperator);

    final String network = mMapObject.getMetadata(Metadata.MetadataType.FMD_NETWORK);
    refreshMetadataOrHide(!TextUtils.isEmpty(network) ? getString(R.string.network, network) : "", mNetwork,
                          mTvNetwork);

    /// @todo I don't like it when we take all data from mapObject, but for cuisines, we should
    /// go into JNI Framework and rely on some "active object".
    refreshMetadataOrHide(Framework.nativeGetActiveObjectFormattedCuisine(), mCuisine, mTvCuisine);
    refreshWiFi();
    refreshMetadataOrHide(mMapObject.getMetadata(Metadata.MetadataType.FMD_FLATS), mEntrance, mTvEntrance);
    final String level = Utils.getLocalizedLevel(getContext(), mMapObject.getMetadata(Metadata.MetadataType.FMD_LEVEL));
    refreshMetadataOrHide(level, mLevel, mTvLevel);

    final String cap = mMapObject.getMetadata(Metadata.MetadataType.FMD_CAPACITY);
    refreshMetadataOrHide(!TextUtils.isEmpty(cap) ? getString(R.string.capacity, cap) : "", mCapacity, mTvCapacity);

    refreshMetadataOrHide(mMapObject.hasAtm() ? getString(app.organicmaps.sdk.R.string.type_amenity_atm) : "", mAtm,
                          mTvAtm);

    final String wheelchair =
        getLocalizedFeatureType(getContext(), mMapObject.getMetadata(Metadata.MetadataType.FMD_WHEELCHAIR));
    refreshMetadataOrHide(wheelchair, mWheelchair, mTvWheelchair);

    final String driveThrough = mMapObject.getMetadata(Metadata.MetadataType.FMD_DRIVE_THROUGH);
    refreshMetadataOrHide(driveThrough.equals("yes") ? getString(R.string.drive_through) : "", mDriveThrough,
                          mTvDriveThrough);

    final String selfService = mMapObject.getMetadata(Metadata.MetadataType.FMD_SELF_SERVICE);
    refreshMetadataOrHide(getTagValueLocalized(getContext(), "self_service", selfService), mSelfService,
                          mTvSelfService);

    final String outdoorSeating = mMapObject.getMetadata(Metadata.MetadataType.FMD_OUTDOOR_SEATING);
    refreshMetadataOrHide(outdoorSeating.equals("yes") ? getString(R.string.outdoor_seating) : "", mOutdoorSeating,
                          mTvOutdoorSeating);

    // showTaxiOffer(mapObject);
    mRoutes = Framework.nativeGetActiveObjectRoutes();
    refreshMetadataOrHide(formatRouteRefs(mRoutes, Framework.nativeGetActiveTransitRouteRef()), mRouteRef, mTvRouteRef);
    if (mRouteRef.getVisibility() == VISIBLE)
    {
      if (mMapObject.isTramStop())
        mIvRouteRef.setImageResource(R.drawable.ic_category_tram);
      else
        mIvRouteRef.setImageResource(R.drawable.ic_category_bus);
    }

    if (RoutingController.get().isNavigating() || RoutingController.get().isPlanning())
    {
      UiUtils.hide(mEditPlace, mAddOrganisation, mAddPlace, mEditTopSpace);
    }
    else
    {
      UiUtils.showIf(Editor.nativeShouldShowEditPlace(), mEditPlace);
      UiUtils.showIf(Editor.nativeShouldShowAddBusiness(), mAddOrganisation);
      UiUtils.showIf(Editor.nativeShouldShowAddPlace(), mAddPlace);
      mEditPlace.setEnabled(Editor.nativeCanEditPlace());
      mAddOrganisation.setEnabled(Editor.nativeCanEditPlace());
      mAddPlace.setEnabled(Editor.nativeCanEditPlace());
      TextView mTvEditPlace = mEditPlace.findViewById(R.id.tv__editor);
      TextView mTvAddBusiness = mAddPlace.findViewById(R.id.tv__editor);
      TextView mTvAddPlace = mAddPlace.findViewById(R.id.tv__editor);
      final int editPlaceButtonColor =
          Editor.nativeCanEditPlace()
              ? ContextCompat.getColor(getContext(),
                                       UiUtils.getStyledResourceId(getContext(), androidx.appcompat.R.attr.colorAccent))
              : getResources().getColor(R.color.button_accent_text_disabled);
      mTvEditPlace.setTextColor(editPlaceButtonColor);
      mTvAddBusiness.setTextColor(editPlaceButtonColor);
      mTvAddPlace.setTextColor(editPlaceButtonColor);
      UiUtils.showIf(
          UiUtils.isVisible(mEditPlace) || UiUtils.isVisible(mAddOrganisation) || UiUtils.isVisible(mAddPlace),
          mEditTopSpace);
    }
    final boolean isTrackOrRecording = mMapObject.isTrack() || mMapObject.isTrackRecording();
    final boolean isRelationTrack = mMapObject.isTrack() && ((Track) mMapObject).isRelationTrack();
    UiUtils.hideIf(mMapObject.isTrackRecording() || isRelationTrack, mShareButton);
    UiUtils.hideIf(isTrackOrRecording, mFrame.findViewById(R.id.ll__place_open_in), mDetailsTopSpace);
    updateLinksView();
    updateOpeningHoursView();
    updateProductsView();
    updateWikipediaView();
    updateNotesView();
    updatePhoneView();
    updateTrackView();
    updateTrackRecordingView();
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

  private void refreshMyPosition(Location l)
  {
    if (mMapObject.isTrack() || mMapObject.isTrackRecording())
      return;

    UiUtils.hide(mTvDistance);
    UiUtils.hide(mAvDirection);

    if (l == null)
      return;

    final StringBuilder builder = new StringBuilder();
    final LocationCompatExtractor.Altitude altitude = LocationCompatExtractor.getAltitude(l);
    if (altitude != null)
      builder.append("▲").append(Framework.nativeFormatAltitude(altitude.altitude()));
    if (l.hasSpeed())
      builder.append("   ").append(Framework.nativeFormatSpeed(l.getSpeed()));

    UiUtils.setTextAndHideIfEmpty(mTvSubtitle, builder.toString());

    mMapObject.setLat(l.getLatitude());
    mMapObject.setLon(l.getLongitude());
    refreshLatLon();
  }

  private void refreshDistanceToObject(Location l)
  {
    if (mMapObject.isTrack() || mMapObject.isTrackRecording())
      return;
    UiUtils.showIf(l != null, mTvDistance);
    if (l == null)
      return;

    double lat = mMapObject.getLat();
    double lon = mMapObject.getLon();
    DistanceAndAzimut distanceAndAzimuth =
        Framework.nativeGetDistanceAndAzimuthFromLatLon(lat, lon, l.getLatitude(), l.getLongitude(), 0.0);
    mTvDistance.setText(distanceAndAzimuth.getDistance().toString(requireContext()));
  }

  private void refreshLatLon()
  {
    final double lat = mMapObject.getLat();
    final double lon = mMapObject.getLon();
    String latLon = Framework.nativeFormatLatLon(lat, lon, mCoordsFormat.getId());
    if (latLon == null) // Some coordinates couldn't be converted to UTM and MGRS
      latLon = "N/A";

    if (mCoordsFormat.showLabel())
      mTvLatlon.setText(mCoordsFormat.getLabel() + ": " + latLon);
    else
      mTvLatlon.setText(latLon);
    UiUtils.hideIf(mMapObject.isTrackRecording() || mMapObject.isTrack(), mFrame.findViewById(R.id.ll__place_latlon));
  }

  private void addOrganisation()
  {
    ((MwmActivity) requireActivity()).showPositionChooserForEditor(true, true);
  }

  private void addPlace()
  {
    ((MwmActivity) requireActivity()).showPositionChooserForEditor(false, true);
  }

  /// @todo
  /// - Why ll__place_editor and ll__place_latlon check if (mMapObject == null)

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
      MwmApplication.prefs(context).edit().putInt(PREF_COORDINATES_FORMAT, mCoordsFormat.getId()).apply();
      refreshLatLon();
    }
    else if (id == R.id.ll__place_open_in)
    {
      final String uri = Framework.nativeGetGeoUri(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getScale(),
                                                   mMapObject.getName());
      Utils.openUri(requireContext(), Uri.parse(uri), R.string.uri_open_location_failed);
    }
    else if (id == R.id.direction_frame)
      showBigDirection();
    else if (id == R.id.item_icon)
      showColorDialog();
    else if (id == R.id.edit_Bookmark)
      showBookmarkEditFragment();
    else if (id == R.id.tv__category)
      showCategoryList();
    else if (id == R.id.ll__place_route_ref_content)
      showRoutesPopup(v);
  }

  @NonNull
  private static CharSequence formatRouteRefs(@Nullable RouteInfo[] routes, @Nullable String selectedRef)
  {
    if (routes == null || routes.length == 0)
      return "";
    // Routes can repeat the same ref with different from/to (e.g. inbound/outbound directions
    // of the same line). Collapse them in the primary row — the popup still shows all entries.
    final HashSet<String> seen = new HashSet<>(routes.length);
    final SpannableStringBuilder sb = new SpannableStringBuilder();
    for (RouteInfo r : routes)
    {
      if (!seen.add(r.getRef()))
        continue;
      if (sb.length() > 0)
        sb.append(" • ");
      final int start = sb.length();
      sb.append(r.getRef());
      if (r.getRef().equals(selectedRef))
      {
        sb.setSpan(new StyleSpan(Typeface.BOLD), start, sb.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        sb.setSpan(new UnderlineSpan(), start, sb.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
      }
    }
    return sb;
  }

  private void showRoutesPopup(@NonNull View anchor)
  {
    if (mRoutes == null || mRoutes.length == 0)
      return;

    final Context context = requireContext();
    final String[] labels = new String[mRoutes.length];
    for (int i = 0; i < mRoutes.length; i++)
      labels[i] = mRoutes[i].formatLabel();

    final int padH =
        (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 16, context.getResources().getDisplayMetrics());
    final int padV =
        (int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 12, context.getResources().getDisplayMetrics());

    // Resolve theme colors.
    final int defaultTextColor = ThemeUtils.getColor(context, android.R.attr.textColorPrimary);
    final int bgColor = ThemeUtils.getColor(context, R.attr.cardBackground);

    // Popup shape parameters.
    final Resources res = context.getResources();
    final float radius = res.getDimension(R.dimen.corner_radius_medium);
    final float arrowWidth = res.getDimension(R.dimen.routes_popup_arrow_width);
    final float arrowHeight = res.getDimension(R.dimen.routes_popup_arrow_height);

    // Dismiss any existing popup (prevent stacking on rapid taps).
    if (mRoutesPopup != null && mRoutesPopup.isShowing())
      mRoutesPopup.dismiss();

    final int margin = (int) res.getDimension(R.dimen.routes_popup_margin);
    final int popupWidth = anchor.getWidth() - 2 * margin;
    if (popupWidth <= 0)
      return;

    // Predict popup direction: show below when there is more space below the anchor.
    final int[] anchorLoc = new int[2];
    anchor.getLocationOnScreen(anchorLoc);
    final Rect displayFrame = new Rect();
    anchor.getWindowVisibleDisplayFrame(displayFrame);
    final int spaceAbove = anchorLoc[1] - displayFrame.top;
    final int spaceBelow = displayFrame.bottom - anchorLoc[1] - anchor.getHeight();
    final boolean arrowOnTop = spaceBelow > spaceAbove;

    // Rounded background with an arrow pointing toward the anchor.
    final ArrowPopupBackground bgDrawable =
        new ArrowPopupBackground(bgColor, ContextCompat.getColor(context, R.color.black_12), radius, arrowWidth,
                                 arrowHeight, res.getDimension(R.dimen.routes_popup_shadow_offset), arrowOnTop);

    // Build the list view.
    final ListView listView = new ListView(context);
    listView.setDivider(null);
    listView.setDividerHeight(0);
    listView.setAdapter(new ArrayAdapter<>(context, 0, labels) {
      @NonNull
      @Override
      public View getView(int position, @Nullable View convertView, @NonNull ViewGroup parent)
      {
        TextView tv = (TextView) convertView;
        if (tv == null)
        {
          tv = new TextView(context);
          tv.setPadding(padH, padV, padH, padV);
          TextViewCompat.setTextAppearance(tv, androidx.appcompat.R.style.TextAppearance_AppCompat_Body1);
        }
        tv.setText(labels[position]);
        if (mRoutes == null || position >= mRoutes.length)
          return tv;
        final RouteInfo r = mRoutes[position];
        if (r.hasColor())
        {
          tv.setBackgroundColor(ColorUtils.blendARGB(r.getColor(), Color.WHITE, 0.75f));
          // Pastel background stays light in both themes — keep text readable on it.
          tv.setTextColor(Color.BLACK);
        }
        else
        {
          tv.setBackgroundColor(Color.TRANSPARENT);
          tv.setTextColor(defaultTextColor);
        }
        return tv;
      }
    });
    listView.setOutlineProvider(new ViewOutlineProvider() {
      @Override
      public void getOutline(View view, Outline outline)
      {
        outline.setRoundRect(0, 0, view.getWidth(), view.getHeight(), radius);
      }
    });
    listView.setClipToOutline(true);

    // Measure content height capped to available space.
    final int maxHeight = (arrowOnTop ? spaceBelow : spaceAbove) - margin;
    if (maxHeight <= 0)
      return;
    listView.measure(View.MeasureSpec.makeMeasureSpec(popupWidth, View.MeasureSpec.AT_MOST),
                     View.MeasureSpec.makeMeasureSpec(maxHeight - (int) arrowHeight, View.MeasureSpec.AT_MOST));
    final int popupHeight = listView.getMeasuredHeight() + (int) arrowHeight;
    if (popupHeight <= 0)
      return;

    final PopupWindow popup = new PopupWindow(listView, popupWidth, popupHeight);
    popup.setBackgroundDrawable(bgDrawable);
    popup.setElevation(res.getDimension(R.dimen.routes_popup_elevation));
    popup.setFocusable(true);
    popup.setOutsideTouchable(true);
    popup.setAnimationStyle(android.R.style.Animation_Dialog);

    listView.setOnItemClickListener((parent, view, position, id) -> {
      final RouteInfo r = mRoutes[position];
      mTvRouteRef.setText(formatRouteRefs(mRoutes, r.getRef()));
      Framework.nativeShowRouteTransit(r.getRelId());
      popup.dismiss();
    });

    // Clamp popupX to screen bounds.
    final int popupX =
        Math.max(displayFrame.left + margin, Math.min(anchorLoc[0] + margin, displayFrame.right - popupWidth - margin));
    final int popupY;
    if (arrowOnTop)
      popupY = anchorLoc[1] + anchor.getHeight() + margin;
    else
      popupY = anchorLoc[1] - popupHeight - margin;
    popup.showAtLocation(anchor, Gravity.NO_GRAVITY, popupX, popupY);
    mRoutesPopup = popup;
  }

  private void showBigDirection()
  {
    final FragmentManager fragmentManager = requireActivity().getSupportFragmentManager();
    final DirectionFragment fragment = (DirectionFragment) fragmentManager.getFragmentFactory().instantiate(
        requireContext().getClassLoader(), DirectionFragment.class.getName());
    fragment.setMapObject(mMapObject);
    fragment.show(fragmentManager, null);
  }

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
    else if (id == R.id.tv__osm_description)
      items.add(mTvOsmDescription.getText().toString());
    else if (id == R.id.ll__place_latlon)
    {
      final double lat = mMapObject.getLat();
      final double lon = mMapObject.getLon();
      for (CoordinatesFormat format : visibleCoordsFormat)
      {
        String formatted = Framework.nativeFormatLatLon(lat, lon, format.getId());
        if (formatted != null)
          items.add(formatted);
      }
    }
    else if (id == R.id.ll__place_open_in)
    {
      final String uri = Framework.nativeGetGeoUri(mMapObject.getLat(), mMapObject.getLon(), mMapObject.getScale(),
                                                   mMapObject.getName());
      PlacePageUtils.copyToClipboard(requireContext(), mFrame, uri);
    }
    else if (id == R.id.ll__place_operator)
      items.add(mTvOperator.getText().toString());
    else if (id == R.id.ll__place_network)
      items.add(mTvNetwork.getText().toString());
    else if (id == R.id.ll__place_level)
      items.add(mTvLevel.getText().toString());
    else if (id == R.id.ll__place_atm)
      items.add(mTvAtm.getText().toString());
    else if (id == R.id.ll__place_capacity)
      items.add(mTvCapacity.getText().toString());
    else if (id == R.id.ll__place_wheelchair)
      items.add(mTvWheelchair.getText().toString());
    else if (id == R.id.ll__place_drive_through)
      items.add(mTvDriveThrough.getText().toString());
    else if (id == R.id.ll__place_outdoor_seating)
      items.add(mTvOutdoorSeating.getText().toString());

    final Context context = requireContext();
    if (items.size() == 1)
      PlacePageUtils.copyToClipboard(context, mFrame, items.get(0));
    else
      PlacePageUtils.showCopyPopup(context, v, items);

    return true;
  }

  private void updateDownloader(CountryItem country)
  {
    if (isInvalidDownloaderStatus(country.status))
    {
      if (mStorageCallbackSlot != 0)
        UiThread.runLater(this::detachCountry);
      return;
    }

    mDownloaderIcon.update(country);

    StringBuilder sb = new StringBuilder(StringUtils.getFileSizeString(requireContext(), country.totalSize));
    if (country.isExpandable())
      sb.append(StringUtils.formatUsingUsLocale(
          "  •  %s: %d", requireContext().getString(R.string.downloader_status_maps), country.totalChildCount));

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

    mDownloaderIcon
        .setOnIconClickListener((v) -> MapManagerHelper.warn3gAndDownload(requireActivity(), mCurrentCountry.id, null))
        .setOnCancelClickListener((v) -> MapManager.nativeCancel(mCurrentCountry.id));
    mDownloaderIcon.show(true);
    UiUtils.show(mDownloaderInfo);
    updateDownloader(mCurrentCountry);
  }

  private void detachCountry()
  {
    if (mStorageCallbackSlot == 0 || mCurrentCountry == null)
      return;

    MapManager.nativeUnsubscribe(mStorageCallbackSlot);
    mStorageCallbackSlot = 0;
    mCurrentCountry = null;
    mDownloaderIcon.setOnIconClickListener(null).setOnCancelClickListener(null);
    mDownloaderIcon.show(false);
    UiUtils.hide(mDownloaderInfo);
  }

  @Override
  public void onChanged(@Nullable MapObject mapObject)
  {
    if (mapObject == null)
      return;
    // Starting the download will fire this callback but the object will be the same
    // Detaching the country in that case will crash the app
    if (!mapObject.sameAs(mMapObject))
      detachCountry();
    setCurrentCountry();

    mMapObject = mapObject;

    refreshViews();
    // In case the place page has already some data, make sure to call the onPlacePageContentChanged callback
    // to catch cases where the new data has the exact same height as the previous one (eg 2 address nodes)
    if (mFrame.getHeight() > 0)
      mPlacePageViewListener.onPlacePageContentChanged(mPreview.getMeasuredHeight(), mFrame.getHeight());
  }

  @Override
  public void onLocationUpdated(@NonNull Location location)
  {
    if (mMapObject == null)
      return;
    if (mMapObject.isMyPosition())
      refreshMyPosition(location);
    else
      refreshDistanceToObject(location);
  }

  @Override
  public void onCompassUpdated(double north)
  {
    if (mMapObject == null || mMapObject.isMyPosition() || mMapObject.isTrack() || mMapObject.isTrackRecording())
      return;

    final Location location = MwmApplication.from(requireContext()).getLocationHelper().getSavedLocation();
    if (location == null)
    {
      UiUtils.hide(mAvDirection);
      return;
    }

    final double azimuth =
        Framework
            .nativeGetDistanceAndAzimuthFromLatLon(mMapObject.getLat(), mMapObject.getLon(), location.getLatitude(),
                                                   location.getLongitude(), north)
            .getAzimuth();
    UiUtils.showIf(azimuth >= 0, mAvDirection);
    if (azimuth >= 0)
    {
      mAvDirection.setAzimuth(azimuth);
    }
  }

  void shareClickListener(View v)
  {
    if (mMapObject.isTrackRecording())
      return;
    if (mMapObject.isTrack())
    {
      MenuBottomSheetFragment.newInstance(TRACK_SHARE_MENU_ID, getString(R.string.share_track))
          .show(getChildFragmentManager(), TRACK_SHARE_MENU_ID);
    }
    else
      SharingUtils.shareMapObject(requireContext(), mMapObject);
  }

  private void onShareTrackSelected(long trackId, FileType fileType)
  {
    BookmarksSharingHelper.INSTANCE.prepareTrackForSharing(requireActivity(), trackId, fileType);
  }

  @Nullable
  @Override
  public ArrayList<MenuBottomSheetItem> getMenuBottomSheetItems(String id)
  {
    return switch (id)
    {
      case TRACK_SHARE_MENU_ID -> getTrackShareMenuItems();
      default -> null;
    };
  }

  public ArrayList<MenuBottomSheetItem> getTrackShareMenuItems()
  {
    Track track = (Track) mMapObject;
    ArrayList<MenuBottomSheetItem> items = new ArrayList<>();
    items.add(new MenuBottomSheetItem(R.string.export_file, R.drawable.ic_file_kmz,
                                      () -> onShareTrackSelected(track.getTrackId(), FileType.Kml)));
    items.add(new MenuBottomSheetItem(R.string.export_file_gpx, R.drawable.ic_file_gpx,
                                      () -> onShareTrackSelected(track.getTrackId(), FileType.Gpx)));
    items.add(new MenuBottomSheetItem(R.string.export_file_geojson, R.drawable.ic_file_geojson,
                                      () -> onShareTrackSelected(track.getTrackId(), FileType.GeoJson)));
    return items;
  }

  @Override
  public void onPreparedFileForSharing(@NonNull BookmarkSharingResult result)
  {
    BookmarksSharingHelper.INSTANCE.onPreparedFileForSharing(requireActivity(), shareLauncher, result);
  }

  public interface PlacePageViewListener
  {
    // Called when the content has actually changed and we are ready to compute the peek height
    void onPlacePageContentChanged(int previewHeight, int frameHeight);

    void onPlacePageRequestToggleState();
    void onPlacePageRequestClose();
  }

  /**
   * Rounded rectangle background with a triangular arrow on one side,
   * plus a soft shadow behind the arrow to match the popup's elevation shadow.
   */
  private static final class ArrowPopupBackground extends Drawable
  {
    private final Paint mPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Paint mShadowPaint = new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Path mPath = new Path();
    private final RectF mRectF = new RectF();
    private final float mRadius;
    private final float mArrowWidth;
    private final float mArrowHeight;
    private final float mShadowDy;
    private final boolean mArrowOnTop;

    ArrowPopupBackground(int bgColor, int shadowColor, float radius, float arrowWidth, float arrowHeight,
                         float shadowDy, boolean arrowOnTop)
    {
      mRadius = radius;
      mArrowWidth = arrowWidth;
      mArrowHeight = arrowHeight;
      mShadowDy = shadowDy;
      mArrowOnTop = arrowOnTop;
      mPaint.setColor(bgColor);
      mShadowPaint.setColor(shadowColor);
    }

    @Override
    public void draw(@NonNull Canvas canvas)
    {
      final Rect b = getBounds();
      final float cx = b.exactCenterX();
      final float arrowBase = mArrowOnTop ? b.top + mArrowHeight : b.bottom - mArrowHeight;
      final float arrowTip = mArrowOnTop ? b.top : b.bottom;
      final float shadowTip = mArrowOnTop ? b.top - mShadowDy : b.bottom + mShadowDy;

      // Body: rounded rectangle.
      if (mArrowOnTop)
        mRectF.set(b.left, arrowBase, b.right, b.bottom);
      else
        mRectF.set(b.left, b.top, b.right, arrowBase);
      mPath.reset();
      mPath.addRoundRect(mRectF, mRadius, mRadius, Path.Direction.CW);
      canvas.drawPath(mPath, mPaint);

      // Arrow shadow.
      mPath.reset();
      mPath.moveTo(cx - mArrowWidth / 2 - mShadowDy, arrowBase);
      mPath.lineTo(cx, shadowTip);
      mPath.lineTo(cx + mArrowWidth / 2 + mShadowDy, arrowBase);
      mPath.close();
      canvas.drawPath(mPath, mShadowPaint);

      // Arrow triangle.
      mPath.reset();
      mPath.moveTo(cx - mArrowWidth / 2, arrowBase);
      mPath.lineTo(cx, arrowTip);
      mPath.lineTo(cx + mArrowWidth / 2, arrowBase);
      mPath.close();
      canvas.drawPath(mPath, mPaint);
    }

    @Override
    public boolean getPadding(@NonNull Rect padding)
    {
      if (mArrowOnTop)
        padding.set(0, (int) mArrowHeight, 0, 0);
      else
        padding.set(0, 0, 0, (int) mArrowHeight);
      return true;
    }

    @Override
    public void getOutline(@NonNull Outline outline)
    {
      final Rect b = getBounds();
      if (mArrowOnTop)
        outline.setRoundRect(b.left, (int) (b.top + mArrowHeight), b.right, b.bottom, mRadius);
      else
        outline.setRoundRect(b.left, b.top, b.right, (int) (b.bottom - mArrowHeight), mRadius);
    }

    @Override
    public void setAlpha(int alpha)
    {
      mPaint.setAlpha(alpha);
    }
    @Override
    public void setColorFilter(@Nullable ColorFilter cf)
    {
      mPaint.setColorFilter(cf);
    }
    @Override
    public int getOpacity()
    {
      return PixelFormat.TRANSLUCENT;
    }
  }
}
