package com.mapswithme.maps.auth;

import java.util.Arrays;
import java.util.List;

class Constants
{
  static final int REQ_CODE_GET_SOCIAL_TOKEN = 101;
  static final String EXTRA_SOCIAL_TOKEN = "extra_social_token";
  static final String EXTRA_TOKEN_TYPE = "extra_token_type";
  static final String EXTRA_AUTH_ERROR = "extra_auth_error";
  static final String EXTRA_IS_CANCEL = "extra_is_cancel";
  static final List<String> FACEBOOK_PERMISSIONS =
      Arrays.asList("email", "user_friends");
}
