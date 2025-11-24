protocol PlacePageHeaderViewProtocol: AnyObject {
  var presenter: PlacePageHeaderPresenterProtocol? { get set }
  var onEditingTitle: ((Bool) -> Void)? { get set }

  func setShadowHidden(_ hidden: Bool)
  func setTitle(_ title: String?, secondaryTitle: String?)
  func showShareTrackMenu()
}

final class PlacePageHeaderViewController: UIViewController {

  private enum Constants {
    static let editImageRect = CGRect(x: 0, y: -2, width: 14, height: 14)
  }

  @IBOutlet private var headerView: PlacePageHeaderView!
  @IBOutlet private var titleTextView: UITextView!
  @IBOutlet private var clearTitleTextButton: CircleImageButton!
  @IBOutlet private var subtitleLabel: UILabel!
  @IBOutlet private var expandView: UIView!
  @IBOutlet private var shadowView: UIView!
  @IBOutlet private var grabberView: UIView!
  @IBOutlet private var closeButton: CircleImageButton!
  @IBOutlet private var shareButton: CircleImageButton!
  @IBOutlet private var cancelButton: UIButton!
  private var titleText: String?
  private var subtitleText: String?

  var presenter: PlacePageHeaderPresenterProtocol?
  var isEditingTitle: Bool = false {
    didSet {
      onEditingTitle?(isEditingTitle)
    }
  }
  var onEditingTitle: ((Bool) -> Void)?

