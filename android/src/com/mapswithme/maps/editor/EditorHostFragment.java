package com.mapswithme.maps.editor;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.data.MapObject;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.util.Utils;


public class EditorHostFragment extends BaseMwmToolbarFragment
                             implements OnBackPressListener, View.OnClickListener
{
  public static final String EXTRA_MAP_OBJECT = "MapObject";

  enum Mode
  {
    MAP_OBJECT,
    OPENING_HOURS,
    STREET,
    CUISINE
  }
  private Mode mMode;

  private MapObject mEditedObject;

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
    getArguments().setClassLoader(MapObject.class.getClassLoader());
    mEditedObject = getArguments().getParcelable(EditorHostFragment.EXTRA_MAP_OBJECT);
    editMapObject();
    mToolbarController.findViewById(R.id.save).setOnClickListener(this);
    mToolbarController.getToolbar().setNavigationOnClickListener(new View.OnClickListener() {
      @Override
      public void onClick(View v)
      {
        onBackPressed();
      }
    });
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
    saveEditedPoi();
    outState.putParcelable(EXTRA_MAP_OBJECT, mEditedObject);
  }

  protected void editMapObject()
  {
    mMode = Mode.MAP_OBJECT;
    mToolbarController.setTitle("Edit POI");
    final Bundle args = new Bundle();
    args.putParcelable(EXTRA_MAP_OBJECT, mEditedObject);
    final Fragment editorFragment = Fragment.instantiate(getActivity(), EditorFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, editorFragment, EditorFragment.class.getName())
                             .commit();
  }

  protected void editTimetable()
  {
    saveEditedPoi();
    mMode = Mode.OPENING_HOURS;
    mToolbarController.setTitle("Opening hours");
    final Bundle args = new Bundle();
    args.putString(TimetableFragment.EXTRA_TIME, mEditedObject.getMetadata(Metadata.MetadataType.FMD_OPEN_HOURS));
    final Fragment editorFragment = Fragment.instantiate(getActivity(), TimetableFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, editorFragment, TimetableFragment.class.getName())
                             .commit();
  }

  protected void editStreet()
  {
    saveEditedPoi();
    mMode = Mode.STREET;
    mToolbarController.setTitle("Add Street");
    final Bundle args = new Bundle();
    args.putString(StreetFragment.EXTRA_CURRENT_STREET, mEditedObject.getStreet());
    final Fragment streetFragment = Fragment.instantiate(getActivity(), StreetFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, streetFragment, StreetFragment.class.getName())
                             .commit();
  }

  protected void editCuisine()
  {
    saveEditedPoi();
    mMode = Mode.CUISINE;
    mToolbarController.setTitle("Cuisine");
    final Bundle args = new Bundle();
    args.putString(CuisineFragment.EXTRA_CURRENT_CUISINE, mEditedObject.getMetadata(Metadata.MetadataType.FMD_CUISINE));
    final Fragment cuisineFragment = Fragment.instantiate(getActivity(), CuisineFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, cuisineFragment, CuisineFragment.class.getName())
                             .commit();
  }

  protected void saveEditedPoi()
  {
    final EditorFragment editorFragment = (EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName());
    mEditedObject.addMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER, editorFragment.getPhone());
    mEditedObject.addMetadata(Metadata.MetadataType.FMD_WEBSITE, editorFragment.getWebsite());
    mEditedObject.addMetadata(Metadata.MetadataType.FMD_EMAIL, editorFragment.getEmail());
    mEditedObject.addMetadata(Metadata.MetadataType.FMD_CUISINE, editorFragment.getCuisine());
    mEditedObject.addMetadata(Metadata.MetadataType.FMD_INTERNET, editorFragment.getWifi());
    mEditedObject.addMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, editorFragment.getOpeningHours());
    mEditedObject.setName(editorFragment.getName());
    mEditedObject.setStreet(editorFragment.getStreet());
    mEditedObject.setHouseNumber(editorFragment.getHouseNumber());
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
        mEditedObject.addMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, fragment.getTimetable());
        editMapObject();
        break;
      case STREET:
        final String street = ((StreetFragment) getChildFragmentManager().findFragmentByTag(StreetFragment.class.getName())).getStreet();
        mEditedObject.setStreet(street);
        editMapObject();
        break;
      case CUISINE:
        String cuisine = ((CuisineFragment) getChildFragmentManager().findFragmentByTag(CuisineFragment.class.getName())).getCuisine();
        mEditedObject.addMetadata(Metadata.MetadataType.FMD_CUISINE, cuisine);
        editMapObject();
        break;
      case MAP_OBJECT:
        final EditorFragment editorFragment = (EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName());
        Editor.setMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER, editorFragment.getPhone());
        Editor.setMetadata(Metadata.MetadataType.FMD_WEBSITE, editorFragment.getWebsite());
        Editor.setMetadata(Metadata.MetadataType.FMD_EMAIL, editorFragment.getEmail());
        Editor.setMetadata(Metadata.MetadataType.FMD_CUISINE, editorFragment.getCuisine());
        Editor.setMetadata(Metadata.MetadataType.FMD_INTERNET, editorFragment.getWifi());
        Editor.setMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, editorFragment.getOpeningHours());
        Editor.nativeSetName(editorFragment.getName());
        Editor.nativeEditFeature(editorFragment.getStreet(), editorFragment.getHouseNumber());
        if (OsmOAuth.isAuthorized())
          Utils.navigateToParent(getActivity());
        else
          showAuthorization();
        break;
      }
    }
  }

  private void showAuthorization()
  {
    getMwmActivity().replaceFragment(AuthFragment.class, null, null);
  }
}
