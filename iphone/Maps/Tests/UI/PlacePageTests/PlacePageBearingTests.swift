import CoreLocation
@testable import Organic_Maps__Debug_
import XCTest

final class PlacePageBearingTests: XCTestCase {
  override func setUp() {
    super.setUp()
    if !StyleManager.shared.hasTheme() {
      StyleManager.shared.setTheme(MainTheme())
    }
  }

  func testCoordinateBearingsUseTrueNorthClockwise() {
    let origin = CLLocationCoordinate2D(latitude: 0, longitude: 0)
    let cases: [(Double, Double, Int)] = [
      (1, 0, 0), (1, 1, 45), (0, 1, 90), (-1, 1, 135),
      (-1, 0, 180), (-1, -1, 225), (0, -1, 270), (1, -1, 315),
    ]
    for (latitude, longitude, expected) in cases {
      let target = CLLocationCoordinate2D(latitude: latitude, longitude: longitude)
      let angle = GeoUtil.angle(atPoint: origin, toPoint: target)
      XCTAssertEqual(PlacePageCommonLayout.bearingDegrees(from: angle), expected)
    }
  }

  func testRoundingWrapsAtNorth() {
    let cases: [(Double, Int)] = [(359.4, 359), (359.6, 0), (0.4, 0), (0.6, 1)]
    for (bearing, expected) in cases {
      let angle = Float(((90 - bearing) * .pi / 180).remainder(dividingBy: 2 * .pi))
      XCTAssertEqual(PlacePageCommonLayout.bearingDegrees(from: angle), expected)
    }
  }

  func testTurningDeviceChangesArrowButNotBearing() throws {
    let (layout, preview) = try makeLayout(latitude: 0, longitude: 1)
    layout.onLocationUpdate(CLLocation(latitude: 0, longitude: 0))
    let arrow = try arrow(in: preview.fullScreenDirectionView)
    let bearing = try bearingLabel(in: preview.fullScreenDirectionView)
    for (heading, rotation) in [(0.0, 90.0), (90, 0), (180, -90), (270, -180)] {
      layout.onHeadingUpdate(MockHeading(trueHeading: heading))
      XCTAssertEqual(bearing.text, "90°")
      XCTAssertFalse(arrow.isHidden)
      assertRotation(arrow, degrees: rotation)
    }
  }

  func testLocationUpdatesRefreshArrowFromLatestHeading() throws {
    var heading: CLHeading? = MockHeading(trueHeading: 0)
    let (layout, preview) = try makeLayout(latitude: 1, longitude: 0, headingProvider: { heading })
    layout.onLocationUpdate(CLLocation(latitude: 0, longitude: 0))
    let arrow = try arrow(in: preview.fullScreenDirectionView)
    let bearing = try bearingLabel(in: preview.fullScreenDirectionView)
    XCTAssertEqual(bearing.text, "0°")
    assertRotation(arrow, degrees: 0)

    layout.onLocationUpdate(CLLocation(latitude: 1, longitude: -1))
    XCTAssertEqual(bearing.text, "90°")
    assertRotation(arrow, degrees: 90)

    heading = nil
    layout.onLocationUpdate(CLLocation(latitude: 1, longitude: -1))
    XCTAssertTrue(arrow.isHidden)
    XCTAssertEqual(bearing.text, "90°")
  }

  func testUnavailableHeadingKeepsBearingAndDistance() throws {
    let (layout, preview) = try makeLayout(latitude: 0, longitude: 1)
    layout.onHeadingUpdate(MockHeading(trueHeading: 0))
    let arrow = try arrow(in: preview.fullScreenDirectionView)
    XCTAssertTrue(arrow.isHidden)

    layout.onLocationUpdate(CLLocation(latitude: 0, longitude: 0))
    XCTAssertEqual(try bearingLabel(in: preview.fullScreenDirectionView).text, "90°")
    XCTAssertTrue(arrow.isHidden)

    layout.onHeadingUpdate(MockHeading(trueHeading: 0))
    XCTAssertFalse(arrow.isHidden)
    layout.onHeadingUpdate(MockHeading(trueHeading: -1))
    XCTAssertTrue(arrow.isHidden)
    XCTAssertEqual(try bearingLabel(in: preview.fullScreenDirectionView).text, "90°")

    preview.loadViewIfNeeded()
    XCTAssertFalse(try XCTUnwrap(preview.placePageDirectionView).isHidden)
    XCTAssertTrue(try XCTUnwrap(preview.placePageDirectionView).imageView.isHidden)
    XCTAssertFalse(try XCTUnwrap(preview.placePageDirectionView?.label.text).isEmpty)
  }

