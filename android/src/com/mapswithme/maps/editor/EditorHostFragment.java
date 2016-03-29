package com.mapswithme.maps.editor;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.annotation.StringRes;
import android.support.v4.app.Fragment;
import android.support.v7.app.AlertDialog;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.mapswithme.maps.MwmActivity;
import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmToolbarFragment;
import com.mapswithme.maps.base.OnBackPressListener;
import com.mapswithme.maps.bookmarks.data.Metadata;
import com.mapswithme.maps.widget.SearchToolbarController;
import com.mapswithme.maps.widget.ToolbarController;
import com.mapswithme.util.ConnectionState;
import com.mapswithme.util.UiUtils;
import com.mapswithme.util.Utils;
import com.mapswithme.util.statistics.Statistics;

public class EditorHostFragment extends BaseMwmToolbarFragment
                             implements OnBackPressListener, View.OnClickListener
{
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

    editMapObject();
  }

  @StringRes
  private int getTitle()
  {
    return mIsNewObject ? R.string.editor_add_place_title : R.string.editor_edit_place_title;
  }

  @Override
  protected ToolbarController onCreateToolbarController(@NonNull View root)
  {
    return new SearchToolbarController(root, getActivity())
    {
      @Override
      protected void onTextChanged(String query)
      {
        ((CuisineFragment) getChildFragmentManager().findFragmentByTag(CuisineFragment.class.getName())).setFilter(query);
      }
    };
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
    ((SearchToolbarController) mToolbarController).showControls(false);
    mToolbarController.setTitle(getTitle());
    final Fragment editorFragment = Fragment.instantiate(getActivity(), EditorFragment.class.getName());
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, editorFragment, EditorFragment.class.getName())
                             .commit();
  }

  protected void editTimetable()
  {
    setEdits();
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
    setEdits();
    mMode = Mode.STREET;
    mToolbarController.setTitle(R.string.choose_street);
    final Fragment streetFragment = Fragment.instantiate(getActivity(), StreetFragment.class.getName());
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, streetFragment, StreetFragment.class.getName())
                             .commit();
  }

  protected void editCuisine()
  {
    setEdits();
    mMode = Mode.CUISINE;
    mToolbarController.setTitle("");
    ((SearchToolbarController) mToolbarController).showControls(true);
    final Fragment cuisineFragment = Fragment.instantiate(getActivity(), CuisineFragment.class.getName());
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, cuisineFragment, CuisineFragment.class.getName())
                             .commit();
  }

  private void setEdits()
  {
    ((EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName())).setEdits();
  }

  @Override
  public void onClick(View v)
  {
    if (v.getId() == R.id.save)
    {
      switch (mMode)
      {
      case OPENING_HOURS:
        final String timetables = ((TimetableFragment) getChildFragmentManager().findFragmentByTag(TimetableFragment.class.getName())).getTimetable();
        if (OpeningHours.nativeIsTimetableStringValid(timetables))
        {
          Editor.setMetadata(Metadata.MetadataType.FMD_OPEN_HOURS, timetables);
          editMapObject();
        }
        else
        {
          new AlertDialog.Builder(getActivity())
              .setMessage(R.string.editor_correct_mistake)
              .setPositiveButton(android.R.string.ok, null)
              .show();
        }
        break;
      case STREET:
        setStreet(((StreetFragment) getChildFragmentManager().findFragmentByTag(StreetFragment.class.getName())).getStreet());
        break;
      case CUISINE:
        String[] cuisines = ((CuisineFragment) getChildFragmentManager().findFragmentByTag(CuisineFragment.class.getName())).getCuisines();
        Editor.nativeSetSelectedCuisines(cuisines);
        editMapObject();
        break;
      case MAP_OBJECT:
        setEdits();
        if (Editor.nativeSaveEditedFeature())
        {
          Statistics.INSTANCE.trackEditorSuccess(mIsNewObject);
          if (OsmOAuth.isAuthorized() || !ConnectionState.isConnected())
            Utils.navigateToParent(getActivity());
          else
          {
            final Activity parent = getActivity();
            Intent intent = new Intent(parent, MwmActivity.class);
            intent.addFlags(Intent.FLAG_ACTIVITY_NO_ANIMATION);
            intent.putExtra(MwmActivity.EXTRA_TASK, new MwmActivity.ShowAuthorizationTask());
            parent.startActivity(intent);

            if (parent instanceof MwmActivity)
              ((MwmActivity) parent).customOnNavigateUp();
            else
              parent.finish();
          }
        }
        else
        {
          Statistics.INSTANCE.trackEditorError(mIsNewObject);
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
}
