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

import android.content.Context;
import android.content.Intent;

public class MWMResponse
{
  private MWMPoint mPoint;

  /**
   *
   * @return point, for which user requested more information in MapsWithMe application.
   */
  public MWMPoint getPoint()     { return mPoint; }
  public boolean  hasPoint()     { return mPoint != null; }

  @Override
  public String toString()
  {
    return "MWMResponse [SelectedPoint=" + mPoint + "]";
  }

  /**
   * Factory method to extract response data from intent.
   *
   * @param context
   * @param intent
   * @return
   */
  public static MWMResponse extractFromIntent(Context context, Intent intent)
  {
    final MWMResponse response = new MWMResponse();
    // parse status
    // parse point
    final double lat = intent.getDoubleExtra(Const.EXTRA_MWM_RESPONSE_POINT_LAT, 0);
    final double lon = intent.getDoubleExtra(Const.EXTRA_MWM_RESPONSE_POINT_LON, 0);
    final String name = intent.getStringExtra(Const.EXTRA_MWM_RESPONSE_POINT_NAME);
    final String id = intent.getStringExtra(Const.EXTRA_MWM_RESPONSE_POINT_ID);
    response.mPoint = new MWMPoint(lat, lon, name, id);

    return response;
  }

  private MWMResponse() {}
}
