@testable import Organic_Maps__Debug_
import XCTest

final class ActionBarViewControllerTests: XCTestCase {
  private struct RouteState {
    var canAddStop = false
    var canReplaceStop = false
    var shouldAppendNewPoints = false
  }

  func test_GivenRouteState_WhenActionBarIsConfigured_ThenShowsMatchingRouteButtons() {
    let full = RouteState()
    let fullReplacing = RouteState(canReplaceStop: true)
    let appendingWithoutRoute = RouteState(shouldAppendNewPoints: true)
    let spare = RouteState(canAddStop: true)
    let spareReplacing = RouteState(canAddStop: true, canReplaceStop: true)
    let spareAppending = RouteState(canAddStop: true, shouldAppendNewPoints: true)

    let cases: [(PlacePageObjectType, RouteState, [ActionBarButtonType])] = [
      (.POI, fullReplacing, [.routeReplaceStop, .bookmark]),
      (.bookmark, fullReplacing, [.routeReplaceStop, .bookmark]),
      (.track, fullReplacing, [.routeReplaceStop, .bookmark, .track]),
      (.relationTrack, fullReplacing, [.routeReplaceStop, .bookmark]),
      (.track, spareReplacing, [.routeReplaceStop, .bookmark, .track]),
      (.relationTrack, spareReplacing, [.routeReplaceStop, .bookmark]),
      (.POI, spare, [.routeFrom, .routeAddStop, .bookmark, .routeTo]),
      (.track, spareAppending, [.routeAddDestination, .bookmark, .track]),
      (.POI, full, [.routeFrom, .bookmark, .routeTo]),
      (.track, full, [.routeFrom, .bookmark, .routeTo, .track]),
      (.bookmark, appendingWithoutRoute, [.routeAddDestination, .bookmark]),
      (.relationTrack, appendingWithoutRoute, [.routeAddDestination, .bookmark]),
      (.trackRecording, spareReplacing, [.deleteTrackRecording, .saveTrackRecording]),
    ]

    for (index, (objectType, routeState, expectedButtons)) in cases.enumerated() {
      XCTAssertEqual(buttons(for: objectType, routeState), expectedButtons, "Case \(index)")
    }
  }

  func test_GivenRoutingActions_WhenConfigured_ThenOnlyPrimaryActionIsBlue() throws {
    let cases: [(RouteState, ActionBarButtonType)] = [
      (RouteState(), .routeTo),
      (RouteState(canAddStop: true), .routeTo),
      (RouteState(canReplaceStop: true), .routeReplaceStop),
      (RouteState(shouldAppendNewPoints: true), .routeAddDestination),
    ]
    for (routeState, primary) in cases {
      let actionBar = makeActionBar(data: PlacePageDataStub(objectType: .POI), routeState: routeState)
      try assertColors(actionBar, primary: primary)
    }
  }

  func test_GivenDownloadAndRoutingActions_WhenConfigured_ThenOnlyDownloadIsBlue() throws {
    for state in [RouteState(), RouteState(canReplaceStop: true), RouteState(shouldAppendNewPoints: true)] {
      let data = PlacePageDataStub(objectType: .POI)
      data.stubMapNodeAttributes = MapNodeAttributesStub()
      try assertColors(makeActionBar(data: data, routeState: state), primary: .download)
    }
  }

  func test_GivenDownload_WhenCompleted_ThenPrimaryActionReturnsToRouteTo() throws {
    let data = PlacePageDataStub(objectType: .POI)
    let attributes = MapNodeAttributesStub()
    data.stubMapNodeAttributes = attributes
    let actionBar = makeActionBar(data: data)
    try assertColors(actionBar, primary: .download)

    attributes.status = .onDisk
    // The place page layouts rebuild the buttons when the download completes.
    actionBar.updateMapNodeStatus()
    XCTAssertNil(actionBar.downloadButton)
    try assertColors(actionBar, primary: .routeTo)
  }

  func test_GivenInstalledMap_WhenTerrainDownloads_ThenPrimaryActionStaysRouteTo() throws {
    let data = PlacePageDataStub(objectType: .POI)
    let attributes = MapNodeAttributesStub()
    attributes.status = .downloading
    attributes.stubMapStatus = .onDisk
    data.stubMapNodeAttributes = attributes
    let actionBar = makeActionBar(data: data)
    XCTAssertNil(actionBar.downloadButton)
    try assertColors(actionBar, primary: .routeTo)
  }

  func test_GivenOldHighlightPreference_WhenConfigured_ThenRouteToIsBlue() throws {
    let key = "kUDDidHighlightPoint2PointButton"
    let defaults = UserDefaults.standard
    let previous = defaults.object(forKey: key)
    defer {
      if let previous {
        defaults.set(previous, forKey: key)
      } else {
        defaults.removeObject(forKey: key)
      }
    }
    defaults.set(true, forKey: key)
    try assertColors(makeActionBar(data: PlacePageDataStub(objectType: .POI)), primary: .routeTo)
  }

