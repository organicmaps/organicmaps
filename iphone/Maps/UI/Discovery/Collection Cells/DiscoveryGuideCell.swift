@objc(MWMDiscoveryGuideCell)
final class DiscoveryGuideCell: UICollectionViewCell {
  @IBOutlet var avatar: UIImageView!
  @IBOutlet var titleLabel: UILabel! {
    didSet {
      titleLabel.numberOfLines = 2
    }
  }
  
  @IBOutlet var subtitleLabel: UILabel! {
    didSet {
      subtitleLabel.numberOfLines = 1
    }
  }
  
  @IBOutlet var proLabel: UILabel! {
    didSet {
      proLabel.text = "";
    }
  }
  
  @IBOutlet var proContainer: UIView!
  
  @IBOutlet var detailsButton: UIButton! {
    didSet {
      detailsButton.setTitle(L("details"), for: .normal)
    }
  }
  
  typealias OnDetails = () -> Void
  private var onDetails: OnDetails?
  
  override var isHighlighted: Bool {
    didSet {
      UIView.animate(withDuration: kDefaultAnimationDuration,
                     delay: 0,
                     options: [.allowUserInteraction, .beginFromCurrentState],
                     animations: { self.alpha = self.isHighlighted ? 0.3 : 1 },
                     completion: nil)
    }
  }
  
  override func awakeFromNib() {
    super.awakeFromNib()
  }
  
  override func prepareForReuse() {
    super.prepareForReuse()
    avatar.image = UIImage(named: "img_guide_placeholder")
    titleLabel.text = ""
    subtitleLabel.text = ""
    proLabel.text = ""
    proContainer.isHidden = true
    onDetails = nil
  }
  
  private func setAvatar(_ avatarURL: String?) {
    guard let avatarURL = avatarURL else { return }
    if !avatarURL.isEmpty, let url = URL(string: avatarURL) {
      avatar.image = UIImage(named: "img_guide_placeholder")
      avatar.wi_setImage(with: url, transitionDuration: kDefaultAnimationDuration)
    } else {
      avatar.image = UIImage(named: "img_guide_placeholder")
    }
  }
  
  @objc func config(avatarURL: String?,
                    title: String,
                    subtitle: String,
                    label: String?,
                    labelHexColor: String?,
                    onDetails: @escaping OnDetails) {
    setAvatar(avatarURL)
    titleLabel.text = title
    subtitleLabel.text = subtitle
    self.onDetails = onDetails
    guard let label = label, !label.isEmpty else {
      proContainer.isHidden = true
      return
    }
    proLabel.text = label
    if let labelHexColor = labelHexColor, labelHexColor.count == 6 {
      proContainer.backgroundColor = UIColor(fromHexString: labelHexColor) 
    } else {
      proContainer.backgroundColor = UIColor.red
    }
    proContainer.isHidden = false
  }
  
  @IBAction private func detailsAction() {
    onDetails?()
  }
}
