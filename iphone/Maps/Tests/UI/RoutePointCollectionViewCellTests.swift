@testable import Organic_Maps__Debug_
import XCTest

final class RoutePointCollectionViewCellTests: XCTestCase {
  private let cellSize = CGSize(width: 320, height: 50)

  func test_GivenFinishAndAddCells_WhenSiblingOrderChanges_ThenDotsStayVisibleInLTRAndRTL() throws {
    for direction in [UISemanticContentAttribute.forceLeftToRight, .forceRightToLeft] {
      let finishFirst = try renderPair(direction: direction, finishFirst: true)
      let addFirst = try renderPair(direction: direction, finishFirst: false)
      XCTAssertEqual(finishFirst, addFirst, "Connector changed with cell order in \(direction)")
    }
  }

  func testFixedTrackRowsKeepNativeGeometryTypographyAndConnectorAppearance() throws {
    for appearance in [UIUserInterfaceStyle.light, .dark] {
      for direction in [UISemanticContentAttribute.forceLeftToRight, .forceRightToLeft] {
        let size = CGSize(width: 320, height: RoutePointCollectionViewCell.height())
        let ordinary = RoutePointCollectionViewCell(frame: CGRect(origin: .zero, size: size))
        let fixed = RoutePointCollectionViewCell(frame: CGRect(origin: .zero, size: size))
        var model = try RoutePointCollectionViewCell.PointViewModel(title: "Pier", image: XCTUnwrap(UIImage(named: "route-point-1")),
                                                                    usesCurrentPositionIcon: false, showCloseButton: false,
                                                                    maskedCorners: [], isPlaceholder: false, showSeparator: false,
                                                                    onCloseHandler: nil)
        ordinary.configure(with: .point(model), topConnector: .solid, bottomConnector: .solid)
        model.showReorderButton = false
        model.accessibilityDetail = "Track start · Riverside ride"
        fixed.configure(with: .point(model), topConnector: .solid, bottomConnector: .solid)
        for cell in [ordinary, fixed] {
          cell.overrideUserInterfaceStyle = appearance
          func apply(_ view: UIView) {
            view.semanticContentAttribute = direction
            view.subviews.forEach(apply)
            view.applyTheme()
          }
          apply(cell)
          cell.layoutIfNeeded()
        }
        let ordinaryBackground = try XCTUnwrap(ordinary.contentView.subviews.first)
        let fixedBackground = try XCTUnwrap(fixed.contentView.subviews.first)
        XCTAssertEqual(fixedBackground.frame, ordinaryBackground.frame)
        XCTAssertEqual(fixedBackground.backgroundColor, ordinaryBackground.backgroundColor)
        XCTAssertEqual(fixedBackground.layer.cornerRadius, ordinaryBackground.layer.cornerRadius)
        let ordinaryIcon = try XCTUnwrap(ordinaryBackground.subviews.compactMap { $0 as? UIImageView }.first)
        let fixedIcon = try XCTUnwrap(fixedBackground.subviews.compactMap { $0 as? UIImageView }.first)
        XCTAssertEqual(fixedIcon.frame, ordinaryIcon.frame)
        XCTAssertEqual(fixedIcon.bounds.size, CGSize(width: 28, height: 28))
        let ordinaryText = try XCTUnwrap(ordinaryBackground.subviews.compactMap { $0 as? UIStackView }.first)
        let fixedText = try XCTUnwrap(fixedBackground.subviews.compactMap { $0 as? UIStackView }.first)
        XCTAssertEqual(fixedText.arrangedSubviews.count, 1)
        XCTAssertEqual(fixedText.frame, ordinaryText.frame)
        let ordinaryTitle = try XCTUnwrap(ordinaryText.arrangedSubviews.first as? UILabel)
        let fixedTitle = try XCTUnwrap(fixedText.arrangedSubviews.first as? UILabel)
        XCTAssertEqual(fixedTitle.font, ordinaryTitle.font)
        XCTAssertEqual(fixedTitle.textColor, ordinaryTitle.textColor)
        XCTAssertEqual(try iconColumnImage(fixed, direction: direction), try iconColumnImage(ordinary, direction: direction))
        XCTAssertTrue(fixedBackground.subviews.compactMap { $0 as? UIButton }.allSatisfy(\.isHidden))
        XCTAssertEqual(fixed.accessibilityLabel, "Pier, Track start · Riverside ride")
        XCTAssertTrue(fixed.isAccessibilityElement)
        model.showReorderButton = true
        model.accessibilityDetail = nil
        fixed.configure(with: .point(model), topConnector: .solid, bottomConnector: .solid)
        XCTAssertFalse(fixed.isAccessibilityElement)
        XCTAssertEqual(fixedBackground.subviews.compactMap { $0 as? UIButton }.filter { !$0.isHidden }.count, 1)
      }
    }
  }

