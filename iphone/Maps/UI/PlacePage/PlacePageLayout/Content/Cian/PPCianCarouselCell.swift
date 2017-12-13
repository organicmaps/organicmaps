import MyTrackerSDK

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
  @objc var data: [CianItemModel]? {
    didSet {
      updateCollectionView { [weak self] in
        self?.collectionView.reloadSections(IndexSet(integer: 0))
      }
    }
  }

  fileprivate let kMaximumNumberOfElements = 5
  fileprivate var delegate: MWMPlacePageButtonsProtocol?

  fileprivate var statisticsParameters: [AnyHashable: Any] { return [kStatProvider: kStatCian,
                                                                     kStatPlacement : kStatPlacePage] }

  @objc func config(delegate d: MWMPlacePageButtonsProtocol?) {
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
    MRMyTracker.trackEvent(withName: "Placepage_SponsoredGallery_LogoItem_selected_Cian.Ru")
    Statistics.logEvent(kStatPlacepageSponsoredLogoSelected, withParameters: statisticsParameters)
    delegate?.openSponsoredURL(nil)
  }
}

extension PPCianCarouselCell: UICollectionViewDelegate, UICollectionViewDataSource {
  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: CianElement.self,
                                                  indexPath: indexPath) as! CianElement
    if let data = data {
      if data.isEmpty {
        cell.state = .error(onButtonAction: { [unowned self] in
          Statistics.logEvent(kStatPlacepageSponsoredError, withParameters: self.statisticsParameters)
          self.delegate?.openSponsoredURL(nil)
        })
      } else {
        let model = isLastCell(indexPath) ? nil : data[indexPath.item]
        var params = statisticsParameters
        params[kStatItem] = indexPath.row + 1
        params[kStatDestination] = kStatExternal
        cell.state = .offer(model: model,
                            onButtonAction: { [unowned self] model in
                              let isMore = model == nil
                              MRMyTracker.trackEvent(withName: isMore ? "Placepage_SponsoredGallery_MoreItem_selected_Cian.Ru" :
                                "Placepage_SponsoredGallery_ProductItem_selected_Cian.Ru")
                              Statistics.logEvent(isMore ? kStatPlacepageSponsoredMoreSelected : kStatPlacepageSponsoredItemSelected,
                                                  withParameters: params)
                              self.delegate?.openSponsoredURL(model?.pageURL)
        })
      }
    } else {
      cell.state = .pending(onButtonAction: { [unowned self] in
        MRMyTracker.trackEvent(withName: "Placepage_SponsoredGallery_MoreItem_selected_Cian.Ru")
        Statistics.logEvent(kStatPlacepageSponsoredMoreSelected, withParameters: self.statisticsParameters)
        self.delegate?.openSponsoredURL(nil)
      })
    }
    return cell
  }

  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
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
