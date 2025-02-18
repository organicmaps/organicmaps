class InfoItemViewController: UIViewController {
  enum Style {
    case regular
    case link
  }

  typealias TapHandler = () -> Void

  @IBOutlet var imageView: UIImageView!
  @IBOutlet var infoLabel: UILabel!
  @IBOutlet var accessoryButton: UIButton!

  private var tapGestureRecognizer: UITapGestureRecognizer!
  private var longPressGestureRecognizer: UILongPressGestureRecognizer!

  var tapHandler: TapHandler?
  var longPressHandler: TapHandler?
  var accessoryImageTapHandler: TapHandler?

  private var style: Style = .regular

  override func viewDidLoad() {
    super.viewDidLoad()
    setupView()
  }

  private func setupView() {
    tapGestureRecognizer = UITapGestureRecognizer(target: self, action: #selector(onTap))
    longPressGestureRecognizer = UILongPressGestureRecognizer(target: self, action: #selector(onLongPress(_:)))
    view.addGestureRecognizer(tapGestureRecognizer)
    view.addGestureRecognizer(longPressGestureRecognizer)
    
    accessoryButton.addTarget(self, action: #selector(onAccessoryButtonTap), for: .touchUpInside)
    setStyle(style)
  }

  @objc
  private func onTap() {
    tapHandler?()
  }

  @objc
  private func onLongPress(_ sender: UILongPressGestureRecognizer) {
    guard sender.state == .began else { return }
    longPressHandler?()
  }

  @objc
  private func onAccessoryButtonTap() {
    accessoryImageTapHandler?()
  }

  func setStyle(_ style: Style) {
    switch style {
    case .regular:
      imageView?.setStyleAndApply(.black)
      infoLabel?.setFontStyleAndApply(.blackPrimary)
    case .link:
      imageView?.setStyleAndApply(.blue)
      infoLabel?.setFontStyleAndApply(.linkBlue)
    }
    accessoryButton.setStyleAndApply(.black)
    self.style = style
  }

  func setAccessory(image: UIImage?, tapHandler: TapHandler? = nil) {
    accessoryButton.setTitle("", for: .normal)
    accessoryButton.setImage(image, for: .normal)
    accessoryButton.isHidden = image == nil
    accessoryImageTapHandler = tapHandler
  }
}

protocol PlacePageInfoViewControllerDelegate: AnyObject {
  var shouldShowOpenInApp: Bool { get }

  func didPressCall()
  func didPressWebsite()
  func didPressWebsiteMenu()
  func didPressKayak()
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
  private typealias TapHandler = InfoItemViewController.TapHandler
  private typealias Style = InfoItemViewController.Style

  @IBOutlet var stackView: UIStackView!

  private lazy var openingHoursView: OpeningHoursViewController = {
    storyboard!.instantiateViewController(ofType: OpeningHoursViewController.self)
  }()

  private var rawOpeningHoursView: InfoItemViewController?
  private var phoneView: InfoItemViewController?
  private var websiteView: InfoItemViewController?
  private var websiteMenuView: InfoItemViewController?
  private var kayakView: InfoItemViewController?
  private var wikipediaView: InfoItemViewController?
  private var wikimediaCommonsView: InfoItemViewController?
  private var emailView: InfoItemViewController?
  private var facebookView: InfoItemViewController?
  private var instagramView: InfoItemViewController?
  private var twitterView: InfoItemViewController?
  private var vkView: InfoItemViewController?
  private var lineView: InfoItemViewController?
  private var cuisineView: InfoItemViewController?
  private var operatorView: InfoItemViewController?
  private var wifiView: InfoItemViewController?
  private var atmView: InfoItemViewController?
  private var addressView: InfoItemViewController?
  private var levelView: InfoItemViewController?
  private var coordinatesView: InfoItemViewController?
  private var openWithAppView: InfoItemViewController?
  private var capacityView: InfoItemViewController?
  private var wheelchairView: InfoItemViewController?
  private var selfServiceView: InfoItemViewController?
  private var outdoorSeatingView: InfoItemViewController?
  private var driveThroughView: InfoItemViewController?
  private var networkView: InfoItemViewController?

  weak var placePageInfoData: PlacePageInfoData!
  weak var delegate: PlacePageInfoViewControllerDelegate?
  var coordinatesFormatId: Int {
    get { UserDefaults.standard.integer(forKey: Constants.coordFormatIdKey) }
    set { UserDefaults.standard.set(newValue, forKey: Constants.coordFormatIdKey) }
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    setupViews()
  }

  // MARK: private
  private func setupViews() {
    if let openingHours = placePageInfoData.openingHours {
      openingHoursView.openingHours = openingHours
      addToStack(openingHoursView)
    } else if let openingHoursString = placePageInfoData.openingHoursString {
      rawOpeningHoursView = createInfoItem(openingHoursString, icon: UIImage(named: "ic_placepage_open_hours"))
      rawOpeningHoursView?.infoLabel.numberOfLines = 0
    }

    if let cuisine = placePageInfoData.cuisine {
      cuisineView = createInfoItem(cuisine, icon: UIImage(named: "ic_placepage_cuisine"))
    }

    /// @todo Entrance is missing compared with Android. It's shown in title, but anyway ..

    if let phone = placePageInfoData.phone {
      var cellStyle: Style = .regular
      if let phoneUrl = placePageInfoData.phoneUrl, UIApplication.shared.canOpenURL(phoneUrl) {
        cellStyle = .link
      }
      phoneView = createInfoItem(phone,
                                 icon: UIImage(named: "ic_placepage_phone_number"),
                                 style: cellStyle,
                                 tapHandler: { [weak self] in
        self?.delegate?.didPressCall()
      },
                                 longPressHandler: { [weak self] in
        self?.delegate?.didCopy(phone)
      })
    }

    if let ppOperator = placePageInfoData.ppOperator {
      operatorView = createInfoItem(ppOperator, icon: UIImage(named: "ic_placepage_operator"))
    }
    
    if let network = placePageInfoData.network {
      networkView = createInfoItem(network, icon: UIImage(named: "ic_placepage_network"))
    }

    if let website = placePageInfoData.website {
      // Strip website url only when the value is displayed, to avoid issues when it's opened or edited.
      websiteView = createInfoItem(stripUrl(str: website),
                                   icon: UIImage(named: "ic_placepage_website"),
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
                                   icon: UIImage(named: "ic_placepage_website_menu"),
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
                                     icon: UIImage(named: "ic_placepage_wiki"),
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
                                            icon: UIImage(named: "ic_placepage_wikimedia_commons"),
                                            style: .link,
                                            tapHandler: { [weak self] in
        self?.delegate?.didPressWikimediaCommons()
      },
                                            longPressHandler: { [weak self] in
        self?.delegate?.didCopy(wikimediaCommons)
      })
    }

    if let wifi = placePageInfoData.wifiAvailable {
      wifiView = createInfoItem(wifi, icon: UIImage(named: "ic_placepage_wifi"))
    }

    if let atm = placePageInfoData.atm {
      atmView = createInfoItem(atm, icon: UIImage(named: "ic_placepage_atm"))
    }

    if let level = placePageInfoData.level {
      levelView = createInfoItem(level, icon: UIImage(named: "ic_placepage_level"))
    }

    if let capacity = placePageInfoData.capacity {
      capacityView = createInfoItem(capacity, icon: UIImage(named: "ic_placepage_capacity"))
    }

    if let wheelchair = placePageInfoData.wheelchair {
      wheelchairView = createInfoItem(wheelchair, icon: UIImage(named: "ic_placepage_wheelchair"))
    }

    if let selfService = placePageInfoData.selfService {
      selfServiceView = createInfoItem(selfService, icon: UIImage(named: "ic_placepage_self_service"))
    }

    if let outdoorSeating = placePageInfoData.outdoorSeating {
      outdoorSeatingView = createInfoItem(outdoorSeating, icon: UIImage(named: "ic_placepage_outdoor_seating"))
    }

    if let driveThrough = placePageInfoData.driveThrough {
      driveThroughView = createInfoItem(driveThrough, icon: UIImage(named: "ic_placepage_drive_through"))
    }

    if let email = placePageInfoData.email {
      emailView = createInfoItem(email,
                                 icon: UIImage(named: "ic_placepage_email"),
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
                                    icon: UIImage(named: "ic_placepage_facebook"),
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
                                     icon: UIImage(named: "ic_placepage_instagram"),
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
                                   icon: UIImage(named: "ic_placepage_twitter"),
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
                              icon: UIImage(named: "ic_placepage_vk"),
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
                                icon: UIImage(named: "ic_placepage_line"),
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
                                   icon: UIImage(named: "ic_placepage_address"),
                                   longPressHandler: { [weak self] in
        self?.delegate?.didCopy(address)
      })
    }

    if let kayak = placePageInfoData.kayak {
      kayakView = createInfoItem(L("more_on_kayak"),
                                 icon: UIImage(named: "ic_placepage_kayak"),
                                 style: .link,
                                 tapHandler: { [weak self] in
        self?.delegate?.didPressKayak()
      },
                                 longPressHandler: { [weak self] in
        self?.delegate?.didCopy(kayak)
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
                                     icon: UIImage(named: "ic_placepage_coordinate"),
                                     accessoryImage: UIImage(named: "ic_placepage_change"),
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
      coordinatesView?.accessoryButton.menu = menu
      coordinatesView?.accessoryButton.showsMenuAsPrimaryAction = true
    }
  }

  private func setCoordinatesSelected(formatId: Int) {
    guard let coordFormats = placePageInfoData.coordFormats as? Array<String> else { return }
    coordinatesFormatId = formatId
    let coordinates: String = coordFormats[formatId]
    coordinatesView?.infoLabel.text = coordinates
  }

  private func copyCoordinatesToPasteboard() {
    guard let coordFormats = placePageInfoData.coordFormats as? Array<String> else { return }
    let coordinates: String = coordFormats[coordinatesFormatId]
    delegate?.didCopy(coordinates)
  }

  private func setupOpenWithAppView() {
    guard let delegate, delegate.shouldShowOpenInApp else { return }
    openWithAppView = createInfoItem(L("open_in_app"),
                                     icon: UIImage(named: "ic_open_in_app"),
                                     style: .link,
                                     tapHandler: { [weak self] in
      guard let self, let openWithAppView else { return }
      self.delegate?.didPressOpenInApp(from: openWithAppView.view)
    })
  }

  private func createInfoItem(_ info: String,
                              icon: UIImage?,
                              style: Style = .regular,
                              accessoryImage: UIImage? = nil,
                              tapHandler: TapHandler? = nil,
                              longPressHandler: TapHandler? = nil,
                              accessoryImageTapHandler: TapHandler? = nil) -> InfoItemViewController {
    let vc = storyboard!.instantiateViewController(ofType: InfoItemViewController.self)
    addToStack(vc)
    vc.imageView.image = icon
    vc.infoLabel.text = info
    vc.setStyle(style)
    vc.tapHandler = tapHandler
    vc.longPressHandler = longPressHandler
    vc.setAccessory(image: accessoryImage, tapHandler: accessoryImageTapHandler)
    return vc;
  }

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubviewWithSeparator(viewController.view, insets: UIEdgeInsets(top: 0, left: 56, bottom: 0, right: 0))
    viewController.didMove(toParent: self)
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
