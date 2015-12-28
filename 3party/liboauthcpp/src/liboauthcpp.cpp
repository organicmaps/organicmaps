#include <liboauthcpp/liboauthcpp.h>
#include "HMAC_SHA1.h"
#include "base64.h"
#include "urlencode.h"
#include <cstdlib>
#include <vector>
#include <cassert>

namespace OAuth {

namespace Defaults
{
    /* Constants */
    const int BUFFSIZE = 1024;
    const int BUFFSIZE_LARGE = 1024;
    const std::string CONSUMERKEY_KEY = "oauth_consumer_key";
    const std::string CALLBACK_KEY = "oauth_callback";
    const std::string VERSION_KEY = "oauth_version";
    const std::string SIGNATUREMETHOD_KEY = "oauth_signature_method";
    const std::string SIGNATURE_KEY = "oauth_signature";
    const std::string TIMESTAMP_KEY = "oauth_timestamp";
    const std::string NONCE_KEY = "oauth_nonce";
    const std::string TOKEN_KEY = "oauth_token";
    const std::string TOKENSECRET_KEY = "oauth_token_secret";
    const std::string VERIFIER_KEY = "oauth_verifier";

    const std::string AUTHHEADER_FIELD = "Authorization: ";
    const std::string AUTHHEADER_PREFIX = "OAuth ";
};

/** std::string -> std::string conversion function */
typedef std::string(*StringConvertFunction)(const std::string&);

LogLevel gLogLevel = LogLevelNone;

void SetLogLevel(LogLevel lvl) {
    gLogLevel = lvl;
}
#define LOG(lvl, msg)                                           \
    do  {                                                       \
        if (lvl <= gLogLevel) std::cerr << "OAUTH: " << msg << std::endl; \
    } while(0)

std::string PercentEncode(const std::string& decoded) {
    return urlencode(decoded, URLEncode_Everything);
}

std::string URLEncode(const std::string& decoded) {
    return PercentEncode(decoded);
}

std::string HttpEncodePath(const std::string& decoded) {
    return urlencode(decoded, URLEncode_Path);
}

std::string HttpEncodeQueryKey(const std::string& decoded) {
    return urlencode(decoded, URLEncode_QueryKey);
}

std::string HttpEncodeQueryValue(const std::string& decoded) {
    return urlencode(decoded, URLEncode_QueryValue);
}

namespace {
std::string PassThrough(const std::string& decoded) {
    return decoded;
}

std::string RequestTypeString(const Http::RequestType rt) {
    switch(rt) {
      case Http::Invalid: return "Invalid Request Type"; break;
      case Http::Head: return "HEAD"; break;
      case Http::Get: return "GET"; break;
      case Http::Post: return "POST"; break;
      case Http::Delete: return "DELETE"; break;
      case Http::Put: return "PUT"; break;
      default: return "Unknown Request Type"; break;
    }
    return "";
}
}

// Parse a single key-value pair
static std::pair<std::string, std::string> ParseKeyValuePair(const std::string& encoded) {
    std::size_t eq_pos = encoded.find("=");
    if (eq_pos == std::string::npos)
        throw ParseError("Failed to find '=' in key-value pair.");
    return std::pair<std::string, std::string>(
        encoded.substr(0, eq_pos),
        encoded.substr(eq_pos+1)
    );
}

KeyValuePairs ParseKeyValuePairs(const std::string& encoded) {
    KeyValuePairs result;

    if (encoded.length() == 0) return result;

    // Split by &
    std::size_t last_amp = 0;
    // We can bail when the last one "found" was the end of the string
    while(true) {
        std::size_t next_amp = encoded.find('&', last_amp+1);
        std::string keyval =
            (next_amp == std::string::npos) ?
            encoded.substr(last_amp) :
            encoded.substr(last_amp, next_amp-last_amp);
        result.insert(ParseKeyValuePair(keyval));
        // Track spot after the & so the first iteration works without dealing
        // with -1 index
        last_amp = next_amp+1;

        // Exit condition
        if (next_amp == std::string::npos) break;
    }
    return result;
}

// Helper for parameters in key-value pair lists that should only appear
// once. Either replaces an existing entry or adds a new entry.
static void ReplaceOrInsertKeyValuePair(KeyValuePairs& kvp, const std::string& key, const std::string& value) {
    assert(kvp.count(key) <= 1);
    KeyValuePairs::iterator it = kvp.find(key);
    if (it != kvp.end())
        it->second = value;
    else
        kvp.insert(KeyValuePairs::value_type(key, value));
}

Consumer::Consumer(const std::string& key, const std::string& secret)
 : mKey(key), mSecret(secret)
{
}



Token::Token(const std::string& key, const std::string& secret)
 : mKey(key), mSecret(secret)
{
}

Token::Token(const std::string& key, const std::string& secret, const std::string& pin)
 : mKey(key), mSecret(secret), mPin(pin)
{
}

Token Token::extract(const std::string& response) {
    return Token::extract(ParseKeyValuePairs(response));
}

Token Token::extract(const KeyValuePairs& response) {
    std::string token_key, token_secret;

    KeyValuePairs::const_iterator it = response.find(Defaults::TOKEN_KEY);
    if (it == response.end())
        throw MissingKeyError("Couldn't find oauth_token in response");
    token_key = it->second;

    it = response.find(Defaults::TOKENSECRET_KEY);
    if (it == response.end())
        throw MissingKeyError("Couldn't find oauth_token_secret in response");
    token_secret = it->second;

    return Token(token_key, token_secret);
}


bool Client::initialized = false;
int Client::testingNonce = 0;
time_t Client::testingTimestamp = 0;

void Client::initialize() {
    if(!initialized) {
        srand( time( NULL ) );
        initialized = true;
    }
}

void Client::initialize(int nonce, time_t timestamp) {
    if(!initialized) {
        testingNonce = nonce;
        testingTimestamp = timestamp;
        initialized = true;
    }
}

void Client::__resetInitialize() {
    testingNonce = 0;
    testingTimestamp = 0;
    initialized = false;
}

Client::Client(const Consumer* consumer)
 : mConsumer(consumer),
   mToken(NULL)
{
}

Client::Client(const Consumer* consumer, const Token* token)
 : mConsumer(consumer),
   mToken(token)
{
}


Client::~Client()
{
}



/*++
* @method: Client::generateNonceTimeStamp
*
* @description: this method generates nonce and timestamp for OAuth header
*
* @input: none
*
* @output: none
*
* @remarks: internal method
*
*--*/
void Client::generateNonceTimeStamp()
{
    // Make sure the random seed has been initialized
    Client::initialize();

    char szTime[Defaults::BUFFSIZE];
    char szRand[Defaults::BUFFSIZE];
    memset( szTime, 0, Defaults::BUFFSIZE );
    memset( szRand, 0, Defaults::BUFFSIZE );

    // Any non-zero timestamp triggers testing mode with fixed values. Fixing
    // both values makes life easier because generating a signature is
    // idempotent -- otherwise using macros can cause double evaluation and
    // incorrect results because of repeated calls to rand().
    sprintf( szRand, "%x", ((testingTimestamp != 0) ? testingNonce : rand()) );
    sprintf( szTime, "%ld", ((testingTimestamp != 0) ? testingTimestamp : time( NULL )) );

    m_nonce.assign( szTime );
    m_nonce.append( szRand );

    m_timeStamp.assign( szTime );
}

/*++
* @method: Client::buildOAuthTokenKeyValuePairs
*
* @description: this method prepares key-value pairs required for OAuth header
*               and signature generation.
*
* @input: includeOAuthVerifierPin - flag to indicate whether oauth_verifer key-value
*                                   pair needs to be included. oauth_verifer is only
*                                   used during exchanging request token with access token.
*         rawData - url encoded data. this is used during signature generation.
*         oauthSignature - base64 and url encoded OAuth signature.
*         generateTimestamp - If true, then generate new timestamp for nonce.
*
* @input: urlEncodeValues - if true, URLEncode the values inserted into the
*         output keyValueMap
* @output: keyValueMap - map in which key-value pairs are populated
*
* @remarks: internal method
*
*--*/
bool Client::buildOAuthTokenKeyValuePairs( const bool includeOAuthVerifierPin,
                                          const std::string& rawData,
                                          const std::string& oauthSignature,
                                          KeyValuePairs& keyValueMap,
                                          const bool urlEncodeValues,
                                          const bool generateTimestamp )
{
    // Encodes value part of key-value pairs depending on type of output (query
    // string vs. HTTP headers.
    StringConvertFunction value_encoder = (urlEncodeValues ? HttpEncodeQueryValue : PassThrough);

    /* Generate nonce and timestamp if required */
    if( generateTimestamp )
    {
        generateNonceTimeStamp();
    }

    /* Consumer key and its value */
    ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::CONSUMERKEY_KEY, value_encoder(mConsumer->key()));

