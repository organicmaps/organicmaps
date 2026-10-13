@testable import Organic_Maps__Debug_
import XCTest

final class TransportTransitStepsTests: XCTestCase {
  private let transportCases: [(type: MWMRouterTransitType, imageName: String)] = [
    (.subway, "ic_20px_route_planning_metro"),
    (.train, "ic_20px_route_planning_train"),
    (.lightRail, "ic_20px_route_planning_lightrail"),
    (.monorail, "ic_20px_route_planning_monorail"),
    (.tram, "ic_20px_route_planning_tram"),
    (.bus, "ic_20px_route_planning_bus"),
    (.ferry, "ic_20px_route_planning_ferry"),
    (.cableTram, "ic_20px_route_planning_tram"),
    (.aerialLift, "ic_20px_route_planning_aerial_lift"),
    (.funicular, "ic_20px_route_planning_funicular"),
    (.airService, "ic_bm_airport"),
    (.waterService, "ic_20px_route_planning_ferry"),
  ]

  func testTransportCellsKeepIconsNumbersAndColors() throws {
    for style in [UIUserInterfaceStyle.light, .dark] {
      let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
      collection.frame = CGRect(x: 0, y: 0, width: 1000, height: 200)
      collection.steps = transportCases.map { makeStep(type: $0.type, number: "12") }
      let controller = UIViewController()
      controller.overrideUserInterfaceStyle = style
      controller.view.addSubview(collection)
      let window = UIWindow(frame: collection.frame)
      window.rootViewController = controller
      window.isHidden = false
      defer { window.isHidden = true }
      collection.layoutIfNeeded()

      for (index, test) in transportCases.enumerated() {
        let type = test.type
        let cell = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: index, section: 0)))
        XCTAssertTrue(cell is TransportTransitTrain, "\(type)")
        let views = descendants(of: cell.contentView)
        let icon = try XCTUnwrap(views.compactMap { $0 as? UIImageView }.first)
        let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)
        let expected = try XCTUnwrap(UIImage(named: test.imageName))
        XCTAssertEqual(icon.image?.pngData(), try XCTUnwrap(expected.pngData()), "\(type)")
        XCTAssertEqual(icon.image?.renderingMode, .alwaysTemplate, "\(type)")
        XCTAssertGreaterThan(icon.image?.size.width ?? 0, 0, "\(type)")
        XCTAssertGreaterThan(icon.image?.size.height ?? 0, 0, "\(type)")
        XCTAssertEqual(icon.tintColor, .white)
        XCTAssertEqual(icon.superview?.backgroundColor, collection.steps[index].color)
        XCTAssertEqual(label.text, "12")
        XCTAssertEqual(label.textColor, .white)
        XCTAssertFalse(label.isHidden)
      }

      let renderer = UIGraphicsImageRenderer(bounds: collection.bounds)
      let attachment = XCTAttachment(image: renderer.image { context in
        UIColor.systemBackground.resolvedColor(with: collection.traitCollection).setFill()
        context.fill(collection.bounds)
        collection.layer.render(in: context.cgContext)
      })
      attachment.name = "Transit cells - \(style == .light ? "light" : "dark")"
      attachment.lifetime = .keepAlways
      add(attachment)
    }
  }

  func testTransportCellRecoversWhenLineNumberChanges() throws {
    let cell = try XCTUnwrap(UINib(nibName: "TransportTransitTrain", bundle: .main)
      .instantiate(withOwner: nil).first as? TransportTransitTrain)
    let views = descendants(of: cell.contentView)
    let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)
    let icon = try XCTUnwrap(views.compactMap { $0 as? UIImageView }.first)

    for test in transportCases {
      cell.config(step: makeStep(type: test.type, number: ""))
      XCTAssertTrue(label.isHidden)
      XCTAssertEqual(icon.tintColor, .white)
      let lightStep = makeStep(type: test.type, number: "F1")
      lightStep.color = .white
      cell.config(step: lightStep)
      XCTAssertFalse(label.isHidden)
      XCTAssertEqual(label.text, "F1")
      XCTAssertEqual(label.textColor, .black)
      XCTAssertEqual(icon.tintColor, .black)
    }
  }

  func testForegroundContrastsWithPaletteAndRawLineColors() throws {
    let url = try XCTUnwrap(Bundle.main.url(forResource: "transit_colors", withExtension: "txt"))
    let palette = try JSONDecoder().decode(TransitPalette.self, from: Data(contentsOf: url))
    let colors = palette.colors.values.flatMap { [$0.clear, $0.night] } + ["000000", "FFFFFF", "CCCCCC", "767676"]
    let cell = try XCTUnwrap(UINib(nibName: "TransportTransitTrain", bundle: .main)
      .instantiate(withOwner: nil).first as? TransportTransitTrain)
    let views = descendants(of: cell.contentView)
    let label = try XCTUnwrap(views.compactMap { $0 as? UILabel }.first)
    let icon = try XCTUnwrap(views.compactMap { $0 as? UIImageView }.first)

    for hex in colors {
      let rgb = try XCTUnwrap(UInt32(hex, radix: 16))
      let step = makeStep(type: .bus, number: "12")
      step.color = UIColor(CGFloat((rgb >> 16) & 255), CGFloat((rgb >> 8) & 255), CGFloat(rgb & 255), 1)
      cell.config(step: step)
      XCTAssertGreaterThanOrEqual(contrast(label.textColor, step.color), 4.5, hex)
      XCTAssertEqual(icon.tintColor, label.textColor, hex)
      XCTAssertEqual(icon.superview?.backgroundColor, step.color, hex)
    }
  }

  func testWalkingWaypointsAndRulerKeepTheirCells() throws {
    let types: [MWMRouterTransitType] = [.pedestrian, .intermediatePoint, .ruler]
    let collection = TransportTransitStepsCollectionView(layout: TransportTransitFlowLayout())
    collection.frame = CGRect(x: 0, y: 0, width: 300, height: 100)
    collection.steps = types.map { makeStep(type: $0, number: "") }
    collection.layoutIfNeeded()
    let walking = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 0, section: 0)))
    let waypoint = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 1, section: 0)))
    let ruler = try XCTUnwrap(collection.cellForItem(at: IndexPath(item: 2, section: 0)))
    XCTAssertTrue(walking is TransportTransitPedestrian)
    XCTAssertTrue(waypoint is TransportTransitIntermediatePoint)
    XCTAssertTrue(ruler is TransportRuler)
  }

  private func makeStep(type: MWMRouterTransitType, number: String) -> MWMRouterTransitStepInfo {
    let step = MWMRouterTransitStepInfo()
    step.type = type
    step.number = number
    step.distance = "42"
    step.distanceUnits = "m"
    step.color = UIColor(red: 47 / 255, green: 100 / 255, blue: 153 / 255, alpha: 1)
    step.intermediateIndex = 2
    return step
  }

  private func descendants(of view: UIView) -> [UIView] {
    view.subviews.flatMap { [$0] + descendants(of: $0) }
  }

  private func contrast(_ first: UIColor, _ second: UIColor) -> CGFloat {
    func luminance(_ color: UIColor) -> CGFloat {
      var red: CGFloat = 0
      var green: CGFloat = 0
      var blue: CGFloat = 0
      var alpha: CGFloat = 0
      precondition(color.getRed(&red, green: &green, blue: &blue, alpha: &alpha))
      func linear(_ component: CGFloat) -> CGFloat {
        component <= 0.04045 ? component / 12.92 : pow((component + 0.055) / 1.055, 2.4)
      }
      return 0.2126 * linear(red) + 0.7152 * linear(green) + 0.0722 * linear(blue)
    }
    let firstLuminance = luminance(first)
    let secondLuminance = luminance(second)
    return (max(firstLuminance, secondLuminance) + 0.05) / (min(firstLuminance, secondLuminance) + 0.05)
  }

  private struct TransitPalette: Decodable {
    struct Color: Decodable {
      let clear: String
      let night: String
    }

    let colors: [String: Color]
  }
}
