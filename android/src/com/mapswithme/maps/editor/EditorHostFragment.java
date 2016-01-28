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
    mMode = Mode.CUISINE;
    mToolbarController.setTitle("Cuisine");
    final Bundle args = new Bundle();
    args.putString(CuisineFragment.EXTRA_CURRENT_CUISINE, mEditedObject.getMetadata(Metadata.MetadataType.FMD_CUISINE));
    final Fragment cuisineFragment = Fragment.instantiate(getActivity(), CuisineFragment.class.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, cuisineFragment, CuisineFragment.class.getName())
                             .commit();
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
        mEditedObject.addMetadata(Metadata.MetadataType.FMD_OPEN_HOURS.toInt(), fragment.getTimetable());
        editMapObject();
        break;
      case STREET:
        final String street = ((StreetFragment) getChildFragmentManager().findFragmentByTag(StreetFragment.class.getName())).getStreet();
        mEditedObject.setStreet(street);
        editMapObject();
        break;
      case CUISINE:
        final String cuisine = ((CuisineFragment) getChildFragmentManager().findFragmentByTag(CuisineFragment.class.getName())).getCuisine();
        mEditedObject.addMetadata(Metadata.MetadataType.FMD_CUISINE.toInt(), cuisine);
        editMapObject();
        break;
      case MAP_OBJECT:
        final EditorFragment editorFragment = (EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName());
        Editor.nativeSetMetadata(Metadata.MetadataType.FMD_PHONE_NUMBER.toInt(), editorFragment.getPhone());
        Editor.nativeSetMetadata(Metadata.MetadataType.FMD_WEBSITE.toInt(), editorFragment.getWebsite());
        Editor.nativeSetMetadata(Metadata.MetadataType.FMD_EMAIL.toInt(), editorFragment.getEmail());
        Editor.nativeSetMetadata(Metadata.MetadataType.FMD_CUISINE.toInt(), mEditedObject.getMetadata(Metadata.MetadataType.FMD_CUISINE));
        Editor.nativeSetMetadata(Metadata.MetadataType.FMD_INTERNET.toInt(), editorFragment.getWifi());
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