    /* Nonce key and its value */
    ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::NONCE_KEY, value_encoder(m_nonce));

    /* Signature if supplied */
    if( oauthSignature.length() )
    {
        // Signature is exempt from encoding. The procedure for
        // computing it already percent-encodes it as required by the
        // spec for both query string and Auth header
        // methods. Therefore, it's pass-through in both cases.
        ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::SIGNATURE_KEY, oauthSignature);
    }

    /* Signature method, only HMAC-SHA1 as of now */
    ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::SIGNATUREMETHOD_KEY, std::string( "HMAC-SHA1" ));

    /* Timestamp */
    ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::TIMESTAMP_KEY, value_encoder(m_timeStamp));

    /* Token */
    if( mToken && mToken->key().length() )
    {
        ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::TOKEN_KEY, value_encoder(mToken->key()));
    }

    /* Verifier */
    if( includeOAuthVerifierPin && mToken && mToken->pin().length() )
    {
        ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::VERIFIER_KEY, value_encoder(mToken->pin()));
    }

    /* Version */
    ReplaceOrInsertKeyValuePair(keyValueMap, Defaults::VERSION_KEY, std::string( "1.0" ));

    /* Data if it's present */
    if( rawData.length() )
    {
        /* Data should already be urlencoded once */
        std::string dummyStrKey;
        std::string dummyStrValue;
        size_t nPos = rawData.find_first_of( "=" );
        if( std::string::npos != nPos )
        {
            dummyStrKey = rawData.substr( 0, nPos );
            dummyStrValue = rawData.substr( nPos + 1 );
            ReplaceOrInsertKeyValuePair(keyValueMap, dummyStrKey, dummyStrValue);
        }
    }

    return ( keyValueMap.size() ) ? true : false;
}