  func test_GivenSavedBookmark_WhenConfigured_ThenFilledIconRemainsNeutral() throws {
    let data = PlacePageDataStub(objectType: .bookmark)
    data.stubBookmarkData = PlacePageBookmarkData()
    let actionBar = makeActionBar(data: data)
    try assertColors(actionBar, primary: .routeTo)
    let bookmark = try XCTUnwrap(actionBar.stackView.arrangedSubviews.compactMap { $0 as? ActionBarButton }
      .first { $0.type == .bookmark })
    let control = try XCTUnwrap(bookmark.subviews.compactMap { $0 as? UIButton }.first)
    XCTAssertEqual(control.image(for: .normal), UIImage(named: "ic_bookmarks_on"))
    XCTAssertFalse(control.isSelected)
  }

  func test_GivenTrackRecording_WhenConfigured_ThenDeleteIconAndTitleAreRed() throws {
    try assertColors(makeActionBar(data: PlacePageDataStub(objectType: .trackRecording)), primary: nil)
  }

  private func assertColors(_ actionBar: ActionBarViewController, primary: ActionBarButtonType?,
                            file: StaticString = #filePath, line: UInt = #line) throws {
    let actions = actionBar.stackView.arrangedSubviews.compactMap { $0 as? ActionBarButton }
    XCTAssertFalse(actions.isEmpty, file: file, line: line)
    if let primary {
      XCTAssertEqual(actions.filter { $0.type == primary }.count, 1, file: file, line: line)
    }
    let settled = expectation(description: "Download control state applied")
    DispatchQueue.main.async { settled.fulfill() }
    wait(for: [settled], timeout: 2)

    for appearance in [UIUserInterfaceStyle.light, .dark] {
      actionBar.overrideUserInterfaceStyle = appearance
      let traits = UITraitCollection(userInterfaceStyle: appearance)
      var blueCount = 0
      for action in actions {
        let control = try XCTUnwrap(iconButton(in: action), file: file, line: line)
        let label = try XCTUnwrap(action.subviews.compactMap { $0 as? UILabel }.first, file: file, line: line)
        let expected: UIColor = action.type == primary ? .linkBlue :
          (action.type == .track || action.type == .deleteTrackRecording ? .redPrimary : .blackSecondaryText)
        label.applyTheme()
        let tint = control.tintColor.resolvedColor(with: traits)
        XCTAssertEqual(tint, expected.resolvedColor(with: traits), "Icon for \(action.type)", file: file, line: line)
        XCTAssertEqual(label.textColor.resolvedColor(with: traits), expected.resolvedColor(with: traits),
                       "Title for \(action.type)", file: file, line: line)
        if tint == UIColor.linkBlue.resolvedColor(with: traits) {
          blueCount += 1
        }
      }
      XCTAssertEqual(blueCount, primary == nil ? 0 : 1, file: file, line: line)
    }
  }

  private func iconButton(in view: UIView) -> UIButton? {
    // Download embeds its visible icon in a circular progress control.
    for subview in view.subviews {
      if let button = iconButton(in: subview) {
        return button
      }
    }
    return (view as? UIButton).flatMap { $0.image(for: .normal) == nil ? nil : $0 }
  }

  private func makeActionBar(data: PlacePageDataStub, routeState: RouteState = RouteState()) -> ActionBarViewController {
    if !StyleManager.shared.hasTheme() {
      StyleManager.shared.setTheme(MainTheme())
    }
    let actionBar = UIStoryboard.instance(.placePage).instantiateViewController(ofType: ActionBarViewController.self)
    actionBar.placePageData = data
    actionBar.canAddStop = routeState.canAddStop
    actionBar.canReplaceStop = routeState.canReplaceStop
    actionBar.shouldAppendNewPoints = routeState.shouldAppendNewPoints
    actionBar.loadViewIfNeeded()
    return actionBar
  }

  private func buttons(for objectType: PlacePageObjectType, _ routeState: RouteState) -> [ActionBarButtonType] {
    let actionBar = makeActionBar(data: PlacePageDataStub(objectType: objectType), routeState: routeState)
    return actionBar.stackView.arrangedSubviews.compactMap { ($0 as? ActionBarButton)?.type }
  }
}

private final class PlacePageDataStub: PlacePageData {
  var stubMapNodeAttributes: MapNodeAttributes?
  var stubBookmarkData: PlacePageBookmarkData?
  private let stubObjectType: PlacePageObjectType

  init(objectType: PlacePageObjectType) {
    stubObjectType = objectType
    super.init(trackInfo: TrackInfo.empty(), elevationInfo: nil)
  }

  override var mapNodeAttributes: MapNodeAttributes? { stubMapNodeAttributes }
  override var bookmarkData: PlacePageBookmarkData? { stubBookmarkData }
  override var objectType: PlacePageObjectType { stubObjectType }
}

private final class MapNodeAttributesStub: MapNodeAttributes {
  var status: MapNodeStatus = .notDownloaded
  var stubMapStatus: MapNodeStatus?
  override var nodeStatus: MapNodeStatus { status }
  override var mapStatus: MapNodeStatus { stubMapStatus ?? status }
}
