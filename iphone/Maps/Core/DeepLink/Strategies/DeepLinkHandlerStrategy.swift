protocol IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL { get }

  func execute()
  func sendStatisticsOnSuccess(type: String)
  func sendStatisticsOnFail(type: String)
}

extension IDeepLinkHandlerStrategy {
  fileprivate func makeParams(type: String) -> [String: String] {
    let params = [kStatType : type, kStatProvider: deeplinkURL.provider.statName]
    guard let components = URLComponents(string: deeplinkURL.url.absoluteString) else {
      return params
    }
    
    var result = [String: String]()
    components.queryItems?
      .filter({ $0.name.starts(with: "utm_") || $0.name == "affiliate_id" })
      .forEach({ result[$0.name] = $0.value ?? "" })
    
    // Internal params are more perffered than url params.
    for param in params {
      result.updateValue(param.value, forKey: param.key)
    }
    
    return result
  }
  
  func sendStatisticsOnSuccess(type:String) {
    Statistics.logEvent(kStatDeeplinkCall, withParameters: makeParams(type:type))
  }

  func sendStatisticsOnFail(type:String) {
    Statistics.logEvent(kStatDeeplinkCallMissed, withParameters: makeParams(type:type))
  }
}
