#!/bin/sh
#
# Copyright 2010-present Facebook.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Note: Use of this script requires Perl

#
# step 1 - confirm we have an app id and app secret to work with
#

if [ -z "$APPID" ]
then
  echo '$APPID must be exported and set to the application id for the sample before running this script'
  exit 1
fi

if [ -z "$APPSECRET" ]
then
  echo '$APPSECRET must be exported set to the app secret for the sample before running this script'
  exit 1
fi

#
# step 2 - stage images and capture their URIs in variables
#

echo curling...

ROCK_IMAGE_URI=` \
  curl -s -k -X POST https://graph.facebook.com/$APPID/staging_resources -F access_token="$APPID|$APPSECRET" -F 'file=@res/drawable/left_rock.png;type=image/png' \
  | perl -ne '/"uri":"(.*)"}/ && print $1' `

PAPER_IMAGE_URI=` \
  curl -s -k -X POST https://graph.facebook.com/$APPID/staging_resources -F access_token="$APPID|$APPSECRET" -F 'file=@res/drawable/left_paper.png;type=image/png' \
  | perl -ne '/"uri":"(.*)"}/ && print $1' `

SCISSORS_IMAGE_URI=` \
  curl -s -k -X POST https://graph.facebook.com/$APPID/staging_resources -F access_token="$APPID|$APPSECRET" -F 'file=@res/drawable/left_scissors.png;type=image/png' \
  | perl -ne '/"uri":"(.*)"}/ && print $1' `

echo "created staged resources..."
echo "  rock=$ROCK_IMAGE_URI"
echo "  paper=$PAPER_IMAGE_URI"
echo "  scissors=$SCISSORS_IMAGE_URI"

# step 3 - create facebook hosted applink page for the app:
# For mobile only app, facebook provide applink host service to generate a page: https://developers.facebook.com/docs/graph-api/reference/v2.0/app/app_link_hosts

echo "creating facebook host applink page for mobile-only app:"

FB_APPLINK_HOST_ID=` \
curl https://graph.facebook.com/app/app_link_hosts -F access_token="$APPID|$APPSECRET" -F pretty=true -F name="RPSSample" \
-F ios='[
    {
      "url" : "rps-sample-applink-example://",
      "app_store_id" : 794163692,
      "app_name" : "RPS Sample",
    },
  ]' \
-F android=' [
    {
      "package" : "com.facebook.samples.rps",
      "app_name" : "RPS Sample",
    },
  ]' \
-F web=' {
    "should_fallback" : false,
  }' \
| perl -ne '/"id":\s*"(.*)"/ && print $1'`

FB_APPLINK_HOST_URL=` \
curl -X GET https://graph.facebook.com/v2.0/$FB_APPLINK_HOST_ID?access_token="$APPID|$APPSECRET" \
| perl -ne '/"canonical_url":\s*"(.*)"/ && print $1' `

echo "  applink host url id: $FB_APPLINK_HOST_ID"
echo "  applink host url: $FB_APPLINK_HOST_URL"

#
# step 4 - create objects and capture their IDs in variables
#

# rock
ROCK_OBJID=` \
  curl -s -X POST -F "object={\"title\":\"Rock\",\"description\":\"Breaks scissors, alas is covered by paper.\",\"image\":\"$ROCK_IMAGE_URI\",\"url\":\"$FB_APPLINK_HOST_URL?gesture=rock\"}" "https://graph.facebook.com/$APPID/objects/fb_sample_rps:gesture?access_token=$APPID|$APPSECRET" \
  | perl -ne '/"id":"(.*)"}/ && print $1' `

# paper
PAPER_OBJID=` \
  curl -s -X POST -F "object={\"title\":\"Paper\",\"description\":\"Covers rock, sadly scissors cut it.\",\"image\":\"$PAPER_IMAGE_URI\",\"url\":\"$FB_APPLINK_HOST_URL?gesture=paper\"}" "https://graph.facebook.com/$APPID/objects/fb_sample_rps:gesture?access_token=$APPID|$APPSECRET" \
  | perl -ne '/"id":"(.*)"}/ && print $1' `

# scissors
SCISSORS_OBJID=` \
  curl -s -X POST -F "object={\"title\":\"Scissors\",\"description\":\"Cuts paper, broken by rock -- bother.\",\"image\":\"$SCISSORS_IMAGE_URI\",\"url\":\"$FB_APPLINK_HOST_URL?gesture=scissors\"}" "https://graph.facebook.com/$APPID/objects/fb_sample_rps:gesture?access_token=$APPID|$APPSECRET" \
  | perl -ne '/"id":"(.*)"}/ && print $1' `

#
# step 5 - echo progress
#

echo "created application objects..."
echo "  rock=$ROCK_OBJID"
echo "  paper=$PAPER_OBJID"
echo "  scissors=$SCISSORS_OBJID"

#
# step 6 - write .java file for common objects
#

MFILE=src/com/facebook/samples/rps/CommonObjects.java

cat > $MFILE << EOF
/*
 * Copyright 2010-present Facebook.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.samples.rps;

public class CommonObjects {
    public static final String BUILT_IN_OPEN_GRAPH_OBJECTS[] = {
            "$ROCK_OBJID", // rock
            "$PAPER_OBJID", // paper
            "$SCISSORS_OBJID"  // scissors
    };
}

EOF

echo "created $MFILE ..."
echo done.
