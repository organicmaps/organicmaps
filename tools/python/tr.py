#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# Copyright (C) 2010 Google Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Simple command-line example for Translate.

Command-line application that translates some text.
"""

__author__ = 'jcgregorio@google.com (Joe Gregorio)'

from apiclient.discovery import build
import sys

def translate(text):

  # Build a service object for interacting with the API. Visit
  # the Google APIs Console <http://code.google.com/apis/console>
  # to get an API key for your own application.
  service = build('translate', 'v2',
            developerKey='AIzaSyDD5rPHpqmeEIRVI34wYI1zMplMq9O_w2k')
  langList = ["ja", "fr", "ar", "de", "ru", "sv", "zh", "fi", "ko", "be", "nl", "ga", "el", "it", "es", "th", "ca", "cy", "hu", "sr", "fa", "pl", "uk", "sl", "ro", "sq", "cs", "sk", "af", "hr", "tr", "pt", "lt", "bg", "et", "vi", "mk", "lv", "is", "hi"]
  resText = ''
  try:
    for lang in langList:
      res = service.translations().list(
        source='en',
        target=lang,
        q=text
      ).execute()
  
      for trText in res['translations']:
        resText += '|' + lang + ':' + trText['translatedText']
  except:
    pass
  return resText

if __name__ == '__main__':
  reload(sys)
  sys.setdefaultencoding('utf-8')
  for line in sys.stdin:
    line = line.rstrip('\n\r')
    retText = 'en:' + line + translate(line)
    print retText
