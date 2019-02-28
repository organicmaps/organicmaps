import UIKit

final class ExpandableReviewView: UIView {
  var contentLabel: UILabel = {
    let label = UILabel(frame: .zero)
    label.numberOfLines = 0
    label.clipsToBounds = true
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  var moreLabel: UILabel = {
    let label = UILabel(frame: .zero)
    label.clipsToBounds = true
    label.translatesAutoresizingMaskIntoConstraints = false
    return label
  }()
  
  var moreZeroHeight: NSLayoutConstraint!
  private var settings: ExpandableReviewSettings = ExpandableReviewSettings()
  private var isExpanded = false
  private var onUpdateHandler: (() -> Void)?
  
  override init(frame: CGRect) {
    super.init(frame: frame)
    configureContent()
  }
  
  required init?(coder aDecoder: NSCoder) {
    super.init(coder: aDecoder)
    configureContent()
  }
  
  override func layoutSubviews() {
    super.layoutSubviews()
    updateRepresentation()
  }
  
  func configureContent() {
    self.addSubview(contentLabel)
    self.addSubview(moreLabel)
    let labels: [String: Any] = ["contentLabel": contentLabel, "moreLabel": moreLabel]
    var contentConstraints: [NSLayoutConstraint] = []
    let verticalConstraints = NSLayoutConstraint.constraints(withVisualFormat: "V:|-0-[contentLabel]-0-[moreLabel]",
                                                             metrics: nil,
                                                             views: labels)
    contentConstraints += verticalConstraints
    let moreBottomConstraint = bottomAnchor.constraint(equalTo: moreLabel.bottomAnchor)
    moreBottomConstraint.priority = .defaultLow
    contentConstraints.append(moreBottomConstraint)
    
    let contentHorizontalConstraints = NSLayoutConstraint.constraints(withVisualFormat: "H:|-0-[contentLabel]-0-|",
                                                                      metrics: nil,
                                                                      views: labels)
    contentConstraints += contentHorizontalConstraints
    let moreHorizontalConstraints = NSLayoutConstraint.constraints(withVisualFormat: "H:|-0-[moreLabel]-0-|",
                                                                   metrics: nil,
                                                                   views: labels)
    contentConstraints += moreHorizontalConstraints
    NSLayoutConstraint.activate(contentConstraints)
    moreZeroHeight = moreLabel.heightAnchor.constraint(equalToConstant: 0.0)
    apply(settings: settings)
    
    layer.backgroundColor = UIColor.clear.cgColor
    isOpaque = true
    gestureRecognizers = nil
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onTap)))
  }
  
  func apply(settings: ExpandableReviewSettings) {
    self.settings = settings
    self.contentLabel.textColor = settings.textColor
    self.contentLabel.font = settings.textFont
    self.moreLabel.font = settings.textFont
    self.moreLabel.textColor = settings.expandTextColor
    self.moreLabel.text = settings.expandText
  }

  override func mwm_refreshUI() {
    super.mwm_refreshUI()
    settings.textColor = settings.textColor.opposite()
    settings.expandTextColor = settings.expandTextColor.opposite()
  }
  
  func configure(text: String, isExpanded: Bool, onUpdate: @escaping () -> Void) {
    contentLabel.text = text
    self.isExpanded = isExpanded
    contentLabel.numberOfLines = isExpanded ? 0 : settings.numberOfCompactLines
    onUpdateHandler = onUpdate
  }

  @objc private func onTap() {
    if !isExpanded {
      isExpanded = true
      updateRepresentation()
      contentLabel.numberOfLines = 0
      onUpdateHandler?()
    }
  }
  
  func updateRepresentation() {
    guard let text = contentLabel.text else {
      return
    }
    if isExpanded {
      moreZeroHeight.isActive = true
    } else {
      let height = (text as NSString).boundingRect(with: CGSize(width: contentLabel.bounds.width,
                                                                height: .greatestFiniteMagnitude),
                                                   options: .usesLineFragmentOrigin,
                                                   attributes: [.font: contentLabel.font],
                                                   context: nil).height
      if height > contentLabel.bounds.height {
        moreZeroHeight.isActive = false
      } else {
        moreZeroHeight.isActive = true
      }
    }
  }
}