  override func viewDidLoad() {
    super.viewDidLoad()
    presenter?.configure()

    let tap = UITapGestureRecognizer(target: self, action: #selector(onExpandPressed(sender:)))
    expandView.addGestureRecognizer(tap)
    headerView.layer.maskedCorners = [.layerMinXMinYCorner, .layerMaxXMinYCorner]
    iPadSpecific { [weak self] in
      self?.grabberView.isHidden = true
    }
    closeButton.setImage(UIImage(resource: .icClose))
    shareButton.setImage(UIImage(resource: .icShare))

    cancelButton.setStyle(.searchCancelButton)
    cancelButton.setTitle(L("cancel"), for: .normal)
    cancelButton.titleLabel?.numberOfLines = 1
    cancelButton.titleLabel?.minimumScaleFactor = 0.5
    cancelButton.titleLabel?.adjustsFontSizeToFitWidth = true
    cancelButton.addTarget(self, action: #selector(didTapCancelButton), for: .touchUpInside)
    cancelButton.isHidden = true

    titleTextView.font = StyleManager.shared.theme!.fonts.medium20
    titleTextView.isEditable = presenter?.canEditTitle ?? false
    titleTextView.isScrollEnabled = false
    titleTextView.backgroundColor = .clear
    titleTextView.textContainerInset = .zero
    titleTextView.textContainer.lineFragmentPadding = .zero
    titleTextView.delegate = self

    let longTapGesture = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressTitleTextView(_:)))
    titleTextView.addGestureRecognizer(longTapGesture)

    clearTitleTextButton.setImage(UIImage(resource: .icClear), style: GlobalStyleSheet.gray)
    clearTitleTextButton.isHidden = true
    clearTitleTextButton.addTarget(self, action: #selector(didTapClearTitleButton), for: .touchUpInside)

    subtitleLabel.font = StyleManager.shared.theme!.fonts.medium16

    if presenter?.objectType == .track {
      configureTrackSharingMenu()
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    guard traitCollection.userInterfaceStyle != previousTraitCollection?.userInterfaceStyle else { return }
    updateTitleEditingStyle()
  }

  @objc private func onExpandPressed(sender: UITapGestureRecognizer) {
    presenter?.onExpandPress()
  }

  @objc private func didTapClearTitleButton() {
    titleTextView.text = ""
    clearTitleTextButton.isHidden = true
  }

  @objc private func didTapCancelButton() {
    resetTitleEditing()
  }

  @objc private func didLongPressTitleTextView(_ sender: UILongPressGestureRecognizer) {
    guard sender.state == .began else { return }
    presenter?.onCopy(titleTextView.text)
  }

  @IBAction private func onCloseButtonPressed(_ sender: Any) {
    presenter?.onClosePress()
  }

  @IBAction private func onShareButtonPressed(_ sender: Any) {
    presenter?.onShareButtonPress(from: shareButton)
  }

  private func resetTitleEditing() {
    titleTextView.text = titleText
    titleTextView.resignFirstResponder()
    updateTitleEditingStyle()
  }
}

// MARK: - PlacePageHeaderViewProtocol

extension PlacePageHeaderViewController: PlacePageHeaderViewProtocol {
  func setShadowHidden(_ hidden: Bool) {
    shadowView.isHidden = hidden
  }

  func setTitle(_ title: String?, secondaryTitle: String?) {
    switch (title, secondaryTitle) {
    case (nil, nil):
      subtitleLabel.isHidden = true
    case (nil, let subtitle):
      titleText = subtitle
      subtitleLabel.isHidden = true
    case (let title, nil):
      titleText = title
      subtitleLabel.isHidden = true
    case (let title, let subtitle):
      titleText = title
      subtitleText = subtitle
    }
    updateTitleEditingStyle()
  }

  private func updateTitleEditingStyle() {
    let titleAttributes: [NSAttributedString.Key: Any] = [
      .font: StyleManager.shared.theme!.fonts.medium20,
      .foregroundColor: UIColor.blackPrimaryText()
    ]
    let editImage = NSTextAttachment()
    editImage.image = UIImage(resource: .ic24PxEdit)
    editImage.bounds = Constants.editImageRect
    let editString = NSMutableAttributedString(attachment: editImage)
    editString.addAttributes([.foregroundColor: UIColor.linkBlue()],
                             range: NSRange(location: 0, length: editString.length))

    let titleString = NSMutableAttributedString(string: titleText ?? "", attributes: titleAttributes)
    if presenter?.canEditTitle == true && !isEditingTitle {
      titleString.append(NSAttributedString(string: " "))
      titleString.append(editString)
    }
    titleTextView.attributedText = titleString
    subtitleLabel.text = subtitleText
  }

  func showShareTrackMenu() {
    if #available(iOS 14.0, *) {
      // The menu will be shown by the shareButton itself
    } else {
      let alert = UIAlertController(title: nil, message: nil, preferredStyle: .actionSheet)
      let kmlAction = UIAlertAction(title: L("export_file"), style: .default) { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.text, from: self.shareButton)
      }
      let gpxAction = UIAlertAction(title: L("export_file_gpx"), style: .default) { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.gpx, from: self.shareButton)
      }
      alert.addAction(kmlAction)
      alert.addAction(gpxAction)
      present(alert, animated: true, completion: nil)
    }
  }

  private func configureTrackSharingMenu() {
    if #available(iOS 14.0, *) {
      let menu = UIMenu(title: "", image: nil, children: [
        UIAction(title: L("export_file"), image: nil, handler: { [weak self] _ in
          guard let self else { return }
          self.presenter?.onExportTrackButtonPress(.text, from: self.shareButton)
        }),
        UIAction(title: L("export_file_gpx"), image: nil, handler: { [weak self] _ in
          guard let self else { return }
          self.presenter?.onExportTrackButtonPress(.gpx, from: self.shareButton)
        }),
      ])
      shareButton.menu = menu
      shareButton.showsMenuAsPrimaryAction = true
    }
  }
}

// MARK: - UITextViewDelegate

extension PlacePageHeaderViewController: UITextViewDelegate {
  func textViewDidBeginEditing(_ textView: UITextView) {
    isEditingTitle = true
    clearTitleTextButton.isHidden = false
    cancelButton.isHidden = false
    closeButton.isHidden = true
    shareButton.isHidden = true
    updateTitleEditingStyle()
  }

  func textViewDidChange(_ textView: UITextView) {
    clearTitleTextButton.isHidden = textView.text.isEmpty
  }

  func textViewDidEndEditing(_ textView: UITextView) {
    clearTitleTextButton.isHidden = true
    cancelButton.isHidden = true
    closeButton.isHidden = false
    shareButton.isHidden = false

    isEditingTitle = false
    let cleanedText = textView.text
      .trimmingCharacters(in: .whitespacesAndNewlines)
      .replacingOccurrences(of: "\\R", with: "", options: .regularExpression)
    guard cleanedText != titleText else { return }
    if cleanedText.isEmpty {
      resetTitleEditing()
    } else {
      presenter?.onFinishEditingTitle(cleanedText)
    }
  }

  func textView(_ textView: UITextView, shouldChangeTextIn range: NSRange, replacementText text: String) -> Bool {
    let isReturnTapped = text == "\n"
    if (isReturnTapped) {
      textView.resignFirstResponder()
      updateTitleEditingStyle()
      return false
    }
    return true
  }
}
