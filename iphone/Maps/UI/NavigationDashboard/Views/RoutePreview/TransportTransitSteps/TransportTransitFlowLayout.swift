final class TransportTransitFlowLayout: UICollectionViewLayout {
  enum Config {
    static let lineSpacing = CGFloat(8)
    static let separator = TransportTransitSeparator.self
    static let separatorKind = toString(separator)
    static let separatorSize = CGSize(width: 16, height: 20)
    static let minimumCellWidthAfterShrink = CGFloat(56)
  }

  private var cellsLayoutAttrs = [UICollectionViewLayoutAttributes]()
  private var decoratorsLayoutAttrs = [UICollectionViewLayoutAttributes]()

  override init() {
    super.init()
    register(Config.separator, forDecorationViewOfKind: Config.separatorKind)
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    register(Config.separator, forDecorationViewOfKind: Config.separatorKind)
  }

  override var flipsHorizontallyInOppositeLayoutDirection: Bool { true }

  override var collectionViewContentSize: CGSize {
    let width = collectionView?.frame.width ?? 0
    let height = cellsLayoutAttrs.reduce(0) { max($0, $1.frame.maxY) }
    return CGSize(width: width, height: height)
  }

  override func prepare() {
    super.prepare()
    let cv = collectionView as! TransportTransitStepsCollectionView

    let cellSizes = (0 ..< cv.numberOfItems(inSection: 0)).map { cv.estimatedCellSize(item: $0) }
    let frames = Self.frames(cellSizes: cellSizes, width: cv.width)
    let isRightToLeft = cv.effectiveUserInterfaceLayoutDirection == .rightToLeft

    cellsLayoutAttrs = frames.cells.enumerated().map { item, frame in
      let attr = UICollectionViewLayoutAttributes(forCellWith: IndexPath(item: item, section: 0))
      attr.frame = frame
      return attr
    }
    decoratorsLayoutAttrs = frames.separators.enumerated().map { item, frame in
      let attr = UICollectionViewLayoutAttributes(forDecorationViewOfKind: Config.separatorKind,
                                                  with: IndexPath(item: item + 1, section: 0))
      attr.frame = frame
      // UIKit mirrors positions; the separator artwork also follows the route direction.
      if isRightToLeft {
        attr.transform = CGAffineTransform(scaleX: -1, y: 1)
      }
      return attr
    }
  }

  static func frames(cellSizes: [CGSize], width: CGFloat) -> (cells: [CGRect], separators: [CGRect]) {
    var cells = [CGRect]()
    var separators = [CGRect]()
    var x = CGFloat(0)
    var y = CGFloat(0)

    for (item, size) in cellSizes.enumerated() {
      if item != 0 {
        separators.append(CGRect(origin: CGPoint(x: x, y: y), size: Config.separatorSize))
        x += Config.separatorSize.width
      }

      var cellSize = size
      // Reserve room for the separator drawn before the next step.
      let availableWidth = width - (item + 1 < cellSizes.count ? Config.separatorSize.width : 0)
      let minimumSpaceRequired = min(cellSize.width, Config.minimumCellWidthAfterShrink)
      if availableWidth - x < minimumSpaceRequired {
        x = 0
        y += Config.separatorSize.height + Config.lineSpacing
      }
      cellSize.width = min(cellSize.width, max(0, availableWidth - x))

      cells.append(CGRect(origin: CGPoint(x: x, y: y), size: cellSize))
      x += cellSize.width
    }
    return (cells, separators)
  }

  override func layoutAttributesForItem(at indexPath: IndexPath) -> UICollectionViewLayoutAttributes? {
    cellsLayoutAttrs.first(where: { $0.indexPath == indexPath })
  }

  override func layoutAttributesForDecorationView(ofKind _: String, at indexPath: IndexPath) -> UICollectionViewLayoutAttributes? {
    decoratorsLayoutAttrs.first(where: { $0.indexPath == indexPath })
  }

  override func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
    var layoutAttrs = cellsLayoutAttrs.filter { $0.frame.intersects(rect) }
    layoutAttrs.append(contentsOf: decoratorsLayoutAttrs.filter { $0.frame.intersects(rect) })
    return layoutAttrs
  }
}
