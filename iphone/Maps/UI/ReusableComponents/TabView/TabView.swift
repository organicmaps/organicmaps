private class ContentCell: UICollectionViewCell {
  var view: UIView? {
    didSet {
      if let view = view, view != oldValue {
        oldValue?.removeFromSuperview()
        view.frame = contentView.bounds
        contentView.addSubview(view)
      }
    }
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    if let view = view {
      view.frame = contentView.bounds
    }
  }
}

private class HeaderCell: UICollectionViewCell {
  private let label = UILabel()
  private var selectedAttributes: [NSAttributedString.Key: Any] = [:]
  private var deselectedAttributes: [NSAttributedString.Key: Any] = [:]
  private var title = ""

  override init(frame: CGRect) {
    super.init(frame: frame)
    contentView.addSubview(label)
    label.textAlignment = .center
  }

  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    contentView.addSubview(label)
    label.textAlignment = .center
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    label.text = nil
    title = ""
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    label.frame = contentView.bounds
  }

  func configureWith(selectedAttributes: [NSAttributedString.Key: Any],
                     deselectedAttributes: [NSAttributedString.Key: Any],
                     text: String,
                     selectionProgress: CGFloat) {
    self.selectedAttributes = selectedAttributes
    self.deselectedAttributes = deselectedAttributes
    title = text.uppercased()
    label.font = deselectedAttributes[.font] as? UIFont
    label.adjustsFontForContentSizeCategory = true
    label.configureSingleLineAutoScaling()
    updateSelectionProgress(selectionProgress)
  }

  func updateSelectionProgress(_ selectionProgress: CGFloat) {
    let selectionProgress = max(0, min(1, selectionProgress))
    let deselectedColor = deselectedAttributes[.foregroundColor] as? UIColor
    let selectedColor = selectedAttributes[.foregroundColor] as? UIColor

    if let deselectedColor, let selectedColor {
      label.textColor = deselectedColor.interpolated(to: selectedColor, progress: selectionProgress)
    } else {
      label.textColor = deselectedColor ?? selectedColor
    }

    label.text = title
  }
}

protocol TabViewDataSource: AnyObject {
  func numberOfPages(in tabView: TabView) -> Int
  func tabView(_ tabView: TabView, viewAt index: Int) -> UIView
  func tabView(_ tabView: TabView, titleAt index: Int) -> String?
}

protocol TabViewDelegate: AnyObject {
  func tabView(_ tabView: TabView, didSelectTabAt index: Int)
}

@objcMembers
class TabView: UIView {
  private enum CellId {
    static let content = "contentCell"
    static let header = "headerCell"
  }

  private let tabsLayout = UICollectionViewFlowLayout()
  private let tabsContentLayout = UICollectionViewFlowLayout()
  private let tabsCollectionView: UICollectionView
  private let tabsContentCollectionView: UICollectionView
  private let headerView = UIView()
  private let slidingView = UIView()
  private var slidingViewLeft: NSLayoutConstraint!
  private var slidingViewWidth: NSLayoutConstraint!
  private lazy var pageCount = self.dataSource?.numberOfPages(in: self) ?? 0
  var selectedIndex: Int? {
    didSet {
      updateSelectedHeader()
    }
  }

  private var lastSelectedIndex: Int?

  weak var dataSource: TabViewDataSource?
  weak var delegate: TabViewDelegate?

  var barTintColor = UIColor.white {
    didSet {
      headerView.backgroundColor = barTintColor
    }
  }

  var selectedHeaderTextAttributes: [NSAttributedString.Key: Any] = [
    .foregroundColor: UIColor.white,
    .font: UIFont.semibold14.dynamic,
  ] {
    didSet {
      tabsCollectionView.reloadData()
    }
  }

  var deselectedHeaderTextAttributes: [NSAttributedString.Key: Any] = [
    .foregroundColor: UIColor.gray,
    .font: UIFont.semibold14.dynamic,
  ] {
    didSet {
      tabsCollectionView.reloadData()
    }
  }

  var contentFrame: CGRect {
    safeAreaLayoutGuide.layoutFrame
  }

  override var tintColor: UIColor! {
    didSet {
      slidingView.backgroundColor = tintColor
    }
  }

  override init(frame: CGRect) {
    tabsCollectionView = UICollectionView(frame: .zero, collectionViewLayout: tabsLayout)
    tabsContentCollectionView = UICollectionView(frame: .zero, collectionViewLayout: tabsContentLayout)
    super.init(frame: frame)
    configure()
  }

