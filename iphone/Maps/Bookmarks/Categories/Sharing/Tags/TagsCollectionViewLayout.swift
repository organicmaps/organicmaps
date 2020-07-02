final class TagsCollectionViewLayout: UICollectionViewLayout {
  private var headersCache: [IndexPath : UICollectionViewLayoutAttributes] = [:]
  private var cellsCache: [IndexPath : UICollectionViewLayoutAttributes] = [:]
  private var contentHeight: CGFloat = 0

  private var contentWidth: CGFloat {
    guard let collectionView = collectionView else {
      return 0
    }
    
    let insets = collectionView.contentInset
    return collectionView.bounds.width - (insets.left + insets.right)
  }

  override var collectionViewContentSize: CGSize {
    CGSize(width: contentWidth, height: contentHeight)
  }
  
  @IBInspectable var itemHeight: CGFloat = 50
  @IBInspectable var itemWidth: CGFloat = 50
  @IBInspectable var lineSpacing: CGFloat = 10
  @IBInspectable var elementsSpacing: CGFloat = 10
  @IBInspectable var showHeaders: Bool = true

  override func prepare() {
    super.prepare()
    
    guard let collectionView = collectionView else { return }
    
    var xOffset: CGFloat = 0
    var yOffset: CGFloat = 0
    contentHeight = 0
    var lastItemHeight: CGFloat = 0
    
    for section in 0 ..< collectionView.numberOfSections {
      xOffset = 0
      yOffset += (section == 0) ? 0 : lastItemHeight + 2 * lineSpacing
      
      let indexPath = IndexPath(item: 0, section: section)
      
      if showHeaders {
        let headerSize = headersCache[indexPath]?.size ?? CGSize(width: contentWidth, height: itemHeight)
        let frame = CGRect(x: 0, y: yOffset, width: headerSize.width, height: headerSize.height)
        let attr = UICollectionViewLayoutAttributes(forSupplementaryViewOfKind: UICollectionView.elementKindSectionHeader, with: indexPath)
        attr.frame = frame
        headersCache[indexPath] = attr
        
        yOffset += headerSize.height + lineSpacing
      }
      
      for item in 0 ..< collectionView.numberOfItems(inSection: section) {
        let indexPath = IndexPath(item: item, section: section)
        let itemSize = cellsCache[indexPath]?.size ?? CGSize(width: itemWidth, height: itemHeight)
        
        if xOffset + itemSize.width > contentWidth {
          xOffset = 0
          yOffset = yOffset + lineSpacing + lastItemHeight
        }
        
        let frame = CGRect(x: xOffset, y: yOffset, width: itemSize.width, height: itemSize.height)
        let attr = UICollectionViewLayoutAttributes(forCellWith: indexPath)
        attr.frame = frame
        cellsCache[indexPath] = attr
        
        xOffset += itemSize.width + elementsSpacing
        lastItemHeight = itemSize.height
        
        contentHeight = max(contentHeight, frame.maxY)
      }
    }
  }

  override func layoutAttributesForElements(in rect: CGRect) -> [UICollectionViewLayoutAttributes]? {
    [headersCache, cellsCache].reduce([]) { $0 + $1.values }.filter { $0.frame.intersects(rect) }
  }

  override func layoutAttributesForItem(at indexPath: IndexPath) -> UICollectionViewLayoutAttributes? {
    cellsCache[indexPath]
  }
  
  override func shouldInvalidateLayout(forPreferredLayoutAttributes preferredAttributes: UICollectionViewLayoutAttributes,
                                              withOriginalAttributes originalAttributes: UICollectionViewLayoutAttributes) -> Bool {
    //dont validate layout if original width already equals to contentWidth - it's the best deal we can offer to cell
    if preferredAttributes.size.height != originalAttributes.size.height ||
      (preferredAttributes.size.width != originalAttributes.size.width && originalAttributes.size.width < contentWidth) {
      if preferredAttributes.representedElementKind == UICollectionView.elementKindSectionHeader {
        headersCache[originalAttributes.indexPath]?.size = preferredAttributes.size
      } else {
        cellsCache[originalAttributes.indexPath]?.size = CGSize(width: min(preferredAttributes.size.width, contentWidth),
                                                                height: preferredAttributes.size.height)
      }
      return true
    }
    return false
  }
}
