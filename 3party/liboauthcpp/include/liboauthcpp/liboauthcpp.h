#ifndef __LIBOAUTHCPP_LIBOAUTHCPP_H__
#define __LIBOAUTHCPP_LIBOAUTHCPP_H__

#include <string>
#include <list>
#include <map>
#include <stdexcept>
#include <ctime>

namespace OAuth {

namespace Http {
typedef enum _RequestType
{
    Invalid = 0,
    Head,
    Get,
    Post,
    Delete,
    Put
} RequestType;
} // namespace Http

typedef std::list<std::string> KeyValueList;
typedef std::multimap<std::string, std::string> KeyValuePairs;

typedef enum _LogLevel
{
    LogLevelNone = 0,
    LogLevelDebug = 1
} LogLevel;

/** Set the log level. Log messages are sent to stderr. Currently, and for the
 *  foreseeable future, logging only consists of debug messages to help track
 *  down protocol implementation issues.
 */
void SetLogLevel(LogLevel lvl);

/** Deprecated. Complete percent encoding of URLs. Equivalent to
 *  PercentEncode.
 */
std::string URLEncode(const std::string& decoded);

/** Percent encode a string value. This version is *thorough* about
 *  encoding: it encodes all reserved characters (even those safe in
 *  http URLs) and "other" characters not specified by the URI
 *  spec. If you're looking to encode http:// URLs, see the
 *  HttpEncode* functions.
 */
std::string PercentEncode(const std::string& decoded);

/** Percent encodes the path portion of an http URL (i.e. the /foo/bar
 *  in http://foo/bar?a=1&b=2). This encodes minimally, so reserved
 *  subdelimiters that have no meaning in the path are *not* encoded.
 */
std::string HttpEncodePath(const std::string& decoded);

/** Percent encodes a query string key in an http URL (i.e. 'a', 'b' in
 *  http://foo/bar?a=1&b=2). This encodes minimally, so reserved subdelimiters
 *  that have no meaning in the query string are *not* encoded.
 */
std::string HttpEncodeQueryKey(const std::string& decoded);

/** Percent encodes a query string value in an http URL (i.e. '1', '2' in
 *  http://foo/bar?a=1&b=2). This encodes minimally, so reserved subdelimiters
 *  that have no meaning in the query string are *not* encoded.
 */
std::string HttpEncodeQueryValue(const std::string& decoded);

/** Parses key value pairs into a map.
 *  \param encoded the encoded key value pairs, i.e. the url encoded parameters
 *  \returns a map of string keys to string values
 *  \throws ParseError if the encoded data cannot be decoded
 */
KeyValuePairs ParseKeyValuePairs(const std::string& encoded);

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string msg)
     : std::runtime_error(msg)
    {}
};

class MissingKeyError : public std::runtime_error {
public:
    MissingKeyError(const std::string msg)
     : std::runtime_error(msg)
    {}
};

/** A consumer of OAuth-protected services. It is the client to an
 *  OAuth service provider and is usually registered with the service
 *  provider, resulting in a consumer *key* and *secret* used to
 *  identify the consumer. The key is included in all requests and the
 *  secret is used to *sign* all requests.  Signed requests allow the
 *  consumer to securely perform operations, including kicking off
 *  three-legged authentication to enable performing operations on
 *  behalf of a user of the service provider.
 */
class Consumer {
public:
    Consumer(const std::string& key, const std::string& secret);

    const std::string& key() const { return mKey; }
    const std::string& secret() const { return mSecret; }

private:
    const std::string mKey;
    const std::string mSecret;
};

/** An OAuth credential used to request authorization or a protected
 *  resource.
 *
 *  Tokens in OAuth comprise a *key* and a *secret*. The key is
 *  included in requests to identify the token being used, but the
 *  secret is used only in the signature, to prove that the requester
 *  is who the server gave the token to.
 *
 *  When first negotiating the authorization, the consumer asks for a
 *  *request token* that the live user authorizes with the service
 *  provider. The consumer then exchanges the request token for an
 *  *access token* that can be used to access protected resources.
 */
class Token {
public:
    Token(const std::string& key, const std::string& secret);
    Token(const std::string& key, const std::string& secret, const std::string& pin);

    /** Construct a token, extracting the key and secret from a set of
     *  key-value pairs (e.g. those parsed from an request or access
     *  token request).
     */
    static Token extract(const KeyValuePairs& response);
    /** Construct a token, extracting the key and secret from a raw,
     *  encoded response.
     */
    static Token extract(const std::string& requestTokenResponse);

    const std::string& key() const { return mKey; }
    const std::string& secret() const { return mSecret; }

    const std::string& pin() const { return mPin; }
    void setPin(const std::string& pin_) { mPin = pin_; }

private:

    const std::string mKey;
    const std::string mSecret;
    std::string mPin;
};