  required init?(coder aDecoder: NSCoder) {
    tabsCollectionView = UICollectionView(frame: .zero, collectionViewLayout: tabsLayout)
    tabsContentCollectionView = UICollectionView(frame: .zero, collectionViewLayout: tabsContentLayout)
    super.init(coder: aDecoder)
    configure()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) {
      updateSelectedHeader()
    }
  }

  private func configure() {
    backgroundColor = .whitePrimary

    configureHeader()
    configureContent()

    addSubview(tabsContentCollectionView)
    addSubview(headerView)

    configureLayoutContraints()
  }

  private func configureHeader() {
    tabsLayout.scrollDirection = .horizontal
    tabsLayout.minimumLineSpacing = 0
    tabsLayout.minimumInteritemSpacing = 0

    tabsCollectionView.register(HeaderCell.self, forCellWithReuseIdentifier: CellId.header)
    tabsCollectionView.dataSource = self
    tabsCollectionView.delegate = self
    tabsCollectionView.backgroundColor = .clear

    slidingView.backgroundColor = tintColor

    headerView.backgroundColor = barTintColor
    headerView.addSubview(tabsCollectionView)
    headerView.addSubview(slidingView)
    headerView.addSeparator(.bottom)
  }

  private func configureContent() {
    tabsContentLayout.scrollDirection = .horizontal
    tabsContentLayout.minimumLineSpacing = 0
    tabsContentLayout.minimumInteritemSpacing = 0

    tabsContentCollectionView.register(ContentCell.self, forCellWithReuseIdentifier: CellId.content)
    tabsContentCollectionView.dataSource = self
    tabsContentCollectionView.delegate = self
    tabsContentCollectionView.isPagingEnabled = true
    tabsContentCollectionView.bounces = false
    tabsContentCollectionView.showsVerticalScrollIndicator = false
    tabsContentCollectionView.showsHorizontalScrollIndicator = false
    tabsContentCollectionView.backgroundColor = .clear
  }

  private func configureLayoutContraints() {
    tabsCollectionView.translatesAutoresizingMaskIntoConstraints = false
    tabsContentCollectionView.translatesAutoresizingMaskIntoConstraints = false
    headerView.translatesAutoresizingMaskIntoConstraints = false
    slidingView.translatesAutoresizingMaskIntoConstraints = false

    headerView.leftAnchor.constraint(equalTo: leftAnchor).isActive = true
    headerView.rightAnchor.constraint(equalTo: rightAnchor).isActive = true
    headerView.topAnchor.constraint(equalTo: safeAreaLayoutGuide.topAnchor).isActive = true
    headerView.heightAnchor.constraint(equalToConstant: 46).isActive = true

    tabsContentCollectionView.leftAnchor.constraint(equalTo: safeAreaLayoutGuide.leftAnchor).isActive = true
    tabsContentCollectionView.rightAnchor.constraint(equalTo: safeAreaLayoutGuide.rightAnchor).isActive = true
    tabsContentCollectionView.topAnchor.constraint(equalTo: headerView.bottomAnchor).isActive = true
    tabsContentCollectionView.bottomAnchor.constraint(equalTo: safeAreaLayoutGuide.bottomAnchor).isActive = true

    tabsCollectionView.leftAnchor.constraint(equalTo: headerView.leftAnchor).isActive = true
    tabsCollectionView.rightAnchor.constraint(equalTo: headerView.rightAnchor).isActive = true
    tabsCollectionView.topAnchor.constraint(equalTo: headerView.topAnchor).isActive = true
    tabsCollectionView.bottomAnchor.constraint(equalTo: slidingView.topAnchor).isActive = true

    slidingView.heightAnchor.constraint(equalToConstant: 3).isActive = true
    slidingView.bottomAnchor.constraint(equalTo: headerView.bottomAnchor).isActive = true

    slidingViewLeft = slidingView.leftAnchor.constraint(equalTo: safeAreaLayoutGuide.leftAnchor)
    slidingViewLeft.isActive = true
    slidingViewWidth = slidingView.widthAnchor.constraint(equalToConstant: 0)
    slidingViewWidth.isActive = true
  }

  private func updateSelectedHeader() {
    tabsCollectionView.indexPathsForSelectedItems?.forEach {
      tabsCollectionView.deselectItem(at: $0, animated: false)
    }

    guard let selectedIndex,
          selectedIndex >= 0,
          tabsCollectionView.numberOfSections > 0,
          selectedIndex < tabsCollectionView.numberOfItems(inSection: 0) else {
      return
    }

    tabsCollectionView.selectItem(at: IndexPath(item: selectedIndex, section: 0),
                                  animated: false,
                                  scrollPosition: [])
    updateHeaderSelectionProgress()
  }

  private func headerSelectionProgress(for index: Int, selectedPosition: CGFloat) -> CGFloat {
    max(0, min(1, 1 - abs(CGFloat(index) - selectedPosition)))
  }

  private func currentSelectedPosition() -> CGFloat {
    guard tabsContentCollectionView.bounds.width > 0 else {
      return CGFloat(selectedIndex ?? 0)
    }

    return tabsContentCollectionView.contentOffset.x / tabsContentCollectionView.bounds.width
  }

  private func updateHeaderSelectionProgress() {
    let selectedPosition = currentSelectedPosition()
    for indexPath in tabsCollectionView.indexPathsForVisibleItems {
      guard let headerCell = tabsCollectionView.cellForItem(at: indexPath) as? HeaderCell else {
        continue
      }

      headerCell.updateSelectionProgress(headerSelectionProgress(for: indexPath.item,
                                                                 selectedPosition: selectedPosition))
    }
  }

  override func layoutSubviews() {
    tabsLayout.invalidateLayout()
    tabsContentLayout.invalidateLayout()
    super.layoutSubviews()
    assert(pageCount > 0)
    slidingViewWidth.constant = pageCount > 0 ? contentFrame.width / CGFloat(pageCount) : 0
    slidingViewLeft.constant = pageCount > 0 ? contentFrame.width / CGFloat(pageCount) * CGFloat(selectedIndex ?? 0) : 0
    tabsCollectionView.layoutIfNeeded()
    tabsContentCollectionView.layoutIfNeeded()
    if let selectedIndex = selectedIndex {
      tabsContentCollectionView.scrollToItem(at: IndexPath(item: selectedIndex, section: 0),
                                             at: .left,
                                             animated: false)
    }
    updateHeaderSelectionProgress()
  }
}

