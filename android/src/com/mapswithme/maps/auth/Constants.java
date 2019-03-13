package com.mapswithme.maps.auth;

import java.util.Collections;
import java.util.List;

class Constants
{
  static final int REQ_CODE_PHONE_AUTH_RESULT = 102;
  static final int REQ_CODE_GOOGLE_SIGN_IN = 103;
  static final String EXTRA_SOCIAL_TOKEN = "extra_social_token";
  static final String EXTRA_PHONE_AUTH_TOKEN = "extra_phone_auth_token";
  static final String EXTRA_TOKEN_TYPE = "extra_token_type";
  static final String EXTRA_AUTH_ERROR = "extra_auth_error";
  static final String EXTRA_IS_CANCEL = "extra_is_cancel";
  static final String EXTRA_PRIVACY_POLICY_ACCEPTED = "extra_privacy_policy_accepted";
  static final String EXTRA_TERMS_OF_USE_ACCEPTED = "extra_terms_of_use_accepted";
  static final String EXTRA_PROMO_ACCEPTED = "extra_promo_accepted";
  static final List<String> FACEBOOK_PERMISSIONS =
      Collections.singletonList("email");
}