/*++
* @method: Client::getSignature
*
* @description: this method calculates HMAC-SHA1 signature of OAuth header
*
* @input: eType - HTTP request type
*         rawUrl - raw url of the HTTP request
*         rawKeyValuePairs - key-value pairs containing OAuth headers and HTTP data
*
* @output: oAuthSignature - base64 and url encoded signature
*
* @remarks: internal method
*
*--*/
bool Client::getSignature( const Http::RequestType eType,
                          const std::string& rawUrl,
                          const KeyValuePairs& rawKeyValuePairs,
                          std::string& oAuthSignature )
{
    std::string rawParams;
    std::string paramsSeperator;
    std::string sigBase;

    /* Initially empty signature */
    oAuthSignature.assign( "" );

    /* Build a string using key-value pairs */
    paramsSeperator = "&";
    getStringFromOAuthKeyValuePairs( rawKeyValuePairs, rawParams, paramsSeperator );
    LOG(LogLevelDebug, "Normalized parameters: " << rawParams);

    /* Start constructing base signature string. Refer http://dev.twitter.com/auth#intro */
    switch( eType )
    {
      case Http::Head:
        {
            sigBase.assign( "HEAD&" );
        }
        break;

      case Http::Get:
        {
            sigBase.assign( "GET&" );
        }
        break;

      case Http::Post:
        {
            sigBase.assign( "POST&" );
        }
        break;

      case Http::Delete:
        {
            sigBase.assign( "DELETE&" );
        }
        break;

      case Http::Put:
        {
            sigBase.assign( "PUT&" );
        }
        break;

    default:
        {
            return false;
        }
        break;
    }
    sigBase.append( PercentEncode( rawUrl ) );
    sigBase.append( "&" );
    sigBase.append( PercentEncode( rawParams ) );
    LOG(LogLevelDebug, "Signature base string: " << sigBase);

    /* Now, hash the signature base string using HMAC_SHA1 class */
    CHMAC_SHA1 objHMACSHA1;
    std::string secretSigningKey;
    unsigned char strDigest[Defaults::BUFFSIZE_LARGE];

    memset( strDigest, 0, Defaults::BUFFSIZE_LARGE );

    /* Signing key is composed of consumer_secret&token_secret */
    secretSigningKey.assign( PercentEncode(mConsumer->secret()) );
    secretSigningKey.append( "&" );
    if( mToken && mToken->secret().length() )
    {
        secretSigningKey.append( PercentEncode(mToken->secret()) );
    }

    objHMACSHA1.HMAC_SHA1( (unsigned char*)sigBase.c_str(),
                           sigBase.length(),
                           (unsigned char*)secretSigningKey.c_str(),
                           secretSigningKey.length(),
                           strDigest );

    /* Do a base64 encode of signature */
    std::string base64Str = base64_encode( strDigest, 20 /* SHA 1 digest is 160 bits */ );
    LOG(LogLevelDebug, "Signature: " << base64Str);

    /* Do an url encode */
    oAuthSignature = PercentEncode( base64Str );
    LOG(LogLevelDebug, "Percent-encoded Signature: " << oAuthSignature);

    return ( oAuthSignature.length() ) ? true : false;
}

