fileprivate class ContentCell: UICollectionViewCell {
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

fileprivate class HeaderCell: UICollectionViewCell {
  private let label = UILabel()
  private var selectedAttributes: [NSAttributedString.Key : Any] = [:]
  private var deselectedAttributes: [NSAttributedString.Key : Any] = [:]

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

  override var isSelected: Bool {
    didSet {
      label.attributedText = NSAttributedString(string: label.text ?? "",
                                            attributes: isSelected ? selectedAttributes : deselectedAttributes)
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    label.attributedText = nil
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    label.frame = contentView.bounds
  }

  func configureWith(selectedAttributes: [NSAttributedString.Key : Any],
                     deselectedAttributes: [NSAttributedString.Key : Any],
                     text: String) {
    self.selectedAttributes = selectedAttributes
    self.deselectedAttributes = deselectedAttributes
    label.attributedText = NSAttributedString(string: text.uppercased(),
                                              attributes: deselectedAttributes)
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
@objc(MWMTabView)
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
  private lazy var pageCount = { return self.dataSource?.numberOfPages(in: self) ?? 0; }()
  var selectedIndex: Int?
  private var lastSelectedIndex: Int?

  weak var dataSource: TabViewDataSource?
  weak var delegate: TabViewDelegate?

  var barTintColor = UIColor.white {
    didSet {
      headerView.backgroundColor = barTintColor
    }
  }

  var selectedHeaderTextAttributes: [NSAttributedString.Key : Any] = [
    .foregroundColor : UIColor.white,
    .font : UIFont.systemFont(ofSize: 14, weight: .semibold)
    ] {
    didSet {
      tabsCollectionView.reloadData()
    }
  }

  var deselectedHeaderTextAttributes: [NSAttributedString.Key : Any] = [
    .foregroundColor : UIColor.gray,
    .font : UIFont.systemFont(ofSize: 14, weight: .semibold)
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

  private func configure() {
    backgroundColor = .white

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
    tabsCollectionView.translatesAutoresizingMaskIntoConstraints = false;
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
  }
}

extension TabView : UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    return pageCount
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
                                 text: title)
        if indexPath.item == selectedIndex {
          collectionView.selectItem(at: indexPath, animated: false, scrollPosition: [])
        }
      }
    }

    return cell
  }
}

extension TabView : UICollectionViewDelegateFlowLayout {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    if scrollView.contentSize.width > 0 {
      let scrollOffset = scrollView.contentOffset.x / scrollView.contentSize.width
      slidingViewLeft.constant = scrollOffset * contentFrame.width
    }
  }

  func scrollViewWillBeginDragging(_ scrollView: UIScrollView) {
    lastSelectedIndex = selectedIndex
  }

  func scrollViewDidEndDecelerating(_ scrollView: UIScrollView) {
    selectedIndex = Int(round(scrollView.contentOffset.x / scrollView.bounds.width))
    if let selectedIndex = selectedIndex, selectedIndex != lastSelectedIndex {
      delegate?.tabView(self, didSelectTabAt: selectedIndex)
    }
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    if (collectionView == tabsCollectionView) {
      let isSelected = selectedIndex == indexPath.item
      if !isSelected {
        selectedIndex = indexPath.item
        tabsContentCollectionView.scrollToItem(at: indexPath, at: .left, animated: true)
        delegate?.tabView(self, didSelectTabAt: selectedIndex!)
      }
    }
  }

  func collectionView(_ collectionView: UICollectionView, didDeselectItemAt indexPath: IndexPath) {
    if (collectionView == tabsCollectionView) {
      collectionView.deselectItem(at: indexPath, animated: false)
    }
  }

  func collectionView(_ collectionView: UICollectionView,
                      layout collectionViewLayout: UICollectionViewLayout,
                      sizeForItemAt indexPath: IndexPath) -> CGSize {
    let bounds = collectionView.bounds.inset(by: collectionView.adjustedContentInset)

    if collectionView == tabsContentCollectionView {
      return bounds.size
    } else {
      return CGSize(width: bounds.width / CGFloat(pageCount),
                    height: bounds.height)
    }
  }
}
