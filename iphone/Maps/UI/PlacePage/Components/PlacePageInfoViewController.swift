protocol PlacePageInfoViewControllerDelegate: AnyObject {
  var shouldShowOpenInApp: Bool { get }

  func didPressCall(to phone: PlacePagePhone)
  func didPressWebsite()
  func didPressWebsiteMenu()
  func didPressWikipedia()
  func didPressWikimediaCommons()
  func didPressFacebook()
  func didPressInstagram()
  func didPressTwitter()
  func didPressVk()
  func didPressLine()
  func didPressEmail()
  func didPressOpenInApp(from sourceView: UIView)
  func didCopy(_ content: String)
}

class PlacePageInfoViewController: UIViewController {
  private struct Constants {
    static let coordFormatIdKey = "PlacePageInfoViewController_coordFormatIdKey"
  }

  private typealias TapHandler = InfoItemView.TapHandler
  private typealias Style = InfoItemView.Style

  @IBOutlet var stackView: UIStackView!

  private lazy var openingHoursViewController: OpeningHoursViewController = {
    storyboard!.instantiateViewController(ofType: OpeningHoursViewController.self)
  }()

  private var rawOpeningHoursView: InfoItemView?
  private var phoneViews: [InfoItemView] = []
  private var websiteView: InfoItemView?
  private var websiteMenuView: InfoItemView?
  private var wikipediaView: InfoItemView?
  private var wikimediaCommonsView: InfoItemView?
  private var emailView: InfoItemView?
  private var facebookView: InfoItemView?
  private var instagramView: InfoItemView?
  private var twitterView: InfoItemView?
  private var vkView: InfoItemView?
  private var lineView: InfoItemView?
  private var cuisineView: InfoItemView?
  private var operatorView: InfoItemView?
  private var wifiView: InfoItemView?
  private var atmView: InfoItemView?
  private var addressView: InfoItemView?
  private var levelView: InfoItemView?
  private var coordinatesView: InfoItemView?
  private var openWithAppView: InfoItemView?
  private var capacityView: InfoItemView?
  private var wheelchairView: InfoItemView?
  private var selfServiceView: InfoItemView?
  private var outdoorSeatingView: InfoItemView?
  private var driveThroughView: InfoItemView?
  private var networkView: InfoItemView?
  private var routeRefsView: InfoItemView?

  weak var placePageInfoData: PlacePageInfoData!
  weak var delegate: PlacePageInfoViewControllerDelegate?
  var coordinatesFormatId: Int {
    get { UserDefaults.standard.integer(forKey: Constants.coordFormatIdKey) }
    set { UserDefaults.standard.set(newValue, forKey: Constants.coordFormatIdKey) }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    stackView.axis = .vertical
    stackView.alignment = .fill
    stackView.spacing = 0
    stackView.translatesAutoresizingMaskIntoConstraints = false
    view.addSubview(stackView)
    NSLayoutConstraint.activate([
      stackView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
      stackView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
      stackView.topAnchor.constraint(equalTo: view.topAnchor),
      stackView.bottomAnchor.constraint(equalTo: view.bottomAnchor)
    ])
    setupViews()
  }

