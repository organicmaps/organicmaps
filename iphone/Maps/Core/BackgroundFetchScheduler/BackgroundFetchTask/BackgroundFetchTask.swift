@objc class BackgroundFetchTask: NSObject {
  var frameworkType: BackgroundFetchTaskFrameworkType { return .none }

  private var backgroundTaskIdentifier = UIBackgroundTaskInvalid

  private var completionHandler: BackgroundFetchScheduler.FetchResultHandler?

  func start(completion: @escaping BackgroundFetchScheduler.FetchResultHandler) {
    completionHandler = completion
    backgroundTaskIdentifier = UIApplication.shared.beginBackgroundTask(withName:description,
                                                                        expirationHandler: {
                                                                          self.finish(.failed)
                                                                        })
    if backgroundTaskIdentifier != UIBackgroundTaskInvalid { fire() }
  }

  fileprivate func fire() {
    finish(.failed)
  }

  fileprivate func finish(_ result: UIBackgroundFetchResult) {
    guard backgroundTaskIdentifier != UIBackgroundTaskInvalid else { return }
    UIApplication.shared.endBackgroundTask(backgroundTaskIdentifier)
    backgroundTaskIdentifier = UIBackgroundTaskInvalid
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
    MWMUGCHelper.uploadEdits(self.finish)
  }

  override var description: String {
    return "UGC upload"
  }
}

@objc(MWMBackgroundDownloadMapNotification)
final class BackgroundDownloadMapNotification: BackgroundFetchTask {
  override var frameworkType: BackgroundFetchTaskFrameworkType { return .full }

  override fileprivate func fire() {
    LocalNotificationManager.shared().showDownloadMapNotificationIfNeeded(self.finish)
  }

  override var description: String {
    return "Download map notification"
  }
}

