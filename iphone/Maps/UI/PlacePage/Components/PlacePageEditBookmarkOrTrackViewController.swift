protocol PlacePageEditBookmarkOrTrackViewControllerDelegate: AnyObject {
  func didUpdate(color: UIColor, category: MWMMarkGroupID, for data: PlacePageEditData)
  func didPressEdit(_ data: PlacePageEditData)
}

enum PlacePageEditData {
  case bookmark(PlacePageBookmarkData)
  case track(PlacePageTrackData)
}

final class PlacePageEditBookmarkOrTrackViewController: UIViewController {

  @IBOutlet var stackView: UIStackView!
  @IBOutlet var editView: InfoItemView!
  @IBOutlet var expandableLabelContainer: UIView!
  @IBOutlet var expandableLabel: ExpandableLabel! {
    didSet {
      updateExpandableLabelStyle()
    }
  }

  var data: PlacePageEditData? {
    didSet {
      updateViews()
    }
  }
  
  weak var delegate: PlacePageEditBookmarkOrTrackViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    updateViews()
  }

  override func applyTheme() {
    super.applyTheme()
    updateViews()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    if #available(iOS 13.0, *), traitCollection.hasDifferentColorAppearance(comparedTo: previousTraitCollection) {
      applyTheme()
    }
  }

  // MARK: - Private methods

  private func updateViews() {
    guard let data else { return }

    let iconColor: UIColor
    let category: String?
    let description: String?
    let isHtmlDescription: Bool

    switch data {
    case .bookmark(let bookmarkData):
      iconColor = bookmarkData.color.color
      category = bookmarkData.bookmarkCategory
      description = bookmarkData.bookmarkDescription
      isHtmlDescription = bookmarkData.isHtmlDescription
    case .track(let trackData):
      iconColor = trackData.color ?? UIColor.buttonRed()
      category = trackData.trackCategory
      description = trackData.trackDescription
      isHtmlDescription = false
    }

    let editColorImage = circleImageForColor(iconColor, frameSize: 28, diameter: 22, iconName: "ic_bm_none")
    editView.iconButton.setImage(editColorImage, for: .normal)
    editView.infoLabel.text = category
    editView.setStyle(.link)

    editView.iconButtonTapHandler = { [weak self] in
      guard let self else { return }
      self.showColorPicker()
    }
    editView.infoLabelTapHandler = { [weak self] in
      guard let self else { return }
      self.showGroupPicker()
    }
    editView.setAccessory(image: UIImage(resource: .ic24PxEdit), tapHandler: { [weak self] in
      guard let self, let data = self.data else { return }
      self.delegate?.didPressEdit(data)
    })

    if let description, !description.isEmpty {
      expandableLabelContainer.isHidden = false
      if isHtmlDescription {
        setHtmlDescription(description)
      } else {
        expandableLabel.text = description
      }
      updateExpandableLabelStyle()
    } else {
      expandableLabelContainer.isHidden = true
    }
  }

  private func updateExpandableLabelStyle() {
    expandableLabel.font = UIFont.regular14()
    expandableLabel.textColor = UIColor.blackPrimaryText()
    expandableLabel.numberOfLines = 5
    expandableLabel.expandColor = UIColor.linkBlue()
    expandableLabel.expandText = L("placepage_more_button")
  }

  private func showColorPicker() {
    guard let data else { return }
    switch data {
    case .bookmark(let bookmarkData):
      ColorPicker.shared.present(from: self, pickerType: .bookmarkColorPicker(bookmarkData.color)) { [weak self] color in
        self?.update(color: color)
      }
    case .track(let trackData):
      ColorPicker.shared.present(from: self, pickerType: .defaultColorPicker(trackData.color ?? .buttonRed())) { [weak self] color in
        self?.update(color: color)
      }
    }
  }

  private func showGroupPicker() {
    guard let data else { return }
    let groupId: MWMMarkGroupID
    let groupName: String?
    switch data {
    case .bookmark(let bookmarkData):
      groupId = bookmarkData.bookmarkGroupId
      groupName = bookmarkData.bookmarkCategory
    case .track(let trackData):
      groupId = trackData.groupId
      groupName = trackData.trackCategory
    }
    let groupViewController = SelectBookmarkGroupViewController(groupName: groupName ?? "", groupId: groupId)
    let navigationController = UINavigationController(rootViewController: groupViewController)
    groupViewController.delegate = self
    present(navigationController, animated: true, completion: nil)
  }

  private func update(color: UIColor? = nil, category: MWMMarkGroupID? = nil) {
    guard let data else { return }
    switch data {
    case .bookmark(let bookmarkData):
      delegate?.didUpdate(color: color ?? bookmarkData.color.color, category: category ?? bookmarkData.bookmarkGroupId, for: data)
    case .track(let trackData):
      delegate?.didUpdate(color: color ?? trackData.color!, category: category ?? trackData.groupId, for: data)
    }
  }

  private func setHtmlDescription(_ htmlDescription: String) {
    DispatchQueue.global().async {
      let font = UIFont.regular14()
      let color = UIColor.blackPrimaryText()
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineSpacing = 4

      let attributedString: NSAttributedString
      if let str = NSMutableAttributedString(htmlString: htmlDescription, baseFont: font, paragraphStyle: paragraphStyle) {
        str.addAttribute(NSAttributedString.Key.foregroundColor,
                         value: color,
                         range: NSRange(location: 0, length: str.length))
        attributedString = str;
      } else {
        attributedString = NSAttributedString(string: htmlDescription,
                                              attributes: [NSAttributedString.Key.font : font,
                                                           NSAttributedString.Key.foregroundColor: color,
                                                           NSAttributedString.Key.paragraphStyle: paragraphStyle])
      }

      DispatchQueue.main.async {
        self.expandableLabel.attributedText = attributedString
      }
    }
  }
}

// MARK: - SelectBookmarkGroupViewControllerDelegate
extension PlacePageEditBookmarkOrTrackViewController: SelectBookmarkGroupViewControllerDelegate {
  func bookmarkGroupViewController(_ viewController: SelectBookmarkGroupViewController,
                                   didSelect groupTitle: String,
                                   groupId: MWMMarkGroupID) {
    viewController.dismiss(animated: true)
    update(category: groupId)
  }
}
