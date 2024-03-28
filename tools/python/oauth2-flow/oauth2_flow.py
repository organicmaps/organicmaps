import http.client
import logging
import re
import urllib.parse
from http.cookies import SimpleCookie


logging.basicConfig(level=logging.DEBUG, format='%(asctime)s [%(levelname)-5s] %(message)s')

# Prod enviroment
"""
osmHost = "www.openstreetmap.org"
OAuthClientId = "nw9bW3nZ-q99SXzgnH-dlED3ueDSmFPtxl33n3hDwFU"
OAuthClientSecret = "nIxwFx1NXIx9lKoNmb7lAoHd9ariGMf46PtU_YG558c"
OAuthRedirectUri = "om://oauth2/osm/callback"
OAuthResponseType = "code"
OAuthScope = "read_prefs"
"""

# Dev enviroment
osmHost = "master.apis.dev.openstreetmap.org"
OAuthClientId = "uB0deHjh_W86CRUHfvWlisCC1ZIHkdLoKxz1qkuIrrM"
OAuthClientSecret = "xQE7suO-jmzmels19k-m8FQ8gHnkdWuLLVqfW6FIj44"
OAuthRedirectUri = "om://oauth2/osm/callback"
OAuthResponseType = "code"
OAuthScope = "read_prefs write_api write_notes"
test_login = "OrganicMapsTestUser"
test_password = "12345678"

def FetchSessionId():
    logging.info("Getting initial cookie to login ...")

    conn = http.client.HTTPSConnection(osmHost)
    conn.request("GET", "/login?cookie_test=true")
    res = conn.getresponse()

    cookies = SimpleCookie()
    cookies.load(res.headers['Set-Cookie'])

    authToken = FindAuthenticityToken(res.read().decode("utf-8"))

    if authToken is None:
        raise Exception("Can't find 'authenticity_token'")

    logging.info(f"Parsed authToken = {authToken}")
    logging.info("Got cookies: %s",res.headers['Set-Cookie'])

    return cookies, authToken

def LoginUserPassword(username, password, cookies, authToken):
    logging.info("Logging in with username and password ...")

    # Do POST with credentials
    payload = {
        "authenticity_token": authToken,
        "username": username,
        "password": password,
        "remember_me": "yes"
    }
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Cookie": encodeCookies(cookies),
        "Origin": f"https://{osmHost}",
        "Referer": f"https://{osmHost}/login?cookie_test=true"
    }

    conn2 = http.client.HTTPSConnection(osmHost)
    conn2.request("POST", "/login?cookie_test=true", urllib.parse.urlencode(payload), headers)
    res = conn2.getresponse()

    if res.status >= 400:
        print(res.status)
        print(res.headers)
        print(res.read().decode('utf-8'))
        raise Exception("Invalid login or password")

    logging.info("Logged in successfully")

    return cookies


def FetchRequestToken(cookies):
    logging.info("Loading OAuth2 auth page ...")

    query = urllib.parse.urlencode({
        "client_id": OAuthClientId,
        "redirect_uri": OAuthRedirectUri,
        "response_type": OAuthResponseType,
        "scope": OAuthScope
    })

    headers = {
        "Cookie": encodeCookies(cookies),
    }

    conn = http.client.HTTPSConnection(osmHost)
    conn.request("GET", f"/oauth2/authorize?{query}", headers=headers)

    res = conn.getresponse()

    if res.status >= 400:
        print(res.status)
        print(res.headers)
        print(res.read().decode('utf-8'))
        raise Exception("Can't load OAuth2 page")

    elif res.status == 302:
        logging.info("User already accepted OAuth2 request!")
        redirectUri = res.headers['Location']
        logging.info(f"Accepted OAuth2: {redirectUri}")
        oauthCode = FindOauthCode(redirectUri)

        if not oauthCode:
            raise Exception(f"Can't find 'code' in redirect URI: {redirectUri}")

        return oauthCode
    else:
        respBody = res.read().decode("utf-8")
        authToken = FindAuthenticityToken(respBody)
        if not authToken:
            print(res.status)
            print(res.headers)
            print(respBody)
            raise Exception("Invalid authToken '{authToken}'")
        logging.info(f"Parsed authToken = {authToken}")

        return SendAuthRequest(authToken, cookies)

def SendAuthRequest(authToken, cookies):
    logging.info("Accepting OAuth2 ...")
    # Do POST
    payload = {
        "authenticity_token": authToken,
        "client_id": OAuthClientId,
        "redirect_uri": OAuthRedirectUri,
        "response_type": OAuthResponseType,
        "scope": OAuthScope
    }

    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Cookie": encodeCookies(cookies),
        "Origin": f"https://{osmHost}",
    }

    conn2 = http.client.HTTPSConnection(osmHost)
    conn2.request("POST", f"/oauth2/authorize", urllib.parse.urlencode(payload), headers=headers)

    res = conn2.getresponse()
    if res.status != 302:
        print(res.status)
        print(res.headers)
        print(res.read().decode('utf-8'))
        raise Exception("Invalid response. Expected redirect")

    redirectUri = res.headers['Location']
    logging.info(f"Accepted OAuth2: {redirectUri}")
    oauthCode = FindOauthCode(redirectUri)

    if not oauthCode:
        raise Exception(f"Can't find 'code' in redirect URI: {redirectUri}")

    return oauthCode

def FinishAuthorization(code):
    payload = urllib.parse.urlencode({
        "grant_type": "authorization_code",
        "code": code,
        "redirect_uri": OAuthRedirectUri,
        "scope": OAuthScope,
        "client_id": OAuthClientId,
        "client_secret": OAuthClientSecret
    })

    headers = { 'content-type': "application/x-www-form-urlencoded" }

    conn = http.client.HTTPSConnection(osmHost)
    conn.request("POST", "/oauth2/token", payload, headers)

    res = conn.getresponse()
    if res.status >= 400:
        print(res.status)
        print(res.headers)
        print(res.read().decode('utf-8'))
        raise Exception("Error getting OAuth2 token")

    return res.read().decode("utf-8")


def FindAuthenticityToken(htmlCode):
    # search for <input name="authenticity_token" value="...">
    regex = re.compile(r"\<input\b.+?\bname=\"authenticity_token\".+?\bvalue=\"(.+?)\"")
    m = regex.search(htmlCode)
    if m:
        return m.group(1)

    return None

def FindOauthCode(redirectUri):
    query = urllib.parse.urlparse(redirectUri).query
    params = {k:v for (k,v) in urllib.parse.parse_qsl(query)}

    return params.get('code', None)


def encodeCookies(cookies):
    return "; ".join([f"{p.key}={p.value}" for p in cookies.values()])


if __name__ == '__main__':
    """
    This script goes through OAuth2 flow:
    * Login using username and password
    * Request OAuth2 authorization
    * Accepts OAuth2 authorization
    * Get OAuth2 token for API calls

    The same flow is implemented in editor/osm_auth.cpp
    Use this script for flow testing.
    """

    cookies, token = FetchSessionId()
    LoginUserPassword(test_login, test_password, cookies, token)
    code = FetchRequestToken(cookies)
    oauthToken = FinishAuthorization(code)
    logging.info(f"Completed OAuth2 flow. Got OAuth2 Token: '{oauthToken}'")