std::string Client::getHttpHeader(const Http::RequestType eType,
    const std::string& rawUrl,
    const std::string& rawData,
    const bool includeOAuthVerifierPin)
{
    return Defaults::AUTHHEADER_PREFIX + buildOAuthParameterString(AuthorizationHeaderString, eType, rawUrl, rawData, includeOAuthVerifierPin);
}

std::string Client::getFormattedHttpHeader(const Http::RequestType eType,
    const std::string& rawUrl,
    const std::string& rawData,
    const bool includeOAuthVerifierPin)
{
    return Defaults::AUTHHEADER_FIELD + Defaults::AUTHHEADER_PREFIX + buildOAuthParameterString(AuthorizationHeaderString, eType, rawUrl, rawData, includeOAuthVerifierPin);
}

std::string Client::getURLQueryString(const Http::RequestType eType,
    const std::string& rawUrl,
    const std::string& rawData,
    const bool includeOAuthVerifierPin)
{
    return buildOAuthParameterString(QueryStringString, eType, rawUrl, rawData, includeOAuthVerifierPin);
}

std::string Client::buildOAuthParameterString(
    ParameterStringType string_type,
    const Http::RequestType eType,
    const std::string& rawUrl,
    const std::string& rawData,
    const bool includeOAuthVerifierPin)
{
    KeyValuePairs rawKeyValuePairs;
    std::string rawParams;
    std::string oauthSignature;
    std::string paramsSeperator;
    std::string pureUrl( rawUrl );

    LOG(LogLevelDebug, "Signing request " << RequestTypeString(eType) << " " << rawUrl << " " << rawData);

    std::string separator;
    bool do_urlencode;
    if (string_type == AuthorizationHeaderString) {
        separator = ",";
        do_urlencode = false;
    }
    else { // QueryStringString
        separator = "&";
        do_urlencode = true;
    }

    /* Clear header string initially */
    rawKeyValuePairs.clear();

    /* If URL itself contains ?key=value, then extract and put them in map */
    size_t nPos = rawUrl.find_first_of( "?" );
    if( std::string::npos != nPos )
    {
        /* Get only URL */
        pureUrl = rawUrl.substr( 0, nPos );

        /* Get only key=value data part */
        std::string dataPart = rawUrl.substr( nPos + 1 );
        rawKeyValuePairs = ParseKeyValuePairs(dataPart);
    }

    // NOTE: We always request URL encoding on the first pass so that the
    // signature generation works properly. This *relies* on
    // buildOAuthTokenKeyValuePairs overwriting values when we do the second
    // pass to get the values in the form we actually want. The signature and
    // rawdata are the only things that change, but the signature is only used
    // in the second pass and the rawdata is already encoded, regardless of
    // request type.

    /* Build key-value pairs needed for OAuth request token, without signature */
    buildOAuthTokenKeyValuePairs( includeOAuthVerifierPin, rawData, std::string( "" ), rawKeyValuePairs, true, true );

    /* Get url encoded base64 signature using request type, url and parameters */
    getSignature( eType, pureUrl, rawKeyValuePairs, oauthSignature );

    /* Now, again build key-value pairs with signature this time */
    buildOAuthTokenKeyValuePairs( includeOAuthVerifierPin, std::string( "" ), oauthSignature, rawKeyValuePairs, do_urlencode, false );

    /* Get OAuth header in string format. If we're getting the Authorization
     * header, we need to filter out other parameters.
     */
    if (string_type == AuthorizationHeaderString) {
        KeyValuePairs oauthKeyValuePairs;
        std::vector<std::string> oauth_keys;
        oauth_keys.push_back(Defaults::CONSUMERKEY_KEY);
        oauth_keys.push_back(Defaults::NONCE_KEY);
        oauth_keys.push_back(Defaults::SIGNATURE_KEY);
        oauth_keys.push_back(Defaults::SIGNATUREMETHOD_KEY);
        oauth_keys.push_back(Defaults::TIMESTAMP_KEY);
        oauth_keys.push_back(Defaults::TOKEN_KEY);
        oauth_keys.push_back(Defaults::VERIFIER_KEY);
        oauth_keys.push_back(Defaults::VERSION_KEY);

        for(size_t i = 0; i < oauth_keys.size(); i++) {
            assert(rawKeyValuePairs.count(oauth_keys[i]) <= 1);
            KeyValuePairs::iterator oauth_key_it = rawKeyValuePairs.find(oauth_keys[i]);
            if (oauth_key_it != rawKeyValuePairs.end())
                ReplaceOrInsertKeyValuePair(oauthKeyValuePairs, oauth_keys[i], oauth_key_it->second);
        }
        getStringFromOAuthKeyValuePairs( oauthKeyValuePairs, rawParams, separator );
    }
    else if (string_type == QueryStringString) {
        getStringFromOAuthKeyValuePairs( rawKeyValuePairs, rawParams, separator );
    }

    /* Build authorization header */
    return rawParams;
}

