@objc(MWMBackgroundFetchScheduler)
final class BackgroundFetchScheduler: NSObject {
  typealias FetchResultHandler = (UIBackgroundFetchResult) -> Void

  enum Const {
    static let timeoutSafetyIndent: TimeInterval = 1
  }

  private let completionHandler: FetchResultHandler
  private let tasks: [BackgroundFetchTask]
  private let tasksGroup = DispatchGroup()
  private var fetchResult = UIBackgroundFetchResult.noData

  @objc init(tasks: [BackgroundFetchTask], completionHandler: @escaping FetchResultHandler) {
    self.tasks = tasks
    self.completionHandler = completionHandler
    super.init()
  }

  @objc func run() {
    DispatchQueue.main.async {
      self.fullfillFrameworkRequirements()
      let timeout = DispatchTime.now() + UIApplication.shared.backgroundTimeRemaining - Const.timeoutSafetyIndent
      self.performTasks(timeout: timeout)
    }
  }

  private func fullfillFrameworkRequirements() {
    minFrameworkTypeRequired().create()
  }

  private func minFrameworkTypeRequired() -> BackgroundFetchTaskFrameworkType {
    return tasks.reduce(.none) { max($0, $1.frameworkType) }
  }

  private func performTasks(timeout: DispatchTime) {
    DispatchQueue.global().async {
      self.setCompletionHandlers()
      self.startTasks()
      self.waitTasks(timeout: timeout)
    }
  }

  private func setCompletionHandlers() {
    let completionHandler: FetchResultHandler = { [weak self] result in
      self?.finishTask(result: result)
    }
    tasks.forEach { $0.completionHandler = completionHandler }
  }

  private func startTasks() {
    tasks.forEach {
      tasksGroup.enter()
      $0.start()
    }
  }

  private func finishTask(result: UIBackgroundFetchResult) {
    updateFetchResult(result)
    tasksGroup.leave()
  }

  private func waitTasks(timeout: DispatchTime) {
    let groupCompletion = tasksGroup.wait(timeout: timeout)
    DispatchQueue.main.async {
      switch groupCompletion {
      case .success: self.completionHandler(self.fetchResult)
      case .timedOut: self.completionHandler(.failed)
      }
    }
  }

  private func updateFetchResult(_ result: UIBackgroundFetchResult) {
    DispatchQueue.main.async {
      if self.resultPriority(self.fetchResult) < self.resultPriority(result) {
        self.fetchResult = result
      }
    }
  }

  private func resultPriority(_ result: UIBackgroundFetchResult) -> Int {
    switch result {
    case .newData: return 2
    case .noData: return 1
    case .failed: return 3
    }
  }
}
