protocol IDeepLinkHandlerStrategy {
  var deeplinkURL: DeepLinkURL { get }

  func execute()
  func sendStatisticsOnSuccess(type: String)
  func sendStatisticsOnFail(type: String)
}

extension IDeepLinkHandlerStrategy {
  func sendStatisticsOnSuccess(type:String) {
    Statistics.logEvent(kStatDeeplinkCall, withParameters: [kStatType : type,
                                                            kStatProvider: deeplinkURL.provider.statName])
  }

  func sendStatisticsOnFail(type:String) {
    Statistics.logEvent(kStatDeeplinkCallMissed, withParameters: [kStatType : type,
                                                                  kStatProvider: deeplinkURL.provider.statName])
  }
}
