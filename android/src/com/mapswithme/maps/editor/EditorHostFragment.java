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
import com.mapswithme.maps.editor.data.LocalizedStreet;
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
    CUISINE,
    LANGUAGE
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
    final Bundle args = new Bundle();
    args.putString(TimetableFragment.EXTRA_TIME, Editor.nativeGetOpeningHours());
    editWithFragment(Mode.OPENING_HOURS, R.string.editor_time_title, args, TimetableFragment.class, false);
  }

  protected void editStreet()
  {
    editWithFragment(Mode.STREET, R.string.choose_street, null, StreetFragment.class, false);
  }

  protected void editCuisine()
  {
    editWithFragment(Mode.CUISINE, R.string.select_cuisine, null, CuisineFragment.class, true);
  }

  protected void addLocalizedLanguage()
  {
    editWithFragment(Mode.LANGUAGE, R.string.choose_language, null, LanguagesFragment.class, false);
  }

  private void editWithFragment(Mode newMode, @StringRes int toolbarTitle, @Nullable Bundle args, Class<? extends Fragment> fragmentClass, boolean showSearch)
  {
    if (!setEdits())
      return;

    mMode = newMode;
    mToolbarController.setTitle(toolbarTitle);
    ((SearchToolbarController) mToolbarController).showControls(showSearch);
    final Fragment fragment = Fragment.instantiate(getActivity(), fragmentClass.getName(), args);
    getChildFragmentManager().beginTransaction()
                             .replace(R.id.fragment_container, fragment, fragmentClass.getName())
                             .commit();
  }

  protected void editCategory()
  {
    if (!mIsNewObject)
      return;

    final Activity host = getActivity();
    host.finish();
    startActivity(new Intent(host, FeatureCategoryActivity.class));
  }

  private boolean setEdits()
  {
    return ((EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName())).setEdits();
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
          Editor.nativeSetOpeningHours(timetables);
          editMapObject();
        }
        else
        {
          // TODO (yunikkk) correct translation
          showMistakeDialog(R.string.editor_correct_mistake);
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
        if (!setEdits())
          return;

        // Save note
        final String note = ((EditorFragment) getChildFragmentManager().findFragmentByTag(EditorFragment.class.getName())).getDescription();
        if (note.length() != 0)
          Editor.nativeCreateNote(note);
        // Save object edits
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

  private void showMistakeDialog(@StringRes int resId)
  {
    new AlertDialog.Builder(getActivity())
        .setMessage(resId)
        .setPositiveButton(android.R.string.ok, null)
        .show();
  }

  public void setStreet(LocalizedStreet street)
  {
    Editor.nativeSetStreet(street);
    editMapObject();
  }

  public boolean addingNewObject()
  {
    return mIsNewObject;
  }
}
