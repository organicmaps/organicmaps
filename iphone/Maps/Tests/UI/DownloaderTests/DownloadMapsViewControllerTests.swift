@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class DownloadMapsViewControllerTests: XCTestCase {
  func test_ProgressSnapshotUpdatesMapCircleAndLeavesInactiveFlowUnchanged() throws {
    let attrs = Storage.shared().attributes(forCountry: "Slovenia_East")
    let source = DownloaderDataSourceMock(attrs: attrs, isRoot: true)
    let storyboard = UIStoryboard(name: "Main", bundle: .main)
    let controller = try XCTUnwrap(
      storyboard.instantiateViewController(withIdentifier: "DownloadMapsViewController") as? DownloadMapsViewController
    )
    controller.dataSource = source
    let window = UIWindow(frame: CGRect(x: 0, y: 0, width: 400, height: 640))
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
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 0.25)

    controller.processCountry("unrelated",
                              progress: StorageProgress(mwmDownloaded: 100, mwmTotal: 100,
                                                        twmDownloaded: 200, twmTotal: 200))
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 0.25)

    controller.processCountry(attrs.countryId,
                              progress: StorageProgress(mwmDownloaded: 0, mwmTotal: 0,
                                                        twmDownloaded: 18, twmTotal: 20))
    checkProgress(map: CGFloat(AppConstants.maxProgress) * 0.25)
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
  func numberOfItems(in section: Int) -> Int { 1 }
  func item(at indexPath: IndexPath) -> MapNodeAttributes { attrs }
  func matchedName(at indexPath: IndexPath) -> String? { nil }
  func title(for section: Int) -> String { headerTitle }
  func indexTitles() -> [String]? { nil }
  func dataSourceFor(_ childId: String) -> IDownloaderDataSource { self }
  func reload() -> Bool { false }
  func search(_ query: String, locale: String, update: @escaping (Bool) -> Void) { update(true) }
  func cancelSearch() {}
}
