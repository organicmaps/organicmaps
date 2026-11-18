import CoreLocation
@testable import Organic_Maps__Debug_
import XCTest

final class PlacePageHeadingLossTests: XCTestCase {
  override func setUp() {
    super.setUp()
    if !StyleManager.shared.hasTheme() {
      StyleManager.shared.setTheme(MainTheme())
    }
  }

  func testHeadingLossKeepsDistanceAndRecoversWithoutLocationUpdates() throws {
    let (layout, preview) = try makeLayout()
    preview.loadViewIfNeeded()
    layout.onLocationUpdate(CLLocation(latitude: 0, longitude: 0))
    layout.onHeadingUpdate(LossTestHeading(trueHeading: 0))

    preview.onDirectionPressed(self)
    let map = try XCTUnwrap(MapViewController.shared()?.view)
    let dialog = try XCTUnwrap(map.subviews.compactMap { $0 as? DirectionView }.last)
    defer { dialog.removeFromSuperview() }

    let previewDirection = try XCTUnwrap(preview.placePageDirectionView)
    let arrow = try XCTUnwrap(subviews(of: UIImageView.self, in: dialog).first)
    let distance = try XCTUnwrap(previewDirection.label.text)
    let dialogDistance = try XCTUnwrap(subviews(of: UILabel.self, in: dialog).first { $0.text == distance })
    XCTAssertFalse(distance.isEmpty)
    XCTAssertFalse(previewDirection.imageView.isHidden)
    XCTAssertFalse(arrow.isHidden)
    XCTAssertEqual(arrow.transform.b, 1, accuracy: 0.00001)
    XCTAssertEqual(previewDirection.imageView.transform.b, 1, accuracy: 0.00001)

    layout.onHeadingUpdate(nil)
    XCTAssertTrue(previewDirection.imageView.isHidden)
    XCTAssertTrue(arrow.isHidden)
    XCTAssertEqual(previewDirection.label.text, distance)
    XCTAssertEqual(dialogDistance.text, distance)
    XCTAssertFalse(dialogDistance.isHidden)

    layout.onHeadingUpdate(LossTestHeading(trueHeading: 180))
    XCTAssertFalse(previewDirection.imageView.isHidden)
    XCTAssertFalse(arrow.isHidden)
    XCTAssertEqual(arrow.transform.b, -1, accuracy: 0.00001)
    XCTAssertEqual(previewDirection.imageView.transform.b, -1, accuracy: 0.00001)
  }

  func testRecoveryReplacesRotationInFlight() throws {
    let scene = try XCTUnwrap(UIApplication.shared.connectedScenes.compactMap { $0 as? UIWindowScene }.first)
    let window = UIWindow(windowScene: scene)
    let host = UIViewController()
    window.rootViewController = host
    window.isHidden = false
    defer { window.isHidden = true }

    let dialog: DirectionView = try XCTUnwrap(Bundle.main.load(viewClass: DirectionView.self))
    host.view.addSubview(dialog)
    defer { dialog.removeFromSuperview() }
    dialog.alignToSuperview()
    host.view.layoutIfNeeded()

    let layer = try XCTUnwrap(subviews(of: UIImageView.self, in: dialog).first).layer
    dialog.updateHeading(0)
    waitForFrame()
    // Keep the rotation in flight even when the simulator takes longer to render a frame.
    let currentTime = layer.convertTime(CACurrentMediaTime(), from: nil)
    layer.speed = 0
    layer.timeOffset = currentTime
    dialog.updateHeading(.pi / 2)
    waitForFrame()
    XCTAssertEqual(layer.affineTransform().b, 0, accuracy: 0.00001)
    XCTAssertEqual(try XCTUnwrap(layer.presentation()).affineTransform().b, 1, accuracy: 0.00001)

    dialog.updateHeading(nil)
    dialog.updateHeading(.pi)
    waitForFrame()

    XCTAssertEqual(try XCTUnwrap(layer.presentation()).affineTransform().b, -1, accuracy: 0.00001)
  }

  func testUnavailableHeadingClearsStateBeforePreviewLoads() throws {
    for unavailableHeading in [nil, LossTestHeading(trueHeading: -1)] {
      let (layout, preview) = try makeLayout()
      layout.onLocationUpdate(CLLocation(latitude: 0, longitude: 0))
      layout.onHeadingUpdate(LossTestHeading(trueHeading: 0))
      layout.onHeadingUpdate(unavailableHeading)
      preview.loadViewIfNeeded()
      XCTAssertTrue(try XCTUnwrap(preview.placePageDirectionView).imageView.isHidden)
      XCTAssertFalse(try XCTUnwrap(preview.placePageDirectionView?.label.text).isEmpty)
    }
  }

  private func waitForFrame() {
    CATransaction.flush()
    let frame = expectation(description: "Render the direction arrow")
    DispatchQueue.main.asyncAfter(deadline: .now() + 0.05) { frame.fulfill() }
    wait(for: [frame], timeout: 2)
  }

  private func subviews<T: UIView>(of type: T.Type, in parent: UIView) -> [T] {
    parent.subviews.flatMap { view in
      (view as? T).map { [$0] } ?? subviews(of: type, in: view)
    }
  }

  private func makeLayout() throws -> (PlacePageCommonLayout, PlacePagePreviewViewController) {
    let data = LossTestPlacePageData()
    let layout = PlacePageCommonLayout(interactor: PlacePageInteractor(data: data),
                                       storyboard: UIStoryboard.instance(.placePage), data: data)
    let preview = try XCTUnwrap(layout.headerViewControllers.last as? PlacePagePreviewViewController)
    return (layout, preview)
  }
}

private final class LossTestHeading: CLHeading {
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

private final class LossTestPlacePageData: PlacePageData {
  private let preview = LossTestPreviewData(trackInfo: TrackInfo.empty())

  init() {
    super.init(trackInfo: TrackInfo.empty(), elevationInfo: nil)
  }

  override var locationCoordinate: CLLocationCoordinate2D { CLLocationCoordinate2D(latitude: 0, longitude: 1) }
  override var objectType: PlacePageObjectType { .POI }
  override var previewData: PlacePagePreviewData { preview }
  override var trackData: PlacePageTrackData? { nil }
}

private final class LossTestPreviewData: PlacePagePreviewData {
  override var title: String? { "Destination" }
  override var subtitle: String? { "Point" }
}
