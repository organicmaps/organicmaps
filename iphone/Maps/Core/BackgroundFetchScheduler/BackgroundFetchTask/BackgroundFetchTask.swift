@objc class BackgroundFetchTask: NSObject {
  var frameworkType: BackgroundFetchTaskFrameworkType { return .none }

  private var backgroundTaskIdentifier = UIBackgroundTaskIdentifier.invalid

  private var completionHandler: BackgroundFetchScheduler.FetchResultHandler?

  func start(completion: @escaping BackgroundFetchScheduler.FetchResultHandler) {
    completionHandler = completion
    backgroundTaskIdentifier = UIApplication.shared.beginBackgroundTask(withName:description,
                                                                        expirationHandler: {
      LOG(.info, "Background task \(self.description) expired")
                                                                          self.finish(.failed)
                                                                        })
    if backgroundTaskIdentifier != UIBackgroundTaskIdentifier.invalid { fire() }
  }

  fileprivate func fire() {
    LOG(.info, "Run extended background task for \(description)...")
    finish(.failed)
  }

  fileprivate func finish(_ result: UIBackgroundFetchResult) {
    if backgroundTaskIdentifier != UIBackgroundTaskIdentifier.invalid {
      UIApplication.shared.endBackgroundTask(backgroundTaskIdentifier)
      backgroundTaskIdentifier = UIBackgroundTaskIdentifier.invalid
    }
    completionHandler?(result)
  }
}

@objc(MWMBackgroundEditsUpload)
final class BackgroundEditsUpload: BackgroundFetchTask {
  override fileprivate func fire() {
    // We cannot rely on the `UIApplication.shared.backgroundTimeRemaining` because it is valid only after didEnterBackground is finished.
    let timeout = TimeInterval(25.0)
    MWMEditorHelper.uploadEdits(withTimeout: timeout,
                                completionHandler: self.finish)
  }

  override var description: String {
    return "Edits upload"
  }
}
