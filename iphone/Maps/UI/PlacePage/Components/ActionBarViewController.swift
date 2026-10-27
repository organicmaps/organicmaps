protocol ActionBarViewControllerDelegate: AnyObject {
  func actionBar(_ actionBar: ActionBarViewController, didPressButton type: ActionBarButtonType)
}

final class ActionBarViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!
  private(set) var downloadButton: ActionBarButton?

  var placePageData: PlacePageData!
  var canAddStop = false
  var canReplaceStop = false
  var shouldAppendNewPoints = false

  private var visibleButtons: [ActionBarButtonType] = []
  private var additionalButtons: [ActionBarButtonType] = []
  private var canRouteToAndFrom: Bool { !canReplaceStop && !shouldAppendNewPoints }

  weak var delegate: ActionBarViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()
    configureButtons()
  }

  // MARK: - Private methods

  private func configureButtons() {
    if placePageData.isRoutePoint {
      visibleButtons.append(.routeRemoveStop)
    } else if placePageData.roadType != .none {
      let options = RoutingOptions(routerType: MWMRouter.type())
      switch placePageData.roadType {
      case .toll:
        if options.supportedOptions.contains(.toll) {
          visibleButtons.append(.avoidToll)
        }
      case .ferry:
        if options.supportedOptions.contains(.ferry) {
          visibleButtons.append(.avoidFerry)
        }
      case .dirty:
        if options.supportedOptions.contains(.dirty) {
          visibleButtons.append(.avoidDirty)
        }
      default:
        fatalError()
      }
    } else {
      configDefaultButtons()
    }

    setupButtonsState()
  }

  private func configDefaultButtons() {
    if placePageData.objectType != .trackRecording, let mapNodeAttributes = placePageData.mapNodeAttributes {
      switch mapNodeAttributes.nodeStatus {
      case .onDiskOutOfDate, .onDisk, .undefined:
        break
      case .downloading, .applying, .inQueue, .error, .notDownloaded, .partly:
        visibleButtons.append(.download)
      @unknown default:
        fatalError()
      }
    }

    var buttons: [ActionBarButtonType] = []
    let routePointButton: ActionBarButtonType = canReplaceStop ? .routeReplaceStop :
      (shouldAppendNewPoints ? .routeAddDestination : .routeAddStop)
    switch placePageData.objectType {
    case .POI, .bookmark, .track, .relationTrack:
      if canRouteToAndFrom {
        buttons.append(.routeFrom)
      }
      if canReplaceStop || shouldAppendNewPoints || canAddStop {
        buttons.append(routePointButton)
      }
      buttons.append(.bookmark)
      if canRouteToAndFrom {
        buttons.append(.routeTo)
      }
      if placePageData.objectType == .track {
        buttons.append(.track)
      }
    case .trackRecording:
      buttons.append(.deleteTrackRecording)
      buttons.append(.saveTrackRecording)
    @unknown default:
      fatalError()
    }

    guard !buttons.isEmpty else { return }
    let maxVisibleButtons = 5 - visibleButtons.count
    if buttons.count > maxVisibleButtons {
      visibleButtons.append(contentsOf: buttons.prefix(maxVisibleButtons - 1))
      visibleButtons.append(.more)
      additionalButtons.append(contentsOf: buttons.suffix(from: maxVisibleButtons - 1))
    } else {
      visibleButtons.append(contentsOf: buttons)
    }
  }

  private func setupButtonsState() {
    // Download takes priority over routing actions.
    let primaryButton: ActionBarButtonType? = [.download, .routeTo, .routeAddDestination, .routeReplaceStop]
      .first { visibleButtons.contains($0) }
    for buttonType in visibleButtons {
      let selected = isButtonSelected(buttonType)
      let button = ActionBarButton(delegate: self,
                                   buttonType: buttonType,
                                   isSelected: selected,
                                   isPrimary: buttonType == primaryButton)
      stackView.addArrangedSubview(button)
      switch buttonType {
      case .download:
        downloadButton = button
        updateDownloadButtonState(placePageData.mapNodeAttributes!.nodeStatus)
      default:
        break
      }
    }
  }

  private func isButtonSelected(_ buttonType: ActionBarButtonType) -> Bool {
    switch buttonType {
    case .bookmark:
      return placePageData.bookmarkData != nil
    case .track:
      return placePageData.trackData != nil
    default:
      return false
    }
  }

  private func showMore() {
    let actionSheet = UIAlertController(title: placePageData.previewData.title,
                                        message: placePageData.previewData.subtitle,
                                        preferredStyle: .actionSheet)
    for button in additionalButtons {
      let selected = isButtonSelected(button)
      let action = UIAlertAction(title: titleForButton(button, selected),
                                 style: .default,
                                 handler: { [weak self] _ in
                                   guard let self = self else { return }
                                   self.delegate?.actionBar(self, didPressButton: button)
                                 })
      actionSheet.addAction(action)
    }
    actionSheet.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    if let popover = actionSheet.popoverPresentationController, let sourceView = stackView.arrangedSubviews.last {
      popover.sourceView = sourceView
      popover.sourceRect = sourceView.bounds
    }
    present(actionSheet, animated: true)
  }

  // MARK: - Public methods

  func resetButtons() {
    for arrangedSubview in stackView.arrangedSubviews {
      stackView.removeArrangedSubview(arrangedSubview)
      arrangedSubview.removeFromSuperview()
    }
    visibleButtons.removeAll()
    additionalButtons.removeAll()
    downloadButton = nil
    configureButtons()
  }

  func updateDownloadButtonState(_: MapNodeStatus) {
    guard let downloadButton = downloadButton, let mapNodeAttributes = placePageData.mapNodeAttributes else { return }
    switch mapNodeAttributes.nodeStatus {
    case .downloading:
      downloadButton.mapDownloadProgress?.state = .progress
    case .applying, .inQueue:
      downloadButton.mapDownloadProgress?.state = .spinner
    case .error:
      downloadButton.mapDownloadProgress?.state = .failed
    case .onDisk, .undefined, .onDiskOutOfDate:
      downloadButton.mapDownloadProgress?.state = .completed
    case .notDownloaded, .partly:
      downloadButton.mapDownloadProgress?.state = .normal
    @unknown default:
      fatalError()
    }
  }
}

extension ActionBarViewController: ActionBarButtonDelegate {
  func tapOnButton(with type: ActionBarButtonType) {
    switch type {
    case .more:
      showMore()
    default:
      delegate?.actionBar(self, didPressButton: type)
    }
  }
}
