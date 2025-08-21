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
    LOG(.info, "Run extended background task for edits uploading...")
    MWMEditorHelper.uploadEdits(self.finish)
  }

  override var description: String {
    return "Edits upload"
  }
}
