@objc(MWMBackgroundFetchScheduler)
final class BackgroundFetchScheduler: NSObject {
  typealias FetchResultHandler = (UIBackgroundFetchResult) -> Void

  private let completionHandler: FetchResultHandler
  private let tasks: [BackgroundFetchTask]
  private var tasksLeft: Int
  private var bestResultSoFar = UIBackgroundFetchResult.noData

  @objc init(tasks: [BackgroundFetchTask], completionHandler: @escaping FetchResultHandler) {
    self.tasks = tasks
    self.completionHandler = completionHandler
    tasksLeft = tasks.count
    super.init()
  }

  @objc func run() {
    fullfillFrameworkRequirements()

    let completionHandler: FetchResultHandler = { [weak self] result in
      self?.finishTask(result: result)
    }

    tasks.forEach { $0.start(completion: completionHandler) }
  }

  private func fullfillFrameworkRequirements() {
    minFrameworkTypeRequired().create()
  }

  private func minFrameworkTypeRequired() -> BackgroundFetchTaskFrameworkType {
    return tasks.reduce(.none) { max($0, $1.frameworkType) }
  }

  private func finishTask(result: UIBackgroundFetchResult) {
    updateFetchResult(result)
    tasksLeft -= 1
    if tasksLeft <= 0 {
      completionHandler(bestResultSoFar)
    }
  }

  private func updateFetchResult(_ result: UIBackgroundFetchResult) {
    if resultPriority(bestResultSoFar) < resultPriority(result) {
      bestResultSoFar = result
    }
  }

  private func resultPriority(_ result: UIBackgroundFetchResult) -> Int {
    switch result {
      case .newData: return 3
      case .noData: return 1
      case .failed: return 2
      @unknown default: fatalError("Unexpected case in UIBackgroundFetchResult switch")
    }
  }
}