  // MARK: private
  private func setupViews() {
    if let openingHours = placePageInfoData.openingHours {
      openingHoursViewController.openingHours = openingHours
      addChild(openingHoursViewController)
      addToStack(openingHoursViewController.view)
      openingHoursViewController.didMove(toParent: self)
    } else if let openingHoursString = placePageInfoData.openingHoursString {
      rawOpeningHoursView = createInfoItem(openingHoursString, icon: UIImage(resource: .icPlacepageOpenHours))
    }

    if let cuisine = placePageInfoData.cuisine {
      cuisineView = createInfoItem(cuisine, icon: UIImage(resource: .icPlacepageCuisine))
    }

    if let routeRefs = placePageInfoData.routeRefs {
      routeRefsView = createInfoItem(routeRefs, icon: UIImage(resource: .icPlacepageBus))
    }

    /// @todo Entrance is missing compared with Android. It's shown in title, but anyway ..

    phoneViews = placePageInfoData.phones.map({ phone in
      var cellStyle: Style = .regular
      if let phoneUrl = phone.url, UIApplication.shared.canOpenURL(phoneUrl) {
        cellStyle = .link
      }
      return createInfoItem(phone.phone,
                                 icon: UIImage(resource: .icPlacepagePhoneNumber),
                                 style: cellStyle,
                                 tapHandler: { [weak self] in
        self?.delegate?.didPressCall(to: phone)
      },
                                 longPressHandler: { [weak self] in
        self?.delegate?.didCopy(phone.phone)
      })
    })

    if let ppOperator = placePageInfoData.ppOperator {
      operatorView = createInfoItem(ppOperator, icon: UIImage(resource: .icPlacepageOperator))
    }

    if let network = placePageInfoData.network {
      networkView = createInfoItem(network, icon: UIImage(resource: .icPlacepageNetwork))
    }

    if let website = placePageInfoData.website {
      // Strip website url only when the value is displayed, to avoid issues when it's opened or edited.
      websiteView = createInfoItem(stripUrl(str: website),
                                   icon: UIImage(resource: .icPlacepageWebsite),
                                   style: .link,
                                   tapHandler: { [weak self] in
        self?.delegate?.didPressWebsite()
      },
                                   longPressHandler: { [weak self] in
        self?.delegate?.didCopy(website)
      })
    }

    if let websiteMenu = placePageInfoData.websiteMenu {
      websiteView = createInfoItem(L("website_menu"),
                                   icon: UIImage(resource: .icPlacepageWebsiteMenu),
                                   style: .link,
                                   tapHandler: { [weak self] in
        self?.delegate?.didPressWebsiteMenu()
      },
                                   longPressHandler: { [weak self] in
        self?.delegate?.didCopy(websiteMenu)
      })
    }

    if let wikipedia = placePageInfoData.wikipedia {
      wikipediaView = createInfoItem(L("read_in_wikipedia"),
                                     icon: UIImage(resource: .icPlacepageWiki),
                                     style: .link,
                                     tapHandler: { [weak self] in
        self?.delegate?.didPressWikipedia()
      },
                                     longPressHandler: { [weak self] in
        self?.delegate?.didCopy(wikipedia)
      })
    }

    if let wikimediaCommons = placePageInfoData.wikimediaCommons {
      wikimediaCommonsView = createInfoItem(L("wikimedia_commons"),
                                            icon: UIImage(resource: .icPlacepageWikimediaCommons),
                                            style: .link,
                                            tapHandler: { [weak self] in
        self?.delegate?.didPressWikimediaCommons()
      },
                                            longPressHandler: { [weak self] in
        self?.delegate?.didCopy(wikimediaCommons)
      })
    }

    if let wifi = placePageInfoData.wifiAvailable {
      wifiView = createInfoItem(wifi, icon: UIImage(resource: .icPlacepageWifi))
    }

    if let atm = placePageInfoData.atm {
      atmView = createInfoItem(atm, icon: UIImage(resource: .icPlacepageAtm))
    }

    if let level = placePageInfoData.level {
      levelView = createInfoItem(level, icon: UIImage(resource: .icPlacepageLevel))
    }

    if let capacity = placePageInfoData.capacity {
      capacityView = createInfoItem(capacity, icon: UIImage(resource: .icPlacepageCapacity))
    }

    if let wheelchair = placePageInfoData.wheelchair {
      wheelchairView = createInfoItem(wheelchair, icon: UIImage(resource: .icPlacepageWheelchair))
    }

    if let selfService = placePageInfoData.selfService {
      selfServiceView = createInfoItem(selfService, icon: UIImage(resource: .icPlacepageSelfService))
    }

    if let outdoorSeating = placePageInfoData.outdoorSeating {
      outdoorSeatingView = createInfoItem(outdoorSeating, icon: UIImage(resource: .icPlacepageOutdoorSeating))
    }

    if let driveThrough = placePageInfoData.driveThrough {
      driveThroughView = createInfoItem(driveThrough, icon: UIImage(resource: .icPlacepageDriveThrough))
    }

    if let email = placePageInfoData.email {
      emailView = createInfoItem(email,
                                 icon: UIImage(resource: .icPlacepageEmail),
                                 style: .link,
                                 tapHandler: { [weak self] in
        self?.delegate?.didPressEmail()
      },
                                 longPressHandler: { [weak self] in
        self?.delegate?.didCopy(email)
      })
    }

    if let facebook = placePageInfoData.facebook {
      facebookView = createInfoItem(facebook,
                                    icon: UIImage(resource: .icPlacepageFacebook),
                                    style: .link,
                                    tapHandler: { [weak self] in
        self?.delegate?.didPressFacebook()
      },
                                    longPressHandler: { [weak self] in
        self?.delegate?.didCopy(facebook)
      })
    }

    if let instagram = placePageInfoData.instagram {
      instagramView = createInfoItem(instagram,
                                     icon: UIImage(resource: .icPlacepageInstagram),
                                     style: .link,
                                     tapHandler: { [weak self] in
        self?.delegate?.didPressInstagram()
      },
                                     longPressHandler: { [weak self] in
        self?.delegate?.didCopy(instagram)
      })
    }

    if let twitter = placePageInfoData.twitter {
      twitterView = createInfoItem(twitter,
                                   icon: UIImage(resource: .icPlacepageTwitter),
                                   style: .link,
                                   tapHandler: { [weak self] in
        self?.delegate?.didPressTwitter()
      },
                                   longPressHandler: { [weak self] in
        self?.delegate?.didCopy(twitter)
      })
    }

    if let vk = placePageInfoData.vk {
      vkView = createInfoItem(vk,
                              icon: UIImage(resource: .icPlacepageVk),
                              style: .link,
                              tapHandler: { [weak self] in
        self?.delegate?.didPressVk()
      },
                              longPressHandler: { [weak self] in
        self?.delegate?.didCopy(vk)
      })
    }

    if let line = placePageInfoData.line {
      lineView = createInfoItem(line,
                                icon: UIImage(resource: .icPlacepageLine),
                                style: .link,
                                tapHandler: { [weak self] in
        self?.delegate?.didPressLine()
      },
                                longPressHandler: { [weak self] in
        self?.delegate?.didCopy(line)
      })
    }

    if let address = placePageInfoData.address {
      addressView = createInfoItem(address,
                                   icon: UIImage(resource: .icPlacepageAddress),
                                   longPressHandler: { [weak self] in
        self?.delegate?.didCopy(address)
      })
    }

    setupCoordinatesView()
    setupOpenWithAppView()
  }