  func testReorderableTrackRowsExposeTrackNameToVoiceOver() {
    let cell = RoutePointCollectionViewCell(frame: CGRect(origin: .zero, size: cellSize))
    var model = RoutePointCollectionViewCell.PointViewModel(
      title: "Track start", image: UIImage.routePointFinish, usesCurrentPositionIcon: false,
      showCloseButton: false, maskedCorners: [], isPlaceholder: false, showSeparator: true,
      onCloseHandler: nil
    )
    model.accessibilityDetail = "Riverside ride"
    cell.configure(with: .point(model), topConnector: .solid, bottomConnector: .solid)
    XCTAssertTrue(cell.isAccessibilityElement)
    XCTAssertEqual(cell.accessibilityLabel, "Track start, Riverside ride")
  }

  private func iconColumnImage(_ cell: RoutePointCollectionViewCell, direction: UISemanticContentAttribute) throws -> Data {
    let originX: CGFloat = direction == .forceRightToLeft ? cell.bounds.width - 52 : 0
    let size = CGSize(width: 52, height: cell.bounds.height)
    let format = UIGraphicsImageRendererFormat.default()
    format.scale = 2
    let image = UIGraphicsImageRenderer(size: size, format: format).image { context in
      context.cgContext.translateBy(x: -originX, y: 0)
      cell.layer.render(in: context.cgContext)
    }
    return try XCTUnwrap(image.pngData())
  }

  private func renderPair(direction: UISemanticContentAttribute, finishFirst: Bool) throws -> Data {
    let container = UIView(frame: CGRect(x: 0, y: 0, width: cellSize.width, height: cellSize.height * 2))
    container.backgroundColor = .white
    container.semanticContentAttribute = direction

    let finish = RoutePointCollectionViewCell(frame: CGRect(origin: .zero, size: cellSize))
    let finishModel = RoutePointCollectionViewCell.PointViewModel(
      title: "Finish",
      image: UIImage.routePointFinish,
      usesCurrentPositionIcon: false,
      showCloseButton: false,
      maskedCorners: [],
      isPlaceholder: false,
      showSeparator: true,
      onCloseHandler: nil
    )
    finish.configure(with: .point(finishModel), topConnector: .solid, bottomConnector: .threeDots)

    let add = RoutePointCollectionViewCell(frame: CGRect(x: 0, y: cellSize.height,
                                                         width: cellSize.width, height: cellSize.height))
    add.configure(with: .addPoint, topConnector: .threeDots, bottomConnector: .none)
    finish.semanticContentAttribute = direction
    add.semanticContentAttribute = direction
    finish.contentView.semanticContentAttribute = direction
    add.contentView.semanticContentAttribute = direction
    finish.contentView.subviews.forEach { $0.semanticContentAttribute = direction }
    add.contentView.subviews.forEach { $0.semanticContentAttribute = direction }

    if finishFirst {
      container.addSubview(finish)
      container.addSubview(add)
    } else {
      container.addSubview(add)
      container.addSubview(finish)
    }
    container.layoutIfNeeded()
    finish.layoutIfNeeded()
    add.layoutIfNeeded()

    try assertConnectorAligned(in: finish, direction: direction)
    try assertConnectorAligned(in: add, direction: direction)

    let format = UIGraphicsImageRendererFormat.default()
    format.scale = 2
    format.opaque = true
    let image = UIGraphicsImageRenderer(bounds: container.bounds, format: format).image { context in
      container.layer.render(in: context.cgContext)
    }
    return try XCTUnwrap(image.pngData())
  }

  private func assertConnectorAligned(in cell: RoutePointCollectionViewCell,
                                      direction: UISemanticContentAttribute) throws {
    let background = try XCTUnwrap(cell.contentView.subviews.first)
    let icon = try XCTUnwrap(background.subviews.compactMap { $0 as? UIImageView }.first)
    let connector = try XCTUnwrap(background.layer.sublayers?.compactMap { $0 as? CAShapeLayer }.first)
    let path = try XCTUnwrap(connector.path)

    XCTAssertTrue(connector.masksToBounds)
    XCTAssertEqual(path.boundingBoxOfPath.midX, icon.frame.midX, accuracy: 0.5)
    if direction == .forceRightToLeft {
      XCTAssertGreaterThan(icon.frame.midX, cell.bounds.midX)
    } else {
      XCTAssertLessThan(icon.frame.midX, cell.bounds.midX)
    }
  }
}
