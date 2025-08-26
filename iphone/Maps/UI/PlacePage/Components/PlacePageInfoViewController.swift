final class InfoItemView: UIView {
  private enum Constants {
    static let viewHeight: CGFloat = 44
    static let stackViewSpacing: CGFloat = 0
    static let iconButtonSize: CGFloat = 56
    static let iconButtonEdgeInsets = UIEdgeInsets(top: 8, left: 0, bottom: 8, right: 0)
    static let infoLabelFontSize: CGFloat = 16
    static let infoLabelTopBottomSpacing: CGFloat = 10
    static let accessoryButtonSize: CGFloat = 44
  }

  enum Style {
    case regular
    case link
  }

  typealias TapHandler = () -> Void

  let iconButton = UIButton()
  let infoLabel = UILabel()
  let accessoryButton = UIButton()

  var infoLabelTapHandler: TapHandler?
  var infoLabelLongPressHandler: TapHandler?
  var iconButtonTapHandler: TapHandler?
  var accessoryImageTapHandler: TapHandler?

  private var style: Style = .regular

  override init(frame: CGRect) {
    super.init(frame: frame)
    setupView()
    layout()
  }

  required init?(coder: NSCoder) {
    super.init(coder: coder)
    setupView()
    layout()
  }

  private func setupView() {
    addGestureRecognizer(UITapGestureRecognizer(target: self, action: #selector(onInfoLabelTap)))
    addGestureRecognizer(UILongPressGestureRecognizer(target: self, action: #selector(onInfoLabelLongPress(_:))))

    infoLabel.lineBreakMode = .byTruncatingTail
    infoLabel.numberOfLines = 1
    infoLabel.allowsDefaultTighteningForTruncation = true
    infoLabel.isUserInteractionEnabled = false

    iconButton.imageView?.contentMode = .scaleAspectFit
    iconButton.addTarget(self, action: #selector(onIconButtonTap), for: .touchUpInside)
    iconButton.contentEdgeInsets = Constants.iconButtonEdgeInsets

    accessoryButton.addTarget(self, action: #selector(onAccessoryButtonTap), for: .touchUpInside)
  }

  private func layout() {
    addSubview(iconButton)
    addSubview(infoLabel)
    addSubview(accessoryButton)

    translatesAutoresizingMaskIntoConstraints = false
    iconButton.translatesAutoresizingMaskIntoConstraints = false
    infoLabel.translatesAutoresizingMaskIntoConstraints = false
    accessoryButton.translatesAutoresizingMaskIntoConstraints = false

    NSLayoutConstraint.activate([
      heightAnchor.constraint(equalToConstant: Constants.viewHeight),

      iconButton.leadingAnchor.constraint(equalTo: leadingAnchor),
      iconButton.centerYAnchor.constraint(equalTo: centerYAnchor),
      iconButton.widthAnchor.constraint(equalToConstant: Constants.iconButtonSize),
      iconButton.topAnchor.constraint(equalTo: topAnchor),
      iconButton.bottomAnchor.constraint(equalTo: bottomAnchor),

      infoLabel.leadingAnchor.constraint(equalTo: iconButton.trailingAnchor),
      infoLabel.topAnchor.constraint(equalTo: topAnchor),
      infoLabel.bottomAnchor.constraint(equalTo: bottomAnchor),
      infoLabel.trailingAnchor.constraint(equalTo: accessoryButton.leadingAnchor),

      accessoryButton.trailingAnchor.constraint(equalTo: trailingAnchor),
      accessoryButton.centerYAnchor.constraint(equalTo: centerYAnchor),
      accessoryButton.widthAnchor.constraint(equalToConstant: Constants.accessoryButtonSize),
      accessoryButton.topAnchor.constraint(equalTo: topAnchor),
      accessoryButton.bottomAnchor.constraint(equalTo: bottomAnchor)
    ])
  }

  @objc
  private func onInfoLabelTap() {
    infoLabelTapHandler?()
  }

  @objc
  private func onInfoLabelLongPress(_ sender: UILongPressGestureRecognizer) {
    guard sender.state == .began else { return }
    infoLabelLongPressHandler?()
  }

  @objc
  private func onIconButtonTap() {
    iconButtonTapHandler?()
  }

  @objc
  private func onAccessoryButtonTap() {
    accessoryImageTapHandler?()
  }

  func setStyle(_ style: Style) {
    switch style {
    case .regular:
      iconButton.setStyleAndApply(.black)
      infoLabel.setFontStyleAndApply(.regular16, color: .blackPrimary)
    case .link:
      iconButton.setStyleAndApply(.blue)
      infoLabel.setFontStyleAndApply(.regular16, color: .linkBlue)
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
      rawOpeningHoursView = createInfoItem(openingHoursString, icon: UIImage(named: "ic_placepage_open_hours"))
      rawOpeningHoursView?.infoLabel.numberOfLines = 0
    }

    if let cuisine = placePageInfoData.cuisine {
      cuisineView = createInfoItem(cuisine, icon: UIImage(named: "ic_placepage_cuisine"))
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
                                 icon: UIImage(named: "ic_placepage_phone_number"),
                                 style: cellStyle,
                                 tapHandler: { [weak self] in
        self?.delegate?.didPressCall(to: phone)
      },
                                 longPressHandler: { [weak self] in
        self?.delegate?.didCopy(phone.phone)
      })
    })

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
    view.iconButton.setImage(icon?.withRenderingMode(.alwaysTemplate), for: .normal)
    view.iconButtonTapHandler = tapIconHandler
    view.infoLabel.text = info
    view.setStyle(style)
    view.infoLabelTapHandler = tapHandler
    view.infoLabelLongPressHandler = longPressHandler
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
