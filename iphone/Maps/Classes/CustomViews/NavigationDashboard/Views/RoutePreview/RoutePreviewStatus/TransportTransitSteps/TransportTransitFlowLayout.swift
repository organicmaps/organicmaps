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

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    register(Config.separator, forDecorationViewOfKind: Config.separatorKind)
  }

  override var collectionViewContentSize: CGSize {
    let width = collectionView?.frame.width ?? 0
    let height = cellsLayoutAttrs.reduce(0) { return max($0, $1.frame.maxY) }
    return CGSize(width: width, height: height)
  }

  override func prepare() {
    super.prepare()
    let cv = collectionView as! TransportTransitStepsCollectionView

    let section = 0
    let width = cv.width
    var x = CGFloat(0)
    var y = CGFloat(0)

    cellsLayoutAttrs = []
    decoratorsLayoutAttrs = []
    for item in 0 ..< cv.numberOfItems(inSection: section) {
      let ip = IndexPath(item: item, section: section)

      if item != 0 {
        let sepAttr = UICollectionViewLayoutAttributes(forDecorationViewOfKind: Config.separatorKind, with: ip)
        sepAttr.frame = CGRect(origin: CGPoint(x: x, y: y), size: Config.separatorSize)
        decoratorsLayoutAttrs.append(sepAttr)

        x += Config.separatorSize.width
      }

      var cellSize = cv.estimatedCellSize(item: item)
      let spaceLeft = width - x - Config.separatorSize.width
      let minimumSpaceRequired = min(cellSize.width, Config.minimumCellWidthAfterShrink)
      if spaceLeft < minimumSpaceRequired {
        x = 0
        y += Config.separatorSize.height + Config.lineSpacing
      } else {
        cellSize.width = min(cellSize.width, spaceLeft)
      }

      let cellAttr = UICollectionViewLayoutAttributes(forCellWith: ip)
      cellAttr.frame = CGRect(origin: CGPoint(x: x, y: y), size: cellSize)
      cellsLayoutAttrs.append(cellAttr)

      x += cellSize.width
    }
  }

  override func layoutAttributesForItem(at indexPath: IndexPath) -> UICollectionViewLayoutAttributes? {
    return cellsLayoutAttrs.first(where: { $0.indexPath == indexPath })
  }

  override func layoutAttributesForDecorationView(ofKind _: String, at indexPath: IndexPath) -> UICollectionViewLayoutAttributes? {
    return decoratorsLayoutAttrs.first(where: { $0.indexPath == indexPath })
  }

  override func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
    var layoutAttrs = cellsLayoutAttrs.filter { $0.frame.intersects(rect) }
    layoutAttrs.append(contentsOf: decoratorsLayoutAttrs.filter { $0.frame.intersects(rect) })
    return layoutAttrs
  }
}