  private func setupCoordinatesView() {
    guard let coordFormats = placePageInfoData.coordFormats as? Array<String> else { return }
    var formatId = coordinatesFormatId
    if formatId >= coordFormats.count {
      formatId = 0
    }
    coordinatesView = createInfoItem(coordFormats[formatId],
                                     icon: UIImage(resource: .icPlacepageCoordinate),
                                     style: .link,
                                     accessoryImage: UIImage(resource: .icPlacepageChange),
                                     tapHandler: { [weak self] in
      guard let self else { return }
      let formatId = (self.coordinatesFormatId + 1) % coordFormats.count
      self.setCoordinatesSelected(formatId: formatId)
    },
                                     longPressHandler: { [weak self] in
      self?.copyCoordinatesToPasteboard()
    })
    if #available(iOS 14.0, *) {
      let menu = UIMenu(children: coordFormats.enumerated().map { (index, format) in
        UIAction(title: format, handler: { [weak self] _ in
          self?.setCoordinatesSelected(formatId: index)
          self?.copyCoordinatesToPasteboard()
        })
      })
      coordinatesView?.setAccessoryMenu(menu)
    }
  }

  private func setCoordinatesSelected(formatId: Int) {
    guard let coordFormats = placePageInfoData.coordFormats as? Array<String> else { return }
    coordinatesFormatId = formatId
    let coordinates: String = coordFormats[formatId]
    coordinatesView?.setTitle(coordinates, style: .link)
  }

  private func copyCoordinatesToPasteboard() {
    guard let coordFormats = placePageInfoData.coordFormats as? Array<String> else { return }
    let coordinates: String = coordFormats[coordinatesFormatId]
    delegate?.didCopy(coordinates)
  }

  private func setupOpenWithAppView() {
    guard let delegate, delegate.shouldShowOpenInApp else { return }
    openWithAppView = createInfoItem(L("open_in_app"),
                                     icon: UIImage(resource: .icOpenInApp),
                                     style: .link,
                                     tapHandler: { [weak self] in
      guard let self, let openWithAppView else { return }
      self.delegate?.didPressOpenInApp(from: openWithAppView)
    })
  }

  private func createInfoItem(_ info: String,
                              icon: UIImage?,
                              tapIconHandler: TapHandler? = nil,
                              style: Style = .regular,
                              accessoryImage: UIImage? = nil,
                              tapHandler: TapHandler? = nil,
                              longPressHandler: TapHandler? = nil,
                              accessoryImageTapHandler: TapHandler? = nil) -> InfoItemView {
    let view = InfoItemView()
    addToStack(view)
    view.setTitle(info, style: style, tapHandler: tapHandler, longPressHandler: longPressHandler)
    view.setIcon(image: icon?.withRenderingMode(.alwaysTemplate), tapHandler: tapIconHandler)
    view.setAccessory(image: accessoryImage, tapHandler: accessoryImageTapHandler)
    return view
  }

  private func addToStack(_ view: UIView) {
    stackView.addArrangedSubviewWithSeparator(view, insets: UIEdgeInsets(top: 0, left: 56, bottom: 0, right: 0))
  }

  private static let kHttp = "http://"
  private static let kHttps = "https://"

  private func stripUrl(str: String) -> String {
    let dropFromStart = str.hasPrefix(PlacePageInfoViewController.kHttps) ? PlacePageInfoViewController.kHttps.count
        : (str.hasPrefix(PlacePageInfoViewController.kHttp) ? PlacePageInfoViewController.kHttp.count : 0);
    let dropFromEnd = str.hasSuffix("/") ? 1 : 0;
    return String(str.dropFirst(dropFromStart).dropLast(dropFromEnd))
  }
}

private extension UIStackView {
  func addArrangedSubviewWithSeparator(_ view: UIView, insets: UIEdgeInsets = .zero) {
    if !arrangedSubviews.isEmpty {
      view.addSeparator(thickness: CGFloat(1.0), insets: insets)
    }
    addArrangedSubview(view)
  }
}
