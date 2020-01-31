protocol ActionBarViewControllerDelegate: AnyObject {
  func actionBarDidPressButton(_ type: ActionBarButtonType)
}

class ActionBarViewController: UIViewController {
  @IBOutlet var stackView: UIStackView!
  
  var placePageData: PlacePageData!
  var isRoutePlanning = false
  var canAddStop = false

  private var visibleButtons: [ActionBarButtonType] = []
  private var additionalButtons: [ActionBarButtonType] = []

  weak var delegate: ActionBarViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    configButton1()
    configButton2()
    configButton3()
    configButton4()

    for buttonType in visibleButtons {
      var selected = false
      var disabled = false
      if buttonType == .bookmark {
        if let bookmarkData = placePageData.bookmarkData {
          selected = true
          disabled = !bookmarkData.isEditable
        }
      }
      guard let button = ActionBarButton(delegate: self,
                                         buttonType: buttonType,
                                         partnerIndex: placePageData.partnerIndex,
                                         isSelected: selected,
                                         isDisabled: disabled) else { continue }
      stackView.addArrangedSubview(button)
    }
  }

  private func configButton1() {
    var buttons: [ActionBarButtonType] = []
    if isRoutePlanning {
      buttons.append(.routeFrom)
    }
    if placePageData.previewData.isBookingPlace {
      buttons.append(.booking)
    }
    if placePageData.isPartner {
      buttons.append(.partner)
    }
    if placePageData.bookingSearchUrl != nil {
      buttons.append(.bookingSearch)
    }
    if placePageData.infoData?.phone != nil, AppInfo.shared().canMakeCalls {
      buttons.append(.call)
    }
    if !isRoutePlanning {
      buttons.append(.routeFrom)
    }

    assert(buttons.count > 0)
    visibleButtons.append(buttons[0])
    if buttons.count > 1 {
      additionalButtons.append(contentsOf: buttons.suffix(from: 1))
    }
  }

  private func configButton2() {
    var buttons: [ActionBarButtonType] = []
    if canAddStop {
      buttons.append(.routeAddStop)
    }
    buttons.append(.bookmark)

    assert(buttons.count > 0)
    visibleButtons.append(buttons[0])
    if buttons.count > 1 {
      additionalButtons.append(contentsOf: buttons.suffix(from: 1))
    }
  }

  private func configButton3() {
    visibleButtons.append(.routeTo)
  }

  private func configButton4() {
    if additionalButtons.isEmpty {
      visibleButtons.append(.share)
    } else {
      additionalButtons.append(.share)
      visibleButtons.append(.more)
    }
  }

  private func showMore() {
    let actionSheet = UIAlertController(title: placePageData.previewData.title,
                                        message: placePageData.previewData.subtitle,
                                        preferredStyle: .actionSheet)
    for button in additionalButtons {
      actionSheet.addAction(UIAlertAction(title: titleForButton(button, placePageData.partnerIndex, false),
                                          style: .default,
                                          handler: { [weak self] _ in
                                            self?.delegate?.actionBarDidPressButton(button)
      }))
    }
    actionSheet.addAction(UIAlertAction(title: L("cancel"), style: .cancel))
    present(actionSheet, animated: true)
  }
}

extension ActionBarViewController: ActionBarButtonDelegate {
  func tapOnButton(with type: ActionBarButtonType) {
    if type == .more {
      showMore()
      return
    }
    delegate?.actionBarDidPressButton(type)
  }
}
