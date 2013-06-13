package com.mapswithme.maps.api;

import android.app.Dialog;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.view.View;

import com.mapwithme.maps.api.R;

public class DownloadMapsWithMeDialog extends Dialog implements android.view.View.OnClickListener
{

  public DownloadMapsWithMeDialog(Context context)
  {
    super(context);

    setTitle(R.string.mapswithme);
    setContentView(R.layout.dlg_install_mwm);
    findViewById(R.id.download).setOnClickListener(this);
  }


  public void onDownloadButtonClicked(){
    Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(getContext().getString(R.string.downolad_url)));
    getContext().startActivity(i);
    dismiss();
  }


  @Override
  public void onClick(View v)
  {
    if (v.getId() == R.id.download)
      onDownloadButtonClicked();
  }
}