class Client {
public:
    /** Perform static initialization. This will be called automatically, but
     *  you can call it explicitly to ensure thread safety. If you do not call
     *  this explicitly before using the Client class, the same nonce may be
     *  generated twice.
     */
    static void initialize();
    /** Alternative initialize method which lets you specify the seed and
     *  control the timestamp used in generating signatures. This only exists
     *  for testing purposes and should not be used in practice.
     */
    static void initialize(int nonce, time_t timestamp);

    /** Exposed for testing only.
     */
    static void __resetInitialize();

    /** Construct an OAuth Client using only a consumer key and
     *  secret. You can use this to start a three-legged
     *  authentication (to acquire an access token for a user) or for
     *  simple two-legged authentication (signing with empty access
     *  token info).
     *
     *  \param consumer Consumer information. The caller must ensure
     *         it remains valid during the lifetime of this object
     */
    Client(const Consumer* consumer);
    /** Construct an OAuth Client with consumer key and secret (yours)
     *  and access token key and secret (acquired and stored during
     *  three-legged authentication).
     *
     *  \param consumer Consumer information. The caller must ensure
     *         it remains valid during the lifetime of this object
     *  \param token Access token information. The caller must ensure
     *         it remains valid during the lifetime of this object
     */
    Client(const Consumer* consumer, const Token* token);

    ~Client();

    /** Build an OAuth HTTP header for the given request. This version provides
     *  only the field value.
     *
     *  \param eType the HTTP request type, e.g. GET or POST
     *  \param rawUrl the raw request URL (should include query parameters)
     *  \param rawData the raw HTTP request data (can be empty)
     *  \param includeOAuthVerifierPin if true, adds oauth_verifier parameter
     *  \returns a string containing the HTTP header
     */
    std::string getHttpHeader(const Http::RequestType eType,
                         const std::string& rawUrl,
                         const std::string& rawData = "",
                         const bool includeOAuthVerifierPin = false);
    /** Build an OAuth HTTP header for the given request. This version gives a
     *  fully formatted header, i.e. including the header field name.
     *
     *  \param eType the HTTP request type, e.g. GET or POST
     *  \param rawUrl the raw request URL (should include query parameters)
     *  \param rawData the raw HTTP request data (can be empty)
     *  \param includeOAuthVerifierPin if true, adds oauth_verifier parameter
     *  \returns a string containing the HTTP header
     */
    std::string getFormattedHttpHeader(const Http::RequestType eType,
                         const std::string& rawUrl,
                         const std::string& rawData = "",
                         const bool includeOAuthVerifierPin = false);
    /** Build an OAuth HTTP header for the given request.
     *
     *  \param eType the HTTP request type, e.g. GET or POST
     *  \param rawUrl the raw request URL (should include query parameters)
     *  \param rawData the raw HTTP request data (can be empty)
     *  \param includeOAuthVerifierPin if true, adds oauth_verifier parameter
     *  \returns a string containing the query string, including the query
     *         parameters in the rawUrl
     */
    std::string getURLQueryString(const Http::RequestType eType,
                         const std::string& rawUrl,
                         const std::string& rawData = "",
                         const bool includeOAuthVerifierPin = false);
private:
    /** Disable default constructur -- must provide consumer
     * information.
     */
    Client();

    static bool initialized;
    static int testingNonce;
    static time_t testingTimestamp;

    /* OAuth data */
    const Consumer* mConsumer;
    const Token* mToken;
    std::string m_nonce;
    std::string m_timeStamp;

    /* OAuth related utility methods */
    bool buildOAuthTokenKeyValuePairs( const bool includeOAuthVerifierPin, /* in */
                                       const std::string& rawData, /* in */
                                       const std::string& oauthSignature, /* in */
                                       KeyValuePairs& keyValueMap /* out */,
                                       const bool urlEncodeValues /* in */,
                                       const bool generateTimestamp /* in */);

    bool getStringFromOAuthKeyValuePairs( const KeyValuePairs& rawParamMap, /* in */
                                          std::string& rawParams, /* out */
                                          const std::string& paramsSeperator /* in */ );

    typedef enum _ParameterStringType {
        QueryStringString,
        AuthorizationHeaderString
    } ParameterStringType;
    // Utility for building OAuth HTTP header or query string. The string type
    // controls the separator and also filters parameters: for query strings,
    // all parameters are included. For HTTP headers, only auth parameters are
    // included.
    std::string buildOAuthParameterString(
        ParameterStringType string_type,
        const Http::RequestType eType,
        const std::string& rawUrl,
        const std::string& rawData,
        const bool includeOAuthVerifierPin);

    bool getSignature( const Http::RequestType eType, /* in */
                       const std::string& rawUrl, /* in */
                       const KeyValuePairs& rawKeyValuePairs, /* in */
                       std::string& oAuthSignature /* out */ );

    void generateNonceTimeStamp();
};

} // namespace OAuth

#endif // __LIBOAUTHCPP_LIBOAUTHCPP_H__
