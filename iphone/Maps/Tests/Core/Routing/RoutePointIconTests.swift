@testable import Organic_Maps__Debug_
import XCTest

final class RoutePointIconTests: XCTestCase {
  func testBadgeRendersAtRoutePointListSize() {
    let image = RoutePointIconRenderer.image(number: 100)
    XCTAssertEqual(image.size, CGSize(width: 28, height: 28))
    let attachment = XCTAttachment(image: image)
    attachment.name = "Route point 100"
    attachment.lifetime = .keepAlways
    add(attachment)
  }

  func testThreeDigitNumberFitsInsideBadge() {
    let text = "100" as NSString
    let maxWidth: CGFloat = 21
    let font = RoutePointIconRenderer.fittedFont(for: text, maxWidth: maxWidth)
    XCTAssertLessThan(font.pointSize, UIFont.bold14.pointSize)
    XCTAssertLessThanOrEqual(text.size(withAttributes: [.font: font]).width, maxWidth)
  }

  func testAllSupportedStopsHaveDistinctBadgesInTheRoutePointList() throws {
    let points = makeRoutePoints()
    var images = Set<Data>()
    for number in 1 ... 100 {
      let image = points.defaultImage(for: number)
      XCTAssertTrue(try images.insert(XCTUnwrap(image.pngData())).inserted, "Repeated badge for stop \(number)")
    }
  }

  func testSummaryAndRoutePointListUseTheSameNumberedBadges() throws {
    let points = makeRoutePoints()
    let cell = try XCTUnwrap(UINib(nibName: "TransportTransitIntermediatePoint", bundle: .main)
      .instantiate(withOwner: nil).first as? TransportTransitIntermediatePoint)
    let icon = try XCTUnwrap(cell.contentView.subviews.first as? UIImageView)
    let step = MWMRouterTransitStepInfo()
    step.type = .intermediatePoint
    var images = Set<Data>()
    for number in 1 ... 100 {
      step.intermediateIndex = number - 1
      cell.config(step: step)
      let image = try XCTUnwrap(icon.image?.pngData())
      XCTAssertEqual(image, points.defaultImage(for: number).pngData(), "Stop \(number)")
      XCTAssertTrue(images.insert(image).inserted, "Repeated summary badge for stop \(number)")
    }
  }

  private func makeRoutePoints() -> NavigationDashboard.RoutePoints {
    let points = (0 ... 101).map { index in
      MWMRoutePoint(cgPoint: .zero,
                    title: "Stop \(index)",
                    subtitle: nil,
                    type: index == 0 ? .start : (index == 101 ? .finish : .intermediate),
                    intermediateIndex: index == 0 || index == 101 ? 0 : index - 1)
    }
    return NavigationDashboard.RoutePoints(points: points)
  }
}
