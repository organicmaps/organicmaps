protocol WikiDescriptionViewControllerDelegate: AnyObject {
  func didPressMore()
}

class WikiDescriptionViewController: UIViewController {
  @IBOutlet var descriptionTextView: UITextView!
  @IBOutlet var moreButton: UIButton!

  var descriptionHtml: String? {
    didSet{
      updateDescription()
    }
  }
  weak var delegate: WikiDescriptionViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    descriptionTextView.textContainerInset = .zero
    updateDescription()
    
    let longPressGesture = UILongPressGestureRecognizer(target: self, action: #selector(handleLongPress))
      descriptionTextView.addGestureRecognizer(longPressGesture)
  }
  
  @objc func handleLongPress(sender: UILongPressGestureRecognizer) {
    if sender.state == .began {
      UIPasteboard.general.string = descriptionTextView.text
      Toast.toast(withText: "Copied!").show()
    }
  }

  private func updateDescription() {
    guard let descriptionHtml = descriptionHtml else { return }

    DispatchQueue.global().async {
      let font = UIFont.regular14()
      let color = UIColor.blackPrimaryText()
      let paragraphStyle = NSMutableParagraphStyle()
      paragraphStyle.lineSpacing = 4

      let attributedString: NSAttributedString
      if let str = NSMutableAttributedString(htmlString: descriptionHtml, baseFont: font, paragraphStyle: paragraphStyle) {
        str.addAttribute(NSAttributedString.Key.foregroundColor,
                         value: color,
                         range: NSRange(location: 0, length: str.length))
        attributedString = str;
      } else {
        attributedString = NSAttributedString(string: descriptionHtml,
                                              attributes: [NSAttributedString.Key.font : font,
                                                           NSAttributedString.Key.foregroundColor: color,
                                                           NSAttributedString.Key.paragraphStyle: paragraphStyle])
      }

      DispatchQueue.main.async {
        if attributedString.length > 500 {
          self.descriptionTextView.attributedText = attributedString.attributedSubstring(from: NSRange(location: 0,
                                                                                                       length: 500))
        } else {
          self.descriptionTextView.attributedText = attributedString
        }
      }
    }
  }

  @IBAction func onMore(_ sender: UIButton) {
    delegate?.didPressMore()
  }
  
  override func applyTheme() {
    super.applyTheme()
    updateDescription()
  }
}
