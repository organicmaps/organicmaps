enum DirectoryMonitorState: CaseIterable, Equatable {
  case started
  case stopped
}

protocol DirectoryMonitor: AnyObject {
  var state: DirectoryMonitorState { get }

  func start(completion: ((Result<URL, Error>) -> Void)?)
  func stop()
}

protocol LocalDirectoryMonitor: DirectoryMonitor {
  var directory: URL { get }
  var delegate: LocalDirectoryMonitorDelegate? { get set }
}

protocol LocalDirectoryMonitorDelegate : AnyObject {
  func didFinishGathering(_ contents: LocalContents)
  func didUpdate(_ contents: LocalContents, _ update: LocalContentsUpdate)
  func didReceiveLocalMonitorError(_ error: Error)
}

final class FileSystemDispatchSourceMonitor: LocalDirectoryMonitor {

  typealias Delegate = LocalDirectoryMonitorDelegate

  fileprivate enum DispatchSourceDebounceState {
    case stopped
    case started(source: DispatchSourceFileSystemObject)
    case debounce(source: DispatchSourceFileSystemObject, timer: Timer)
  }

  private let bookmarksManager = BookmarksManager.shared()
  private var didFinishGatheringIsCalled = false

  // MARK: - Public properties
  let directory: URL
  private let queue: OperationQueue
  private(set) var state: DirectoryMonitorState = .stopped
  weak var delegate: Delegate?

  init(directory: URL, queue: OperationQueue = .main) throws {
    self.directory = directory
    self.queue = queue
  }

  // MARK: - Public methods
  func start(completion: ((Result<URL, Error>) -> Void)? = nil) {
    guard state != .started else { return }

    bookmarksManager.setCategoryFilesChangedCallback { [weak self] type, URLs in
      guard let self else { return }
      self.queue.addOperation { self.handleChanges(type: type, URLs: URLs) }
    }

    queue.addOperation { [weak self] in
      guard let self else { return }
      self.handleGathering(completion: completion)
    }
  }

  func stop() {
    guard state == .started else { return }
    LOG(.info, "SYNC: Stop local monitor.")
    didFinishGatheringIsCalled = false
    state = .stopped
    bookmarksManager.setCategoryFilesChangedCallback(nil)
  }

  // MARK: - Private helpers
  private func handleChanges(type: FileOperationType, URLs: [URL]) {
    do {
      guard state == .started else { return }
      let currentContents = try bookmarksManager.categoryFilesList().map { try LocalMetadataItem(fileUrl: $0) }
      LOG(.info, "SYNC: Local contents count: \(currentContents.count)")

      let metadata = try URLs.map { try LocalMetadataItem(fileUrl: $0) }
      let update: LocalContentsUpdate
      let logTitle: String

      switch type {
      case .created:
        update = LocalContentsUpdate(added: metadata, updated: [], removed: [])
        logTitle = "Added to the local"
      case .updated:
        update = LocalContentsUpdate(added: [], updated: metadata, removed: [])
        logTitle = "Updated in the local"
      case .deleted:
        update = LocalContentsUpdate(added: [], updated: [], removed: metadata)
        logTitle = "Removed from the local"
      @unknown default:
        fatalError("Unhandled case")
      }

      LOG(.info, "SYNC: \(logTitle) (\(metadata.count)): \n\(metadata.shortDebugDescription)")
      delegate?.didUpdate(currentContents, update)
    } catch {
      delegate?.didReceiveLocalMonitorError(error)
    }
  }

  private func handleGathering(completion: ((Result<URL, Error>) -> Void)?) {
    do {
      didFinishGatheringIsCalled = true
      state = .started
      let content = try bookmarksManager.categoryFilesList().map { try LocalMetadataItem(fileUrl: $0) }
      LOG(.info, "SYNC: Local contents \(content.count):")
      content.sorted(by: { $0.fileName > $1.fileName }).forEach { LOG(.info, "SYNC: " + $0.shortDebugDescription) }
      completion?(.success(directory))
      delegate?.didFinishGathering(content)
    } catch {
      stop()
      completion?(.failure(error))
    }
  }
}
