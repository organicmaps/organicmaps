@objc class BackgroundFetchTask: NSObject {
  var queue: DispatchQueue { return .global() }
  var frameworkType: BackgroundFetchTaskFrameworkType { return .none }

  private var backgroundTaskIdentifier = UIBackgroundTaskInvalid

  lazy var block = { self.finish(.failed) }
  var completionHandler: BackgroundFetchScheduler.FetchResultHandler!

  func start() {
    DispatchQueue.main.async {
      self.backgroundTaskIdentifier = UIApplication.shared.beginBackgroundTask(expirationHandler: {
        self.finish(.failed)
      })
      if self.backgroundTaskIdentifier != UIBackgroundTaskInvalid {
        self.queue.async(execute: self.block)
      }
    }
  }

  func finish(_ result: UIBackgroundFetchResult) {
    guard backgroundTaskIdentifier != UIBackgroundTaskInvalid else { return }
    DispatchQueue.main.async {
      UIApplication.shared.endBackgroundTask(self.backgroundTaskIdentifier)
      self.backgroundTaskIdentifier = UIBackgroundTaskInvalid
      self.queue.async {
        self.completionHandler(result)
      }
    }
  }
}
