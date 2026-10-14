@testable import Organic_Maps__Debug_
import XCTest

final class TransportRulerAppearanceTests: XCTestCase {
  func testDistanceTextRemainsReadableInBothAppearances() throws {
    let cell = try XCTUnwrap(UINib(nibName: "TransportRuler", bundle: .main)
      .instantiate(withOwner: nil).first as? TransportRuler)
    let step = MWMRouterTransitStepInfo()
    step.type = .ruler
    step.distance = "1.2"
    step.distanceUnits = "km"
    cell.config(step: step)
    let label = try XCTUnwrap(descendants(of: cell.contentView).compactMap { $0 as? UILabel }.first)
    XCTAssertEqual(label.text, "1.2 km")

    for style in [UIUserInterfaceStyle.light, .dark] {
      let color = label.textColor.resolvedColor(with: UITraitCollection(userInterfaceStyle: style))
      var white: CGFloat = 0
      var alpha: CGFloat = 0
      XCTAssertTrue(color.getWhite(&white, alpha: &alpha))
      XCTAssertGreaterThan(alpha, 0.8)
      if style == .dark {
        XCTAssertGreaterThan(white, 0.5)
      } else {
        XCTAssertLessThan(white, 0.5)
      }
    }
  }

  private func descendants(of view: UIView) -> [UIView] {
    view.subviews.flatMap { [$0] + descendants(of: $0) }
  }
}