  func testPreviewReplaysValuesReceivedBeforeViewLoads() throws {
    let preview = makePreview()
    preview.updateDistance("1 km")
    preview.updateBearing(90)
    preview.updateHeading(.pi / 2)
    XCTAssertFalse(preview.isViewLoaded)
    preview.loadViewIfNeeded()
    XCTAssertEqual(preview.placePageDirectionView?.label.text, "1 km")
    XCTAssertEqual(try bearingLabel(in: preview.fullScreenDirectionView).text, "90°")
    XCTAssertFalse(try XCTUnwrap(preview.placePageDirectionView).imageView.isHidden)
    preview.updateHeading(nil)
    XCTAssertTrue(try XCTUnwrap(preview.placePageDirectionView).imageView.isHidden)
    XCTAssertEqual(preview.placePageDirectionView?.label.text, "1 km")
  }

  func testDialogOpensWithMeasurementsWithoutHeading() {
    let preview = makePreview()
    let direction = RecordingDirectionView()
    preview.fullScreenDirectionView = direction
    preview.onDirectionPressed(self)
    preview.updateDistance("1 km")
    preview.onDirectionPressed(self)
    XCTAssertEqual(direction.showCount, 0)

    preview.updateBearing(90)
    preview.onDirectionPressed(self)
    XCTAssertEqual(direction.showCount, 1)
    XCTAssertEqual(direction.distance, "1 km")
    XCTAssertEqual(direction.bearing, 90)
    XCTAssertNil(direction.angle)

    preview.updateHeading(.pi / 2)
    XCTAssertEqual(direction.angle, .pi / 2)
    preview.updateHeading(nil)
    XCTAssertNil(direction.angle)
    XCTAssertEqual(direction.bearing, 90)
  }

  private func makeLayout(latitude: Double, longitude: Double,
                          headingProvider: @escaping () -> CLHeading? = { nil }) throws -> (PlacePageCommonLayout, PlacePagePreviewViewController) {
    let data = DirectionPlacePageData(latitude: latitude, longitude: longitude)
    let layout = PlacePageCommonLayout(interactor: PlacePageInteractor(data: data),
                                       storyboard: UIStoryboard.instance(.placePage), data: data, headingProvider: headingProvider)
    let preview = try XCTUnwrap(layout.headerViewControllers.last as? PlacePagePreviewViewController)
    return (layout, preview)
  }

  private func makePreview() -> PlacePagePreviewViewController {
    let preview = UIStoryboard.instance(.placePage).instantiateViewController(ofType: PlacePagePreviewViewController.self)
    preview.placePagePreviewData = DirectionPreviewData(trackInfo: TrackInfo.empty())
    return preview
  }

  private func arrow(in view: DirectionView) throws -> UIImageView {
    try XCTUnwrap(view.directionArrow)
  }

  private func bearingLabel(in view: DirectionView) throws -> UILabel {
    try XCTUnwrap(view.bearingLabel)
  }

  private func assertRotation(_ arrow: UIImageView, degrees: Double, file: StaticString = #filePath, line: UInt = #line) {
    let expected = CGAffineTransform(rotationAngle: CGFloat(degrees * .pi / 180))
    XCTAssertEqual(arrow.transform.a, expected.a, accuracy: 0.00001, file: file, line: line)
    XCTAssertEqual(arrow.transform.b, expected.b, accuracy: 0.00001, file: file, line: line)
  }
}

private final class MockHeading: CLHeading {
  private let direction: CLLocationDirection

  init(trueHeading: CLLocationDirection) {
    direction = trueHeading
    super.init()
  }

  @available(*, unavailable)
  required init?(coder _: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override var trueHeading: CLLocationDirection { direction }
  override var headingAccuracy: CLLocationDirection { 5 }
}

private final class DirectionPlacePageData: PlacePageData {
  private let coordinate: CLLocationCoordinate2D
  private let preview = DirectionPreviewData(trackInfo: TrackInfo.empty())

  init(latitude: Double, longitude: Double) {
    coordinate = CLLocationCoordinate2D(latitude: latitude, longitude: longitude)
    super.init(trackInfo: TrackInfo.empty(), elevationInfo: nil)
  }

  override var locationCoordinate: CLLocationCoordinate2D { coordinate }
  override var objectType: PlacePageObjectType { .POI }
  override var previewData: PlacePagePreviewData { preview }
  override var trackData: PlacePageTrackData? { nil }
}

private final class DirectionPreviewData: PlacePagePreviewData {
  override var title: String? { "Destination" }
  override var subtitle: String? { "Point" }
}

private final class RecordingDirectionView: DirectionView {
  var showCount = 0
  var distance: String?
  var bearing: Int?
  var angle: CGFloat?

  override func updateTitle(_: String?, subtitle _: String?) {}
  override func updateDistance(_ distance: String?) { self.distance = distance }
  override func updateBearing(_ bearing: Int) { self.bearing = bearing }
  override func updateHeading(_ angle: CGFloat?) { self.angle = angle }
  override func show() { showCount += 1 }
}
