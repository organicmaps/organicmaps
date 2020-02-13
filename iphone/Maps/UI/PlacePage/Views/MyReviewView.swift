final class ReviewRatingView: UIView {
  private let stackView = UIStackView()
  let nameLabel = UILabel()
  let starRatingView = StarRatingView()

  override init(frame: CGRect) {
    super.init(frame: frame)

    stackView.axis = .vertical
    stackView.alignment = .leading
    stackView.spacing = 4
    addSubview(stackView)

    stackView.addArrangedSubview(nameLabel)
    stackView.addArrangedSubview(starRatingView)
    stackView.alignToSuperview()
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

extension ReviewRatingView {
  func defaultConfig() {
    nameLabel.styleName = "regular14:blackSecondaryText"
  }
}

final class MyReviewView: UIView {
  private let stackView = UIStackView()
  private let ratingsStackView = UIStackView()
  private let ratingsContainerView = UIView()
  let reviewView = ReviewView()
  let ratingViews = [ReviewRatingView(), ReviewRatingView(), ReviewRatingView()]
  let separator = UIView()

  override init(frame: CGRect) {
    super.init(frame: frame)
    commonInit()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    commonInit()
  }

  private func commonInit() {
    stackView.axis = .vertical
    stackView.alignment = .fill
    stackView.spacing = 8
    addSubview(stackView)

    ratingsStackView.axis = .horizontal
    ratingsStackView.alignment = .fill
    ratingsStackView.distribution = .fillEqually
    ratingsContainerView.addSubview(ratingsStackView)
    ratingsStackView.alignToSuperview(UIEdgeInsets(top: 0, left: 16, bottom: 0, right: 0))

    stackView.addArrangedSubview(reviewView)
    stackView.addArrangedSubview(ratingsContainerView)

    ratingViews.forEach {
      ratingsStackView.addArrangedSubview($0)
    }

    stackView.alignToSuperview(UIEdgeInsets(top: 0, left: 0, bottom: -16, right: 0))

    separator.translatesAutoresizingMaskIntoConstraints = false
    addSubview(separator)
    NSLayoutConstraint.activate([
      separator.heightAnchor.constraint(equalToConstant: 1),
      separator.leftAnchor.constraint(equalTo: leftAnchor),
      separator.bottomAnchor.constraint(equalTo: bottomAnchor),
      separator.rightAnchor.constraint(equalTo: rightAnchor)
    ])
  }
}

extension MyReviewView {
  func defaultConfig() {
    reviewView.defaultConfig()
    ratingViews.forEach {
      $0.defaultConfig()
    }
    separator.backgroundColor = UIColor.blackDividers()
  }
}