extension TabView: UICollectionViewDataSource {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    pageCount
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    var cell = UICollectionViewCell()
    if collectionView == tabsContentCollectionView {
      cell = collectionView.dequeueReusableCell(withReuseIdentifier: CellId.content, for: indexPath)
      if let contentCell = cell as? ContentCell {
        contentCell.view = dataSource?.tabView(self, viewAt: indexPath.item)
      }
    }

    if collectionView == tabsCollectionView {
      cell = collectionView.dequeueReusableCell(withReuseIdentifier: CellId.header, for: indexPath)
      if let headerCell = cell as? HeaderCell {
        let title = dataSource?.tabView(self, titleAt: indexPath.item) ?? ""
        headerCell.configureWith(selectedAttributes: selectedHeaderTextAttributes,
                                 deselectedAttributes: deselectedHeaderTextAttributes,
                                 text: title,
                                 selectionProgress: headerSelectionProgress(for: indexPath.item,
                                                                            selectedPosition: currentSelectedPosition()))
        if indexPath.item == selectedIndex {
          collectionView.selectItem(at: indexPath, animated: false, scrollPosition: [])
        }
      }
    }

    return cell
  }
}

extension TabView: UICollectionViewDelegateFlowLayout {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    guard scrollView == tabsContentCollectionView else {
      return
    }

    if scrollView.contentSize.width > 0 {
      let scrollOffset = scrollView.contentOffset.x / scrollView.contentSize.width
      slidingViewLeft.constant = scrollOffset * contentFrame.width
    }
    updateHeaderSelectionProgress()
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    guard scrollView == tabsContentCollectionView else {
      return
    }

    lastSelectedIndex = selectedIndex
  }

  func scrollViewDidEndDecelerating(_ scrollView: UIScrollView) {
    guard scrollView == tabsContentCollectionView else {
      return
    }

    selectedIndex = Int(round(scrollView.contentOffset.x / scrollView.bounds.width))
    if let selectedIndex = selectedIndex, selectedIndex != lastSelectedIndex {
      delegate?.tabView(self, didSelectTabAt: selectedIndex)
    }
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    if collectionView == tabsCollectionView {
      let isSelected = selectedIndex == indexPath.item
      if !isSelected {
        selectedIndex = indexPath.item
        tabsContentCollectionView.scrollToItem(at: indexPath, at: .left, animated: true)
        delegate?.tabView(self, didSelectTabAt: selectedIndex!)
      }
    }
  }

  func collectionView(_ collectionView: UICollectionView, didDeselectItemAt indexPath: IndexPath) {
    if collectionView == tabsCollectionView {
      collectionView.deselectItem(at: indexPath, animated: false)
    }
  }

  func collectionView(_ collectionView: UICollectionView,
                      layout _: UICollectionViewLayout,
                      sizeForItemAt _: IndexPath) -> CGSize {
    let bounds = collectionView.bounds.inset(by: collectionView.adjustedContentInset)

    if collectionView == tabsContentCollectionView {
      return bounds.size
    } else {
      return CGSize(width: bounds.width / CGFloat(pageCount),
                    height: bounds.height)
    }
  }
}
