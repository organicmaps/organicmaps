protocol PlacePageHeaderViewProtocol: AnyObject {
  var presenter: PlacePageHeaderPresenterProtocol? { get set }
  var didStartEditingTitle: ((Bool) -> Void)? { get set }
  var didChangeEditedTitle: (() -> Void)? { get set }

  func setShadowHidden(_ hidden: Bool)
  func setTitle(_ title: String?, secondaryTitle: String?)
  func showShareTrackMenu()
}

final class PlacePageHeaderViewController: UIViewController {
  private enum Constants {
    static var titleFont: UIFont { UIFont.medium20.dynamic }
    static let didShowEducationalTrackSelectorPopup = "PlacePageHeaderViewController_didShowEducationalTrackSelectorPopup"
    static let educationalTrackSelectorPopupTimeout = 0.3
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
  @IBOutlet private var trackCandidatesButton: UIButton!
  @IBOutlet private var cancelButton: UIButton!
  private var titleText: String?
  private var subtitleText: String?
  private weak var trackCandidatesSelectorViewController: PopoverListSelectorViewController?

  var presenter: PlacePageHeaderPresenterProtocol?
  var isEditingTitle: Bool = false {
    didSet {
      didStartEditingTitle?(isEditingTitle)
    }
  }

  var didStartEditingTitle: ((Bool) -> Void)?
  var didChangeEditedTitle: (() -> Void)?

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
    shareButton.isHidden = !(presenter?.canShare ?? true)

    trackCandidatesButton.tintColor = .linkBlue
    trackCandidatesButton.isHidden = !(presenter?.canSelectTrackCandidates ?? false)
    trackCandidatesButton.addTarget(self, action: #selector(didTapTrackCandidatesButton), for: .touchUpInside)

    cancelButton.setStyle(.searchCancelButton)
    cancelButton.setTitle(L("cancel"), for: .normal)
    cancelButton.titleLabel?.numberOfLines = 1
    cancelButton.titleLabel?.minimumScaleFactor = 0.5
    cancelButton.titleLabel?.adjustsFontSizeToFitWidth = true
    cancelButton.addTarget(self, action: #selector(didTapCancelButton), for: .touchUpInside)
    cancelButton.isHidden = true

    titleTextView.font = Constants.titleFont
    titleTextView.adjustsFontForContentSizeCategory = true
    titleTextView.isEditable = presenter?.canEditTitle ?? false
    titleTextView.isScrollEnabled = false
    titleTextView.backgroundColor = .clear
    titleTextView.textContainerInset = .zero
    titleTextView.textContainer.lineFragmentPadding = .zero
    titleTextView.delegate = self

    let longTapGesture = UILongPressGestureRecognizer(target: self, action: #selector(didLongPressTitleTextView(_:)))
    titleTextView.addGestureRecognizer(longTapGesture)

    clearTitleTextButton.setImage(UIImage.icClear.withRenderingMode(.alwaysTemplate), style: GlobalStyleSheet.gray)
    clearTitleTextButton.isHidden = true
    clearTitleTextButton.addTarget(self, action: #selector(didTapClearTitleButton), for: .touchUpInside)

    subtitleLabel.font = .medium16.dynamic
    subtitleLabel.numberOfLines = 0
    subtitleLabel.adjustsFontForContentSizeCategory = true

    if presenter?.objectType == .track, presenter?.canShare == true {
      configureTrackSharingMenu()
    }
  }

  override func viewDidAppear(_ animated: Bool) {
    super.viewDidAppear(animated)

    if presenter?.canSelectTrackCandidates == true,
       !UserDefaults.standard.bool(forKey: Constants.didShowEducationalTrackSelectorPopup) {
      // Wait for the place page presentation/expansion animation before showing the educational popover.
      DispatchQueue.main.asyncAfter(deadline: .now() + Constants.educationalTrackSelectorPopupTimeout) { [weak self] in
        self?.showEducationalTrackCandidatesSelectorIfNeeded()
      }
    }
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // Popovers with adaptivePresentationStyle == .none should keep the same appearance as the presenting view.
    trackCandidatesSelectorViewController?.overrideUserInterfaceStyle = traitCollection.userInterfaceStyle
    if previousTraitCollection?.preferredContentSizeCategory != traitCollection.preferredContentSizeCategory {
      updateTitleEditingStyle()
    }
  }

  @objc private func onExpandPressed(sender _: UITapGestureRecognizer) {
    presenter?.onExpandPress()
  }

  @objc private func didTapClearTitleButton() {
    titleTextView.text = ""
    clearTitleTextButton.isHidden = true
  }

  @objc private func didTapCancelButton() {
    resetTitleEditing()
  }

  @objc private func didTapTrackCandidatesButton() {
    showTrackCandidatesSelector()
  }

  @objc private func didLongPressTitleTextView(_ sender: UILongPressGestureRecognizer) {
    guard sender.state == .began else { return }
    presenter?.onCopy(titleTextView.text)
  }

  @IBAction private func onCloseButtonPressed(_: Any) {
    presenter?.onClosePress()
  }

  @IBAction private func onShareButtonPressed(_: Any) {
    presenter?.onShareButtonPress(from: shareButton)
  }

  private func resetTitleEditing() {
    titleTextView.text = titleText
    titleTextView.resignFirstResponder()
    updateTitleEditingStyle()
  }

  private func showEducationalTrackCandidatesSelectorIfNeeded() {
    let key = Constants.didShowEducationalTrackSelectorPopup
    guard !UserDefaults.standard.bool(forKey: key) else { return }
    guard showTrackCandidatesSelector() else { return }
    UserDefaults.standard.set(true, forKey: key)
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
      .font: Constants.titleFont,
      .foregroundColor: UIColor.blackPrimaryText,
    ]
    let editImage = NSTextAttachment()
    editImage.image = UIImage(resource: .ic24PxEdit)
    let editImageHeight = Constants.titleFont.pointSize * 0.7
    let editImageRect = CGRect(x: 0, y: -(editImageHeight / 4), width: editImageHeight, height: editImageHeight)
    editImage.bounds = editImageRect
    let editString = NSMutableAttributedString(attachment: editImage)
    editString.addAttributes([.foregroundColor: UIColor.linkBlue],
                             range: NSRange(location: 0, length: editString.length))

    let titleString = NSMutableAttributedString(string: titleText ?? "", attributes: titleAttributes)
    if presenter?.canEditTitle == true, !isEditingTitle {
      titleString.append(NSAttributedString(string: " "))
      titleString.append(editString)
    }
    titleTextView.attributedText = titleString
    subtitleLabel.text = subtitleText
  }

  func showShareTrackMenu() {
    // The menu will be shown by the shareButton itself
  }

  private func configureTrackSharingMenu() {
    let menu = UIMenu(title: "", image: nil, children: [
      UIAction(title: L("export_file"), image: nil, handler: { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.kml, from: self.shareButton)
      }),
      UIAction(title: L("export_file_gpx"), image: nil, handler: { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.gpx, from: self.shareButton)
      }),
      UIAction(title: L("export_file_geojson"), image: nil, handler: { [weak self] _ in
        guard let self else { return }
        self.presenter?.onExportTrackButtonPress(.geoJson, from: self.shareButton)
      }),
    ])
    shareButton.menu = menu
    shareButton.showsMenuAsPrimaryAction = true
  }

