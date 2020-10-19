class GuideDescriptionViewController: MWMViewController {
  @IBOutlet private var photo: UIImageView!
  @IBOutlet private var photoViewContainer: UIView!
  @IBOutlet private var photoActivityIndicator: UIActivityIndicatorView!
  @IBOutlet private var titleLabel: UILabel!
  @IBOutlet private var subtitleLabel: UILabel!
  @IBOutlet private var providerContainerView: UIView!
  @IBOutlet private var providerIcon: UIImageView!
  @IBOutlet private var providerLabel: UILabel!
  @IBOutlet private var descriptionText: UITextView!

  private let category: BookmarkGroup

  init(category: BookmarkGroup) {
    self.category = category
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    title = category.title
    titleLabel.text = category.title
    providerLabel.text = String(coreFormat: L("content_by_component"), arguments: [category.author])
    providerContainerView.isHidden = category.author.isEmpty
    subtitleLabel.attributedText = NSMutableAttributedString(htmlString: category.annotation, baseFont: UIFont.regular16())
    descriptionText.attributedText = NSMutableAttributedString(htmlString: category.detailedAnnotation, baseFont: UIFont.regular16())

    if let photoUrl = category.imageUrl {
      photo.wi_setImage(with: photoUrl, transitionDuration: 0) { [weak self] _, error in
        if error != nil {
          self?.photoViewContainer.isHidden = true
        }
        self?.photoActivityIndicator.stopAnimating()
      }
    } else {
      photoViewContainer.isHidden = true
      photoActivityIndicator.stopAnimating()
    }

    providerIcon.isHidden = !category.isLonelyPlanet
  }
}
