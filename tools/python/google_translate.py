#!/usr/bin/python

import re
import sys
import urllib
import simplejson
import time
 
baseUrl = "http://www.googleapis.com/language/translate/v2"
 
def translate(text,src='en'):
    targetLangs = ["ja", "fr", "ar", "de", "ru", "sv", "zh", "fi", "ko", "ka", "be", "nl", "ga", "el", "it", "es", "th", "ca", "cy", "hu", "sr", "fa", "eu", "pl", "uk", "sl", "ro", "sq", "cs", "sk", "af", "hr", "hy", "tr", "pt", "lt", "bg", "la", "et", "vi", "mk", "lv", "is", "hi"]
    retText=''
    for target in targetLangs:
        params = ({'source': src,
             'target': target,
             'key': 'AIzaSyDD5rPHpqmeEIRVI34wYI1zMplMq9O_w2k'
             })
        translation = target + ':'
        params['q'] = text
        resp = simplejson.load(urllib.urlopen('%s' % (baseUrl), data = urllib.urlencode(params)))
        print resp
        try:
            translation += resp['data']['translations']['translatedText']
        except:
            return retText
        retText += '|' + translation
    return retText
 
def test():
    for line in sys.stdin:
        line = line.rstrip('\n\r')
        retText = 'en:' + line + translate(line)
        print retText
 
 
if __name__=='__main__':
    reload(sys)
    sys.setdefaultencoding('utf-8')
    try:
        test()
    except KeyboardInterrupt:
        print "\n"
        sys.exit(0)
