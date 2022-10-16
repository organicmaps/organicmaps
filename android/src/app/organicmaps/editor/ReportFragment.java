package app.organicmaps.editor;

import android.os.Bundle;
import android.text.TextUtils;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;

import androidx.annotation.IntRange;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import app.organicmaps.R;
import app.organicmaps.base.BaseMwmToolbarFragment;
import app.organicmaps.util.UiUtils;

public class ReportFragment extends BaseMwmToolbarFragment implements View.OnClickListener
{
  private View mSimpleProblems;
  private View mAdvancedProblem;
  private View mSave;
  private EditText mProblemInput;

  private boolean mAdvancedMode;
  @IntRange(from = 0, to = 3)
  private int mSelectedProblem;

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container, @Nullable Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_report, container, false);
  }

  @Override
  public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);
    getToolbarController().setTitle(R.string.editor_report_problem_title);

    mSave = getToolbarController().getToolbar().findViewById(R.id.save);
    mSave.setOnClickListener(this);
    mSimpleProblems = view.findViewById(R.id.ll__problems);
    mSimpleProblems.findViewById(R.id.problem_not_exist).setOnClickListener(this);
//    mSimpleProblems.findViewById(R.id.problem_closed_repair).setOnClickListener(this);
//    mSimpleProblems.findViewById(R.id.problem_duplicated_place).setOnClickListener(this);
    mSimpleProblems.findViewById(R.id.problem_other).setOnClickListener(this);
    mAdvancedProblem = view.findViewById(R.id.ll__other_problem);
    mProblemInput = mAdvancedProblem.findViewById(R.id.input);
    refreshProblems();
  }

  private void refreshProblems()
  {
    UiUtils.showIf(mAdvancedMode, mAdvancedProblem, mSave);
    UiUtils.showIf(!mAdvancedMode, mSimpleProblems);
  }

  private void send(String text)
  {
    Editor.nativeCreateNote(text);
    getToolbarController().onUpClick();
  }

  private void sendNotExist()
  {
    Editor.nativePlaceDoesNotExist("");
    getToolbarController().onUpClick();
  }

  @Override
  public void onClick(View v)
  {
    switch (v.getId())
    {
    case R.id.problem_not_exist:
//    case R.id.problem_closed_repair:
//    case R.id.problem_duplicated_place:
      sendNotExist();
      break;

    case R.id.problem_other:
      mAdvancedMode = true;
      refreshProblems();
      break;

    case R.id.save:
      String text = mProblemInput.getText().toString().trim();
      if (TextUtils.isEmpty(text))
        return;

      send(text);
      break;
    }
  }
}
