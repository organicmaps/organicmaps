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
