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

public class Const
{

  /* Request extras */
  static final String AUTHORITY = "com.mapswithme.maps.api";
  public static final String EXTRA_URL = AUTHORITY +  ".url";
  public static final String EXTRA_TITLE = AUTHORITY + ".title";
  public static final String EXTRA_API_VERSION = AUTHORITY + ".version";
  public static final String EXTRA_CALLER_APP_INFO = AUTHORITY + ".caller_app_info";
  public static final String EXTRA_HAS_PENDING_INTENT = AUTHORITY + ".has_pen_intent";
  public static final String EXTRA_CALLER_PENDING_INTENT = AUTHORITY + ".pending_intent";


  /* Response extras */
  /* Point part-by-part*/
  public static final String EXTRA_MWM_RESPONSE_POINT_NAME = AUTHORITY + ".point_name";
  public static final String EXTRA_MWM_RESPONSE_POINT_LAT = AUTHORITY + ".point_lat";
  public static final String EXTRA_MWM_RESPONSE_POINT_LON = AUTHORITY + ".point_lon";
  public static final String EXTRA_MWM_RESPONSE_POINT_ID = AUTHORITY + ".point_id";


  public static final String ACTION_MWM_REQUEST = AUTHORITY + ".request";
  static final int API_VERSION = 1;
  static final String CALLBACK_PREFIX = "mapswithme.client.";

  private Const() {}
}
