@objc(MWMUGCYourReviewCell)
final class UGCYourReviewCell: MWMTableViewCell {
  private enum Config {
    static let defaultReviewBottomOffset: CGFloat = 16
  }

  @IBOutlet private weak var titleLabel: UILabel! {
    didSet {
      titleLabel.font = UIFont.bold14()
      titleLabel.textColor = UIColor.blackPrimaryText()
      titleLabel.text = L("placepage_reviews_your_comment")
    }
  }

  @IBOutlet private weak var dateLabel: UILabel! {
    didSet {
      dateLabel.font = UIFont.regular12()
      dateLabel.textColor = UIColor.blackSecondaryText()
    }
  }

  @IBOutlet private weak var reviewLabel: ExpandableReviewView! {
    didSet {
      let settings = ExpandableReviewSettings(expandText: L("placepage_more_button"),
                                              expandTextColor: .linkBlue(),
                                              textColor: .blackPrimaryText(),
                                              textFont: .regular14())
      reviewLabel.apply(settings: settings)
    }
  }

  @IBOutlet private weak var reviewBottomOffset: NSLayoutConstraint!
  @IBOutlet private weak var ratingCollectionView: UICollectionView! {
    didSet {
      ratingCollectionView.register(cellClass: UGCSummaryRatingStarsCell.self)
    }
  }

  private var yourReview: UGCYourReview!

  override var frame: CGRect {
    didSet {
      if frame.size != oldValue.size {
        updateCollectionView()
      }
    }
  }

  @objc func config(yourReview: UGCYourReview, isExpanded: Bool, onUpdate: @escaping () -> Void) {
    dateLabel.text = yourReview.date
    self.yourReview = yourReview
    reviewLabel.configure(text: yourReview.text,
                          isExpanded: isExpanded,
                          onUpdate: onUpdate)
    reviewBottomOffset.constant = yourReview.text.isEmpty ? 0 : Config.defaultReviewBottomOffset
    updateCollectionView()
    isSeparatorHidden = true
  }

  override func didMoveToSuperview() {
    super.didMoveToSuperview()
    updateCollectionView()
  }

  private func updateCollectionView() {
    DispatchQueue.main.async {
      self.ratingCollectionView.reloadData()
    }
  }
}

extension UGCYourReviewCell: UICollectionViewDataSource {
  func collectionView(_: UICollectionView, numberOfItemsInSection _: Int) -> Int {
    return yourReview?.ratings.count ?? 0
  }

  func collectionView(_ collectionView: UICollectionView, cellForItemAt indexPath: IndexPath) -> UICollectionViewCell {
    let cell = collectionView.dequeueReusableCell(withCellClass: UGCSummaryRatingStarsCell.self, indexPath: indexPath) as! UGCSummaryRatingStarsCell
    cell.config(rating: yourReview!.ratings[indexPath.item])
    return cell
  }
}
