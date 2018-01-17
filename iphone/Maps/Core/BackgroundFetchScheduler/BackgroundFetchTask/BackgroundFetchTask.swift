@objc class BackgroundFetchTask: NSObject {
  var queue: DispatchQueue { return .global() }
  var frameworkType: BackgroundFetchTaskFrameworkType { return .none }

  private var backgroundTaskIdentifier = UIBackgroundTaskInvalid

  lazy var block = { self.finish(.failed) }
  var completionHandler: BackgroundFetchScheduler.FetchResultHandler!

  func start() {
    backgroundTaskIdentifier = UIApplication.shared.beginBackgroundTask(expirationHandler: {
      self.finish(.failed)
    })
    if backgroundTaskIdentifier != UIBackgroundTaskInvalid {
      queue.async(execute: block)
    }
  }

  func finish(_ result: UIBackgroundFetchResult) {
    guard backgroundTaskIdentifier != UIBackgroundTaskInvalid else { return }
    UIApplication.shared.endBackgroundTask(backgroundTaskIdentifier)
    backgroundTaskIdentifier = UIBackgroundTaskInvalid
    completionHandler(result)
  }
}
