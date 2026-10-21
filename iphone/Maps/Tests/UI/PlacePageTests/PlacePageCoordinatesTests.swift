@testable import Organic_Maps__Debug_
import UIKit
import XCTest

final class PlacePageCoordinatesTests: XCTestCase {
  func test_TerrainElevationIsPreservedWhenCoordinatesChangeButIsNotCopied() throws {
    let formats = ["43.7, 7.3", "43°42′N 7°18′E"]
    let data = makeData(formats: formats, elevation: "100 m")
    defer { withExtendedLifetime(data) {} }
    let delegate = CoordinatesDelegate()
    let controller = makeController(data: data, delegate: delegate)
    let savedFormat = controller.coordinatesFormatId
    defer { controller.coordinatesFormatId = savedFormat }
    controller.coordinatesFormatId = 0
    controller.loadViewIfNeeded()
    let row = try coordinatesRow(in: controller)

    XCTAssertEqual(row.textLabel.text, "\(formats[0])  ▲ 100 m")
    XCTAssertEqual(row.accessoryButton.menu?.children.compactMap { ($0 as? UIAction)?.title }, formats)
    row.infoLabelLongPressHandler?()
    XCTAssertEqual(delegate.copiedContent, formats[0])

    row.infoLabelTapHandler?()
    XCTAssertEqual(row.textLabel.text, "\(formats[1])  ▲ 100 m")
    row.infoLabelLongPressHandler?()
    XCTAssertEqual(delegate.copiedContent, formats[1])
  }

  func test_CoordinatesShowNoElevationForSelectionWithoutTerrain() throws {
    let formats = ["43.7, 7.3", "43°42′N 7°18′E"]
    let data = makeData(formats: formats, elevation: nil)
    defer { withExtendedLifetime(data) {} }
    let controller = makeController(data: data)
    let savedFormat = controller.coordinatesFormatId
    defer { controller.coordinatesFormatId = savedFormat }
    controller.coordinatesFormatId = 0
    controller.loadViewIfNeeded()
    let row = try coordinatesRow(in: controller)

    XCTAssertEqual(row.textLabel.text, formats[0])
    row.infoLabelTapHandler?()
    XCTAssertEqual(row.textLabel.text, formats[1])
  }

  private func makeData(formats: [String], elevation: String?) -> PlacePageInfoData {
    let data = PlacePageInfoData()
    data.setValue([], forKey: "phones")
    data.setValue(formats, forKey: "coordFormats")
    data.setValue(elevation, forKey: "formattedTerrainElevation")
    return data
  }

  private func makeController(data: PlacePageInfoData,
                              delegate: PlacePageInfoViewControllerDelegate? = nil) -> PlacePageInfoViewController {
    let controller = PlacePageInfoViewController()
    controller.placePageInfoData = data
    controller.delegate = delegate
    controller.stackView = UIStackView()
    return controller
  }

  private func coordinatesRow(in controller: PlacePageInfoViewController) throws -> InfoItemView {
    try XCTUnwrap(controller.stackView.arrangedSubviews.compactMap { $0 as? InfoItemView }.first)
  }
}

private final class CoordinatesDelegate: PlacePageInfoViewControllerDelegate {
  let shouldShowOpenInApp = false
  private(set) var copiedContent: String?

  func didCopy(_ content: String) { copiedContent = content }
  func didPressCall(to _: PlacePagePhone) {}
  func didPressWebsite() {}
  func didPressHeritageWebsite() {}
  func didPressWebsiteMenu() {}
  func didPressWikipedia() {}
  func didPressWikimediaCommons() {}
  func didPressFacebook() {}
  func didPressInstagram() {}
  func didPressTwitter() {}
  func didPressVk() {}
  func didPressLine() {}
  func didPressEmail() {}
  func didPressOpenInApp(from _: UIView) {}
  func didSelectPublicTransportRoute(scrollAnchor _: UIView) {}
}
