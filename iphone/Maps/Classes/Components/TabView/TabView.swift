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

  var attributedText: NSAttributedString? {
    didSet {
      label.attributedText = attributedText
    }
  }

  override func prepareForReuse() {
    super.prepareForReuse()
    attributedText = nil
  }

  override func layoutSubviews() {
    super.layoutSubviews()
    label.frame = contentView.bounds
  }
}

protocol TabViewDataSource: AnyObject {
  func numberOfPages(in tabView: TabView) -> Int
  func tabView(_ tabView: TabView, viewAt index: Int) -> UIView
  func tabView(_ tabView: TabView, titleAt index: Int) -> String?
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
  var selectedIndex = -1

  weak var dataSource: TabViewDataSource?

  var barTintColor = UIColor.white {
    didSet {
      headerView.backgroundColor = barTintColor
    }
  }

  var headerTextAttributes: [NSAttributedStringKey : Any] = [
    .foregroundColor : UIColor.white,
    .font : UIFont.systemFont(ofSize: 14, weight: .semibold)
    ] {
    didSet {
      tabsCollectionView.reloadData()
    }
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

    headerView.layer.shadowOffset = CGSize(width: 0, height: 2)
    headerView.layer.shadowColor = UIColor(white: 0, alpha: 1).cgColor
    headerView.layer.shadowOpacity = 0.12
    headerView.layer.shadowRadius = 2
    headerView.layer.masksToBounds = false
    headerView.backgroundColor = barTintColor
    headerView.addSubview(tabsCollectionView)
    headerView.addSubview(slidingView)
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

    let views = ["header": headerView, "content": tabsContentCollectionView]
    addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|[header]|",
                                                  options: [], metrics: [:], views: views))
    addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|[content]|",
                                                  options: [], metrics: [:], views: views))
    addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:|[header(46)][content]|",
                                                  options: [], metrics: [:], views: views))

    let headerViews = ["tabs": tabsCollectionView, "slider": slidingView]
    headerView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "H:|[tabs]|",
                                                             options: [], metrics: [:], views: headerViews))
    headerView.addConstraints(NSLayoutConstraint.constraints(withVisualFormat: "V:|[tabs][slider(3)]|",
                                                             options: [], metrics: [:], views: headerViews))

    slidingViewLeft = NSLayoutConstraint(item: slidingView, attribute: .left, relatedBy: .equal,
                                         toItem: headerView, attribute: .left, multiplier: 1, constant: 0)
    headerView.addConstraint(slidingViewLeft)
    slidingViewWidth = NSLayoutConstraint(item: slidingView, attribute: .width, relatedBy: .equal,
                                          toItem: nil, attribute: .notAnAttribute, multiplier: 1, constant: 0)
    slidingView.addConstraint(slidingViewWidth)
  }

  override func layoutSubviews() {
    tabsLayout.invalidateLayout()
    tabsContentLayout.invalidateLayout()
    super.layoutSubviews()
    slidingViewWidth.constant = pageCount > 0 ? bounds.width / CGFloat(pageCount) : 0
    tabsContentCollectionView.layoutIfNeeded()
    if selectedIndex >= 0 {
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
        headerCell.attributedText = NSAttributedString(string: title.uppercased(), attributes: headerTextAttributes)
      }
    }

    return cell
  }
}

extension TabView : UICollectionViewDelegateFlowLayout {
  func scrollViewDidScroll(_ scrollView: UIScrollView) {
    if scrollView.contentSize.width > 0 {
      let scrollOffset = scrollView.contentOffset.x / scrollView.contentSize.width
      slidingViewLeft.constant = scrollOffset * bounds.width
    }
  }

  func scrollViewDidEndDecelerating(_ scrollView: UIScrollView) {
    selectedIndex = Int(round(scrollView.contentOffset.x / scrollView.bounds.width))
  }

  func collectionView(_ collectionView: UICollectionView, didSelectItemAt indexPath: IndexPath) {
    if (collectionView == tabsCollectionView) {
      selectedIndex = indexPath.item
      tabsContentCollectionView.scrollToItem(at: indexPath, at: .left, animated: true)
    }
  }

  func collectionView(_ collectionView: UICollectionView,
                      layout collectionViewLayout: UICollectionViewLayout,
                      sizeForItemAt indexPath: IndexPath) -> CGSize {
    if collectionView == tabsContentCollectionView {
      return collectionView.bounds.size
    } else {
      return CGSize(width: collectionView.bounds.width / CGFloat(pageCount),
                    height: collectionView.bounds.height)
    }
  }
}
