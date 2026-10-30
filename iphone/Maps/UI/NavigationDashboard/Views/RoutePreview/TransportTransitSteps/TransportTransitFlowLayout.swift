final class TransportTransitFlowLayout: UICollectionViewLayout {
  enum Config {
    static let lineSpacing = CGFloat(8)
    static let separator = TransportTransitSeparator.self
    static let separatorKind = toString(separator)
    static let separatorSize = CGSize(width: 16, height: 20)
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

    let frames = cv.stepFrames
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

  struct Cursor {
    private let width: CGFloat
    private var x: CGFloat = 0
    private var y: CGFloat = 0
    private var isFirst = true
    private(set) var frames: (cells: [CGRect], separators: [CGRect]) = ([], [])

    init(width: CGFloat) {
      self.width = width
    }

    var remainingWidth: CGFloat {
      let separatorWidth = isFirst ? 0 : Config.separatorSize.width
      if x + separatorWidth > width {
        return max(0, width - separatorWidth)
      }
      return max(0, width - x - separatorWidth)
    }

    mutating func append(size: CGSize, hasFollowingStep: Bool, isContinuation: Bool = false) {
      var separator: CGRect?
      if isContinuation {
        x = 0
        y += Config.separatorSize.height + Config.lineSpacing
      } else if !isFirst {
        if x + Config.separatorSize.width > width {
          x = 0
          y += Config.separatorSize.height + Config.lineSpacing
        }
        separator = CGRect(origin: CGPoint(x: x, y: y), size: Config.separatorSize)
        x += Config.separatorSize.width
      }

      var cellSize = size
      cellSize.width = min(cellSize.width, max(0, width))
      // An identifier that needs the full row moves its following arrow to the next row.
      let canReserveSeparator = hasFollowingStep && cellSize.width + Config.separatorSize.width <= width
      let availableWidth = max(0, width - (canReserveSeparator ? Config.separatorSize.width : 0))
      if x > 0, x + cellSize.width > availableWidth {
        x = 0
        y += Config.separatorSize.height + Config.lineSpacing
      }

      let cell = CGRect(origin: CGPoint(x: x, y: y), size: cellSize)
      x += cellSize.width
      isFirst = false
      frames.cells.append(cell)
      if let separator {
        frames.separators.append(separator)
      }
    }
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