  @discardableResult
  private func showTrackCandidatesSelector() -> Bool {
    guard let presenter, presenter.canSelectTrackCandidates,
          view.window != nil,
          !trackCandidatesButton.isHidden,
          presentedViewController == nil,
          trackCandidatesSelectorViewController == nil else {
      return false
    }

    let popoverDataSource = presenter.trackSelectionCandidates.map { candidate in
      PopoverListSelectorViewController.RowViewModel(
        title: .string(candidate.title),
        color: candidate.color,
        isSelected: candidate.isSelected,
        selectionHandler: { [weak self] in
          self?.dismiss(animated: true, completion: { [weak self] in
            self?.presenter?.onSelectTrackCandidate(candidate)
          })
        }
      )
    }
    let viewController = PopoverListSelectorBuilder(dataSource: popoverDataSource,
                                                    style: .icon,
                                                    sourceView: trackCandidatesButton,
                                                    sourceRect: trackCandidatesButton.bounds,
                                                    userInterfaceStyle: traitCollection.userInterfaceStyle)
      .build()
    trackCandidatesSelectorViewController = viewController
    present(viewController, animated: true)
    return true
  }
}

// MARK: - UITextViewDelegate

extension PlacePageHeaderViewController: UITextViewDelegate {
  func textViewDidBeginEditing(_: UITextView) {
    isEditingTitle = true
    clearTitleTextButton.isHidden = false
    cancelButton.isHidden = false
    closeButton.isHidden = true
    shareButton.isHidden = true
    trackCandidatesButton.isHidden = true
    updateTitleEditingStyle()
  }

  func textViewDidChange(_ textView: UITextView) {
    clearTitleTextButton.isHidden = textView.text.isEmpty
    didChangeEditedTitle?()
  }

  func textViewDidEndEditing(_ textView: UITextView) {
    clearTitleTextButton.isHidden = true
    cancelButton.isHidden = true
    closeButton.isHidden = false
    shareButton.isHidden = !(presenter?.canShare ?? true)
    trackCandidatesButton.isHidden = !(presenter?.canSelectTrackCandidates ?? false)

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

  func textView(_ textView: UITextView, shouldChangeTextIn _: NSRange, replacementText text: String) -> Bool {
    let isReturnTapped = text == "\n"
    if isReturnTapped {
      textView.resignFirstResponder()
      updateTitleEditingStyle()
      return false
    }
    return true
  }
}
