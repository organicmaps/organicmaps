package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmApplication;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;

public class EditorHostFragment extends BaseMwmToolbarFragment
                             implements OnBackPressListener, View.OnClickListener
{
  private static final String PREF_LAST_AUTH_DISPLAY_TIMESTAMP = "LastAuth";
  private boolean mIsNewObject;

  enum Mode
  {
    MAP_OBJECT,
    OPENING_HOURS,
    STREET,
    CUISINE
  }

  private Mode mMode;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_editor_host, container, false);
  }

  @Override
  public void onViewCreated(View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    editMapObject();
    mToolbarController.findViewById(R.id.save).setOnClickListener(this);
    mToolbarController.getToolbar().setNavigationOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        onBackPressed();
      }
    });

    if (getArguments() != null)
      mIsNewObject = getArguments().getBoolean(EditorActivity.EXTRA_NEW_OBJECT, false);
    mToolbarController.setTitle(getTitle());
  }

  @StringRes
  private int getTitle()
  {
    // TODO return other resid for add place mode
    return mIsNewObject ? R.string.edit_place : R.string.edit_place;
  }

  @Override
  public boolean onBackPressed()
  {
    switch (mMode)
    {
    case OPENING_HOURS:
    case STREET:
    case CUISINE:
      editMapObject();
      break;
    default:
      Utils.navigateToParent(getActivity());
    }
    return true;
  }

  @Override
  public void onSaveInstanceState(Bundle outState)
  {
    super.onSaveInstanceState(outState);
    temporaryStoreEdits();
  }

  protected void editMapObject()
  {
    mMode = Mode.MAP_OBJECT;
    mToolbarController.setTitle(getTitle());
    final Fragment editorFragment = Fragment.instantiate(getActivity(), EditorFragment.class.getName());
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, editorFragment, EditorFragment.class.getName())
                             .commit();
  }
  protected void editTimetable()
  {
    temporaryStoreEdits();
    mMode = Mode.OPENING_HOURS;
    mToolbarController.setTitle(R.string.editor_time_title);
    final Bundle args = new Bundle();
    args.putString(TimetableFragment.EXTRA_TIME, Editor.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS));
    final Fragment editorFragment = Fragment.instantiate(getActivity(), TimetableFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, editorFragment, TimetableFragment.class.getName())
                             .commit();
  }

  protected void editStreet()
  {
    temporaryStoreEdits();
    mMode = Mode.STREET;
    mToolbarController.setTitle(R.string.choose_street);
    final Bundle args = new Bundle();
    args.putString(StreetFragment.EXTRA_CURRENT_STREET, Editor.nativeGetStreet());
    final Fragment streetFragment = Fragment.instantiate(getActivity(), StreetFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, streetFragment, StreetFragment.class.getName())
                             .commit();
  }

  protected void editCuisine()
  {
    temporaryStoreEdits();
    mMode = Mode.CUISINE;
    mToolbarController.setTitle(R.string.select_cuisine);
    final Bundle args = new Bundle();
    args.putString(CuisineFragment.EXTRA_CURRENT_CUISINE, Editor.getMetadata(Metadata.MetadataType.FMD_CUISINE));
    final Fragment cuisineFragment = Fragment.instantiate(getActivity(), CuisineFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, cuisineFragment, CuisineFragment.class.getName())
                             .commit();
  }

  protected void temporaryStoreEdits()
  {
    final EditorFragment editorFragment = (EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName());
    Editor.setMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, editorFragment.getOpeningHours());
    Editor.setMetadata(Metadata.MetadataType.FMD_CUISINE, editorFragment.getCuisine());
    Editor.setMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER, editorFragment.getPhone());
    Editor.setMetadata(Metadata.MetadataType.FMD_WEBSITE, editorFragment.getWebsite());
    Editor.setMetadata(Metadata.MetadataType.FMD_EMAIL, editorFragment.getEmail());
    Editor.setMetadata(Metadata.MetadataType.FMD_INTERNET, editorFragment.getWifi());
    Editor.nativeSetDefaultName(editorFragment.getName());
    Editor.nativeSetHouseNumber(editorFragment.getHouseNumber());
    Editor.nativeSetStreet(editorFragment.getStreet());
  }

  @Override
  public void onClick(View v)
  {
    if (v.getId() == R.id.save)
    {
      switch (mMode)
      {
      case OPENING_HOURS:
        final TimetableFragment fragment = (TimetableFragment) getChildFragmentManager().findFragmentByTag(TimetableFragment.class.getName());
        Editor.setMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, fragment.getTimetable());
        editMapObject();
        break;
      case STREET:
        setStreet(((StreetFragment) getChildFragmentManager().findFragmentByTag(StreetFragment.class.getName())).getStreet());
        break;
      case CUISINE:
        String cuisine = ((CuisineFragment) getChildFragmentManager().findFragmentByTag(CuisineFragment.class.getName())).getCuisine();
        Editor.setMetadata(Metadata.MetadataType.FMD_CUISINE, cuisine);
        editMapObject();
        break;
      case MAP_OBJECT:
        final EditorFragment editorFragment = (EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName());
        Editor.setMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER, editorFragment.getPhone());
        Editor.setMetadata(Metadata.MetadataType.FMD_WEBSITE, editorFragment.getWebsite());
        Editor.setMetadata(Metadata.MetadataType.FMD_EMAIL, editorFragment.getEmail());
        Editor.setMetadata(Metadata.MetadataType.FMD_INTERNET, editorFragment.getWifi());
        Editor.nativeSetDefaultName(editorFragment.getName());
        // Street, cuisine and opening hours are saved in separate cases.
        Editor.nativeSetHouseNumber(editorFragment.getHouseNumber());
        if (Editor.nativeSaveEditedFeature())
        {
          if (OsmOAuth.isAuthorized() || !ConnectionState.isConnected())
            Utils.navigateToParent(getActivity());
          else
            showAuthorization();
        }
        else
        {
          // TODO(yunikkk) set correct error text.
          UiUtils.showAlertDialog(getActivity(), R.string.downloader_no_space_title);
        }
        break;
      }
    }
  }

  public void setStreet(String street)
  {
    Editor.nativeSetStreet(street);
    editMapObject();
  }

  private void showAuthorization()
  {
    if (!MwmApplication.prefs().contains(PREF_LAST_AUTH_DISPLAY_TIMESTAMP))
    {
      MwmApplication.prefs().edit().putLong(PREF_LAST_AUTH_DISPLAY_TIMESTAMP, System.currentTimeMillis()).apply();
      getMwmActivity().replaceFragment(AuthFragment.class, null, null);
    }
    else
      mToolbarController.onUpClick();
  }
}
