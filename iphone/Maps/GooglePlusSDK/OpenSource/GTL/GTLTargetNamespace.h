//
// Makes the value of GTL_TARGET_NAMESPACE a prefix for all GTL
// library class names
//

//
// To avoid global namespace issues, define GTL_TARGET_NAMESPACE to a short
// string in your target if you are using the GTL library in a shared-code
// environment like a plug-in.
//
// For example:   -DGTL_TARGET_NAMESPACE=MyPlugin
//

//
// com.google.GTLFramework v. 2.0 (29 classes) 2011-10-25 19:25:36 -0700
//

#if defined(__OBJC__) && defined(GTL_TARGET_NAMESPACE)

  #define _GTL_NS_SYMBOL_INNER(ns, symbol) ns ## _ ## symbol
  #define _GTL_NS_SYMBOL_MIDDLE(ns, symbol) _GTL_NS_SYMBOL_INNER(ns, symbol)
  #define _GTL_NS_SYMBOL(symbol) _GTL_NS_SYMBOL_MIDDLE(GTL_TARGET_NAMESPACE, symbol)

  #define _GTL_NS_STRING_INNER(ns) #ns
  #define _GTL_NS_STRING_MIDDLE(ns) _GTL_NS_STRING_INNER(ns)
  #define GTL_TARGET_NAMESPACE_STRING _GTL_NS_STRING_MIDDLE(GTL_TARGET_NAMESPACE)

  #define GTLBatchQuery              _GTL_NS_SYMBOL(GTLBatchQuery)
  #define GTLBatchResult             _GTL_NS_SYMBOL(GTLBatchResult)
  #define GTLCollectionObject        _GTL_NS_SYMBOL(GTLCollectionObject)
  #define GTLDateTime                _GTL_NS_SYMBOL(GTLDateTime)
  #define GTLErrorObject             _GTL_NS_SYMBOL(GTLErrorObject)
  #define GTLErrorObjectData         _GTL_NS_SYMBOL(GTLErrorObjectData)
  #define GTLJSONParser              _GTL_NS_SYMBOL(GTLJSONParser)
  #define GTLObject                  _GTL_NS_SYMBOL(GTLObject)
  #define GTLQuery                   _GTL_NS_SYMBOL(GTLQuery)
  #define GTLRuntimeCommon           _GTL_NS_SYMBOL(GTLRuntimeCommon)
  #define GTLService                 _GTL_NS_SYMBOL(GTLService)
  #define GTLServiceTicket           _GTL_NS_SYMBOL(GTLServiceTicket)
  #define GTLUploadParameters        _GTL_NS_SYMBOL(GTLUploadParameters)
  #define GTLUtilities               _GTL_NS_SYMBOL(GTLUtilities)
  #define GTMCachedURLResponse       _GTL_NS_SYMBOL(GTMCachedURLResponse)
  #define GTMCookieStorage           _GTL_NS_SYMBOL(GTMCookieStorage)
  #define GTMGatherInputStream       _GTL_NS_SYMBOL(GTMGatherInputStream)
  #define GTMHTTPFetcher             _GTL_NS_SYMBOL(GTMHTTPFetcher)
  #define GTMHTTPFetcherService      _GTL_NS_SYMBOL(GTMHTTPFetcherService)
  #define GTMHTTPFetchHistory        _GTL_NS_SYMBOL(GTMHTTPFetchHistory)
  #define GTMHTTPUploadFetcher       _GTL_NS_SYMBOL(GTMHTTPUploadFetcher)
  #define GTMMIMEDocument            _GTL_NS_SYMBOL(GTMMIMEDocument)
  #define GTMMIMEPart                _GTL_NS_SYMBOL(GTMMIMEPart)
  #define GTMOAuth2Authentication    _GTL_NS_SYMBOL(GTMOAuth2Authentication)
  #define GTMOAuth2AuthorizationArgs _GTL_NS_SYMBOL(GTMOAuth2AuthorizationArgs)
  #define GTMOAuth2SignIn            _GTL_NS_SYMBOL(GTMOAuth2SignIn)
  #define GTMOAuth2WindowController  _GTL_NS_SYMBOL(GTMOAuth2WindowController)
  #define GTMReadMonitorInputStream  _GTL_NS_SYMBOL(GTMReadMonitorInputStream)
  #define GTMURLCache                _GTL_NS_SYMBOL(GTMURLCache)

#endif