/*++
* @method: Client::getStringFromOAuthKeyValuePairs
*
* @description: this method builds a sorted string from key-value pairs
*
* @input: rawParamMap - key-value pairs map
*         paramsSeperator - sepearator, either & or ,
*
* @output: rawParams - sorted string of OAuth parameters
*
* @remarks: internal method
*
*--*/
bool Client::getStringFromOAuthKeyValuePairs( const KeyValuePairs& rawParamMap,
                                             std::string& rawParams,
                                             const std::string& paramsSeperator )
{
    rawParams.assign( "" );
    if( rawParamMap.size() )
    {
        KeyValueList keyValueList;
        std::string dummyStr;

        /* Push key-value pairs to a list of strings */
        keyValueList.clear();
        KeyValuePairs::const_iterator itMap = rawParamMap.begin();
        for( ; itMap != rawParamMap.end(); itMap++ )
        {
            dummyStr.assign( itMap->first );
            dummyStr.append( "=" );
            if( paramsSeperator == "," )
            {
                dummyStr.append( "\"" );
            }
            dummyStr.append( itMap->second );
            if( paramsSeperator == "," )
            {
                dummyStr.append( "\"" );
            }
            keyValueList.push_back( dummyStr );
        }

        /* Sort key-value pairs based on key name */
        keyValueList.sort();

        /* Now, form a string */
        dummyStr.assign( "" );
        KeyValueList::iterator itKeyValue = keyValueList.begin();
        for( ; itKeyValue != keyValueList.end(); itKeyValue++ )
        {
            if( dummyStr.length() )
            {
                dummyStr.append( paramsSeperator );
            }
            dummyStr.append( itKeyValue->c_str() );
        }
        rawParams.assign( dummyStr );
    }
    return ( rawParams.length() ) ? true : false;
}


} // namespace OAuth
