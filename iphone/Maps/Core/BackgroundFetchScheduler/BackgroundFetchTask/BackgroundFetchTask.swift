@objc class BackgroundFetchTask: NSObject {
  var frameworkType: BackgroundFetchTaskFrameworkType { .none }

  private var backgroundTaskIdentifier = UIBackgroundTaskIdentifier.invalid

  private var completionHandler: BackgroundFetchScheduler.FetchResultHandler?

  func start(completion: @escaping BackgroundFetchScheduler.FetchResultHandler) {
    completionHandler = completion
    backgroundTaskIdentifier = UIApplication.shared.beginBackgroundTask(withName: description,
                                                                        expirationHandler: {
                                                                          self.finish(.failed)
                                                                        })
    if backgroundTaskIdentifier != UIBackgroundTaskIdentifier.invalid {
      fire()
    }
  }

  fileprivate func fire() {
    finish(.failed)
  }

  fileprivate func finish(_ result: UIBackgroundFetchResult) {
    // The expiration handler is delivered on the main actor and the upload completion on the core's
    // network thread, while the state below is unsynchronized. Serialize both on the main queue so
    // that the background task is ended and reported exactly once.
    if !Thread.isMainThread {
      DispatchQueue.main.async { self.finish(result) }
      return
    }

    guard backgroundTaskIdentifier != UIBackgroundTaskIdentifier.invalid else { return }
    UIApplication.shared.endBackgroundTask(UIBackgroundTaskIdentifier(rawValue: backgroundTaskIdentifier.rawValue))
    backgroundTaskIdentifier = UIBackgroundTaskIdentifier.invalid
    completionHandler?(result)
  }
}

@objc(MWMBackgroundEditsUpload)
final class BackgroundEditsUpload: BackgroundFetchTask {
  override fileprivate func fire() {
    MWMEditorHelper.uploadEdits(finish)
  }

  override var description: String {
    "Edits upload"
  }
}
