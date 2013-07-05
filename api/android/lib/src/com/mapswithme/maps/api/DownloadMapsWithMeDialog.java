/******************************************************************************
   Copyright (c) 2013, MapsWithMe GmbH All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  Redistributions of source code must retain the above copyright notice, this list
  of conditions and the following disclaimer. Redistributions in binary form must
  reproduce the above copyright notice, this list of conditions and the following
  disclaimer in the documentation and/or other materials provided with the
  distribution. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.
******************************************************************************/
package com.mapswithme.maps.api;

import android.app.Activity;
import android.app.Dialog;
import android.content.Intent;
import android.net.Uri;
import android.view.View;
import android.view.Window;

import com.mapwithme.maps.api.R;

public class DownloadMapsWithMeDialog extends Dialog implements android.view.View.OnClickListener
{

  public DownloadMapsWithMeDialog(Activity activity)
  {
    super(activity);

    requestWindowFeature(Window.FEATURE_NO_TITLE);
    setContentView(R.layout.dlg_install_mwm);

    findViewById(R.id.btn_lite).setOnClickListener(this);
    findViewById(R.id.btn_pro).setOnClickListener(this);

    setOwnerActivity(activity);
  }


  public void onDownloadButtonClicked(String url)
  {
    Intent i = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
    getContext().startActivity(i);
    dismiss();
  }


  @Override
  public void onClick(View v)
  {
    String url = getContext().getString(v.getId() == R.id.btn_lite ? R.string.url_lite : R.string.url_pro);
    onDownloadButtonClicked(url);
  }
}
