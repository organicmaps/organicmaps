protocol PlacePageEditBookmarkOrTrackViewControllerDelegate: AnyObject {
  func didPressEdit(_ data: PlacePageEditData)
}

enum PlacePageEditData {
  case bookmark(PlacePageBookmarkData)
  case track(PlacePageTrackData)
}

final class PlacePageEditBookmarkOrTrackViewController: UIViewController {

  @IBOutlet var stackView: UIStackView!
  @IBOutlet var editView: InfoItemView!
  @IBOutlet var topConstraint: NSLayoutConstraint!
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
    switch data {
    case .bookmark(let bookmark):

      editView.isHidden = false
      editView.imageView.image = UIImage(resource: .icFolder)
      editView.infoLabel.text = bookmark.bookmarkCategory
      editView.setStyle(.link)
      editView.tapHandler = {
        print("Edit bookmark tapped")
      }
      let accessoryImage = circleImageForColor(bookmark.color.color, frameSize: 28, diameter: 22, iconName: "ic_bm_none")
      editView.setAccessory(image: accessoryImage, tapHandler: {
        print("Accessory tapped")
      })

      if let description = bookmark.bookmarkDescription {
        if bookmark.isHtmlDescription {
          setHtmlDescription(description)
          topConstraint.constant = 16
        } else {
          expandableLabel.text = description
          topConstraint.constant = description.count > 0 ? 16 : 0
        }
      } else {
        topConstraint.constant = 0
      }
    case .track:

      // TODO: implement track editing

      expandableLabel.isHidden = true
      topConstraint.constant = 0
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
