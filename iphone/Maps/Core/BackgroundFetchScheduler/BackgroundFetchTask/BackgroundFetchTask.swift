@objc class BackgroundFetchTask: NSObject {
  var frameworkType: BackgroundFetchTaskFrameworkType { return .none }

  private var backgroundTaskIdentifier = UIBackgroundTaskIdentifier.invalid

  private var completionHandler: BackgroundFetchScheduler.FetchResultHandler?

  func start(completion: @escaping BackgroundFetchScheduler.FetchResultHandler) {
    completionHandler = completion
    backgroundTaskIdentifier = UIApplication.shared.beginBackgroundTask(withName:description,
                                                                        expirationHandler: {
                                                                          self.finish(.failed)
                                                                        })
    if backgroundTaskIdentifier != UIBackgroundTaskIdentifier.invalid { fire() }
  }

  fileprivate func fire() {
    finish(.failed)
  }

  fileprivate func finish(_ result: UIBackgroundFetchResult) {
    guard backgroundTaskIdentifier != UIBackgroundTaskIdentifier.invalid else { return }
    UIApplication.shared.endBackgroundTask(UIBackgroundTaskIdentifier(rawValue: backgroundTaskIdentifier.rawValue))
    backgroundTaskIdentifier = UIBackgroundTaskIdentifier.invalid
    completionHandler?(result)
  }
}

@objc(MWMBackgroundStatisticsUpload)
final class BackgroundStatisticsUpload: BackgroundFetchTask {
  override fileprivate func fire() {
    Alohalytics.forceUpload(self.finish)
  }

  override var description: String {
    return "Statistics upload"
  }
}

@objc(MWMBackgroundEditsUpload)
final class BackgroundEditsUpload: BackgroundFetchTask {
  override fileprivate func fire() {
    MWMEditorHelper.uploadEdits(self.finish)
  }

  override var description: String {
    return "Edits upload"
  }
}

@objc(MWMBackgroundUGCUpload)
final class BackgroundUGCUpload: BackgroundFetchTask {
  override var frameworkType: BackgroundFetchTaskFrameworkType { return .full }

  override fileprivate func fire() {
    FrameworkHelper.uploadUGC(self.finish)
  }

  override var description: String {
    return "UGC upload"
  }
}
