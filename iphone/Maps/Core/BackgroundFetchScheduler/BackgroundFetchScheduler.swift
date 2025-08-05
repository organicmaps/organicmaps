@objc
protocol BackgroundFetchTask: NSObjectProtocol {
  func start()
}

@objcMembers
final class BackgroundFetchScheduler: NSObject {
  private let tasks: [BackgroundFetchTask]

  init(tasks: [BackgroundFetchTask]) {
    self.tasks = tasks
    super.init()
  }

  func run() {
    tasks.forEach { $0.start() }
  }

  @available(iOS 13.0, *)
  static func registerTasks() {
    ScheduledBackgroundEditsUploadingTask.register()
  }
}
