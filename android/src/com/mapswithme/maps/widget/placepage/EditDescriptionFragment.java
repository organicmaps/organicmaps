package com.mapswithme.maps.widget.placepage;

import android.os.Bundle;
import android.support.annotation.Nullable;
import android.support.v4.app.DialogFragment;
import android.support.v7.widget.Toolbar;
import android.text.Html;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.widget.EditText;
import android.widget.TextView;

import com.mapswithme.maps.R;
import com.mapswithme.maps.base.BaseMwmDialogFragment;
import com.mapswithme.util.StringUtils;
import com.mapswithme.util.UiUtils;

public class EditDescriptionFragment extends BaseMwmDialogFragment
{
  public static final String EXTRA_DESCRIPTION = "ExtraDescription";

  private EditText mEtDescription;

  public interface OnDescriptionSaveListener
  {
    void onSave(String description);
  }

  private OnDescriptionSaveListener mListener;

  public EditDescriptionFragment() {}

  @Override
  public void onCreate(@Nullable Bundle savedInstanceState)
  {
    super.onCreate(savedInstanceState);
    setStyle(DialogFragment.STYLE_NORMAL, R.style.MwmMain_DialogFragment_Fullscreen);
  }

  @Nullable
  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState)
  {
    return inflater.inflate(R.layout.fragment_edit_description, container, false);
  }

  @Override
  public void onViewCreated(View view, Bundle savedInstanceState)
  {
    super.onViewCreated(view, savedInstanceState);

    String description = getArguments().getString(EXTRA_DESCRIPTION);

    if (StringUtils.isHtml(description))
    {
      final String descriptionNoSimpleTags = StringUtils.removeEditTextHtmlTags(description);
      if (!StringUtils.isHtml(descriptionNoSimpleTags))
        description = Html.fromHtml(description).toString();
    }

    mEtDescription = (EditText) view.findViewById(R.id.et__description);
    mEtDescription.setText(description);
    initToolbar(view);

    mEtDescription.requestFocus();
    getDialog().getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE);
  }

  public void setSaveDescriptionListener(OnDescriptionSaveListener listener)
  {
    mListener = listener;
  }

  private void initToolbar(View view)
  {
    Toolbar toolbar = (Toolbar) view.findViewById(R.id.toolbar);
    final TextView textView = (TextView) toolbar.findViewById(R.id.tv__save);
    textView.setOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        saveDescription();
      }
    });
    UiUtils.showHomeUpButton(toolbar);
    toolbar.setTitle(R.string.description);
    toolbar.setNavigationOnClickListener(new View.OnClickListener()
    {
      @Override
      public void onClick(View v)
      {
        dismiss();
      }
    });
  }

  private void saveDescription()
  {
    if (mListener != null)
      mListener.onSave(mEtDescription.getText().toString());
    dismiss();
  }
}
