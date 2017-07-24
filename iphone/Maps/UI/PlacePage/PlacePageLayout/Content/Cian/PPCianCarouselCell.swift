@objc(MWMPPCianCarouselCell)
final class PPCianCarouselCell: MWMTableViewCell {
  @IBOutlet private weak var title: UILabel! {
    didSet {
      title.text = L("subtitle_rent")
      title.font = UIFont.bold14()
      title.textColor = UIColor.blackSecondaryText()
    }
  }
  @IBOutlet private weak var more: UIButton! {
    didSet {
      more.setImage(#imageLiteral(resourceName: "logo_cian"), for: .normal)
      more.titleLabel?.font = UIFont.regular17()
      more.setTitleColor(UIColor.linkBlue(), for: .normal)
    }
  }

  @IBOutlet private weak var collectionView: UICollectionView!
  var data: [CianItemModel]? {
    didSet {
      updateCollectionView { [weak self] in
        self?.collectionView.reloadSections(IndexSet(integer: 0))
      }
    }
  }
  fileprivate let kMaximumNumberOfElements = 5
  fileprivate var delegate: MWMPlacePageButtonsProtocol?

  func config(delegate d: MWMPlacePageButtonsProtocol?) {
    delegate = d
    collectionView.contentOffset = .zero
    collectionView.delegate = self
    collectionView.dataSource = self
    collectionView.register(cellClass: CianElement.self)
    collectionView.reloadData()

    isSeparatorHidden = true
    backgroundColor = UIColor.clear
  }

  fileprivate func isLastCell(_ indexPath: IndexPath) -> Bool {
    return indexPath.item == collectionView.numberOfItems(inSection: indexPath.section) - 1
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    updateCollectionView(nil)
  }

  private func updateCollectionView(_ updates: (() -> Void)?) {
    guard let sv = superview else { return }
    let layout = collectionView.collectionViewLayout as! UICollectionViewFlowLayout
    let screenSize = { sv.size.width - layout.sectionInset.left - layout.sectionInset.right }
    let itemHeight: CGFloat = 136
    let itemWidth: CGFloat
    if let data = data {
      if data.isEmpty {
        itemWidth = screenSize()
      } else {
        itemWidth = 160
      }
    } else {
      itemWidth = screenSize()
    }
    layout.itemSize = CGSize(width: itemWidth, height: itemHeight)
    collectionView.performBatchUpdates(updates, completion: nil)
  }

  @IBAction
  fileprivate func onMore() {
    delegate?.openSponsoredURL(nil)
  }
}

extension PPCianCarouselCell: UICollectionViewDelegate, UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: CianElement.self,
                                                  indexPath: indexPath) as! CianElement
    if let data = data {
      if data.isEmpty {
        cell.state = .error(onButtonAction: onMore)
      } else {
        let model = isLastCell(indexPath) ? nil : data[indexPath.item]
        cell.state = .offer(model: model,
                            onButtonAction: { [unowned self] model in
                              self.delegate?.openSponsoredURL(model?.pageURL)
                            })
      }
    } else {
      cell.state = .pending(onButtonAction: onMore)
    }
    return cell
  }

  func collectionView(_ collectionView: UICollectionView, numberOfItemsInSection section: Int) -> Int {
    if let data = data {
      if data.isEmpty {
        return 1
      } else {
        return min(data.count, kMaximumNumberOfElements) + 1
      }
    } else {
      return 1
    }
  }
}
