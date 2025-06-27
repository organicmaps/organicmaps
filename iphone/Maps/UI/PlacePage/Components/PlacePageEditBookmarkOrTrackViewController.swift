protocol PlacePageEditBookmarkOrTrackViewControllerDelegate: AnyObject {
  func didUpdateColor(_ color: UIColor, for data: PlacePageEditData)
  func didUpdateCategory(_ category: String, for data: PlacePageEditData)
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
      expandableLabel.font = UIFont.regular14()
      expandableLabel.textColor = UIColor.blackPrimaryText()
      expandableLabel.numberOfLines = 5
      expandableLabel.expandColor = UIColor.linkBlue()
      expandableLabel.expandText = L("placepage_more_button")
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

      editView.iconButtonTapHandler = {
        ColorPicker.shared.present(from: self, pickerType: .bookmarkColorPicker(bookmarkData.color)) { [weak self] color in
          self?.delegate?.didUpdateColor(color, for: data)
        }
      }

    case .track(let trackData):
      // TODO: pass default color from trackData
      iconColor = trackData.color ?? UIColor.buttonRed()
      category = trackData.trackCategory
      description = trackData.trackDescription
      isHtmlDescription = false

      editView.iconButtonTapHandler = {
        ColorPicker.shared.present(from: self, pickerType: .defaultColorPicker(iconColor)) { [weak self] color in
          self?.delegate?.didUpdateColor(color, for: data)
        }
      }
    }

    let editColorImage = circleImageForColor(iconColor, frameSize: 28, diameter: 22, iconName: "ic_bm_none")
    editView.iconButton.setImage(editColorImage, for: .normal)
    editView.infoLabel.text = category
    editView.setStyle(.link)
    editView.infoLabelTapHandler = {
      print("Edit bookmark tapped")
    }
    editView.setAccessory(image: UIImage(resource: .ic24PxEdit), tapHandler: {
      print("Accessory tapped")
    })

    if let description, !description.isEmpty {
      expandableLabelContainer.isHidden = false
      if isHtmlDescription {
        setHtmlDescription(description)
      } else {
        expandableLabel.text = description
      }
    } else {
      expandableLabelContainer.isHidden = true
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

  // MARK: -  Actions

  @IBAction func onEdit(_ sender: UIButton) {
    guard let data else { return }
    delegate?.didPressEdit(data)
  }
}
