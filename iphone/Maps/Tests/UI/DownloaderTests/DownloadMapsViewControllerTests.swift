@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class DownloadMapsViewControllerTests: XCTestCase {
  func test_LeavesAndFoldersShowInstalledOrTransferSizeForTheirStatus() throws {
    let attrs = MapNodeAttributes()
    attrs.setValue("Test", forKey: "nodeName")
    let installed: UInt64 = 330 * 1024 * 1024
    let transfer: UInt64 = 300 * 1024 * 1024
    attrs.setValue(installed, forKey: "downloadedSize")
    attrs.setValue(transfer, forKey: "downloadingSize")
    let cell = try XCTUnwrap(
      UINib(nibName: "MWMMapDownloaderTableViewCell", bundle: .main)
        .instantiate(withOwner: nil).first as? MWMMapDownloaderTableViewCell
    )
    let label = try XCTUnwrap(cell.value(forKey: "downloadSize") as? UILabel)
    for count in [1, 3] {
      attrs.setValue(count, forKey: "totalMwmCount")
      attrs.setValue(count > 1 ? 2 : 1, forKey: "downloadedMwmCount")
      attrs.setValue(count > 1, forKey: "hasChildren")
      let modes: [MWMMapDownloaderMode] = [.available, .downloaded]
      let statuses: [MapNodeStatus] = [.onDisk, .onDiskOutOfDate, .notDownloaded, .downloading,
                                       .inQueue, .applying, .error, .partly]
      for mode in modes {
        cell.mode = mode
        for status in statuses {
          attrs.setValue(status.rawValue, forKey: "nodeStatus")
          cell.config(attrs, searchQuery: nil)
          let expected = status == .onDisk || (status == .partly && mode == .downloaded) ? installed : transfer
          XCTAssertEqual(label.text, formattedSize(expected), "\(count) maps, \(mode), \(status)")
          XCTAssertFalse(label.isHidden)
        }
      }
    }
  }

  func test_ProgressSnapshotCombinesMapAndTerrainInTheSameCircle() throws {
    let attrs = Storage.shared().attributes(forCountry: "Slovenia_East")
    let source = DownloaderDataSourceMock(attrs: attrs, isRoot: true)
    let storyboard = UIStoryboard(name: "Main", bundle: .main)
    let controller = try XCTUnwrap(
      storyboard.instantiateViewController(withIdentifier: "DownloadMapsViewController") as? DownloadMapsViewController
    )
    controller.dataSource = source
    let scene = try XCTUnwrap(UIApplication.shared.connectedScenes.first as? UIWindowScene)
    let window = UIWindow(windowScene: scene)
    window.frame = CGRect(x: 0, y: 0, width: 400, height: 640)
    window.rootViewController = controller
    window.isHidden = false
    defer { window.isHidden = true }
    controller.view.layoutIfNeeded()
    controller.tableView.layoutIfNeeded()
    let row = try XCTUnwrap(
      controller.tableView.cellForRow(at: IndexPath(row: 0, section: 0)) as? MWMMapDownloaderTableViewCell
    )

    func checkProgress(map: CGFloat) {
      let predicate = NSPredicate { _, _ in
        abs(row.progress.progress - map) < 0.0001
      }
      wait(for: [XCTNSPredicateExpectation(predicate: predicate, object: nil)], timeout: 3)
      XCTAssertEqual(row.progress.progress, map, accuracy: 0.0001)
    }

    controller.processCountry(attrs.countryId,
                              progress: StorageProgress(mwmDownloaded: 25, mwmTotal: 100,
                                                        twmDownloaded: 150, twmTotal: 200))
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 175 / 300)

    controller.processCountry("unrelated",
                              progress: StorageProgress(mwmDownloaded: 100, mwmTotal: 100,
                                                        twmDownloaded: 200, twmTotal: 200))
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 175 / 300)

    controller.processCountry(attrs.countryId,
                              progress: StorageProgress(mwmDownloaded: 100, mwmTotal: 100,
                                                        twmDownloaded: 100, twmTotal: 200))
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 2 / 3)

    controller.processCountry(attrs.countryId,
                              progress: StorageProgress(mwmDownloaded: 0, mwmTotal: 0,
                                                        twmDownloaded: 18, twmTotal: 20))
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 0.9)
  }
}

private final class DownloaderDataSourceMock: IDownloaderDataSource {
  let attrs: MapNodeAttributes
  let isRoot: Bool
  private let parentAttrs: MapNodeAttributes
  var headerTitle: String
  var isEmpty: Bool { false }
  var isSearching: Bool { false }
  var title: String { isRoot ? "Downloaded" : "Slovenia" }

  init(attrs: MapNodeAttributes, isRoot: Bool) {
    self.attrs = attrs
    self.isRoot = isRoot
    parentAttrs = isRoot ? Storage.shared().attributesForRoot() : Storage.shared().attributes(forCountry: "Slovenia")
    headerTitle = "\(isRoot ? "Downloaded" : "Slovenia") (139 MB)"
  }

  func getParentCountryId() -> String { parentAttrs.countryId }
  func parentAttributes() -> MapNodeAttributes { parentAttrs }
  func numberOfSections() -> Int { 1 }
  func numberOfItems(in _: Int) -> Int { 1 }
  func item(at _: IndexPath) -> MapNodeAttributes { attrs }
  func matchedName(at _: IndexPath) -> String? { nil }
  func title(for _: Int) -> String { headerTitle }
  func indexTitles() -> [String]? { nil }
  func dataSourceFor(_: String) -> IDownloaderDataSource { self }
  func reload() -> Bool { false }
  func search(_: String, locale _: String, update: @escaping (Bool) -> Void) { update(true) }
  func cancelSearch() {}
}
