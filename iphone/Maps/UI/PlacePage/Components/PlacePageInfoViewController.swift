protocol PlacePageInfoViewControllerDelegate: AnyObject {
  var shouldShowOpenInApp: Bool { get }

  func didPressCall(to phone: PlacePagePhone)
  func didPressWebsite()
  func didPressHeritageWebsite()
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
  func didSelectPublicTransportRoute(scrollAnchor: UIView)
}

class PlacePageInfoViewController: UIViewController {
  private enum Constants {
    static let coordFormatIdKey = "PlacePageInfoViewController_coordFormatIdKey"
  }

  private typealias TapHandler = InfoItemView.TapHandler
  private typealias Style = InfoItemView.Style

  @IBOutlet var stackView: UIStackView!

  private lazy var openingHoursViewController: OpeningHoursViewController = storyboard!.instantiateViewController(ofType: OpeningHoursViewController.self)

  private var coordinatesView: InfoItemView?
  private var openWithAppView: InfoItemView?
  private var routeRefsView: InfoItemView?
  /// Relation id of the route most recently picked from the refs popup, or nil.
  /// Used to render exactly one selected row in the popup.
  private var selectedRouteRelId: UInt32?
  private weak var routesSelectorViewController: PopoverListSelectorViewController?

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
      stackView.bottomAnchor.constraint(equalTo: view.bottomAnchor),
    ])
    setupViews()
  }

  override func traitCollectionDidChange(_ previousTraitCollection: UITraitCollection?) {
    super.traitCollectionDidChange(previousTraitCollection)
    // When adaptivePresentationStyle returns .none, the presented view controller loses its parent trait collection relationship,
    // and the user interface style should be updated manually.
    routesSelectorViewController?.overrideUserInterfaceStyle = traitCollection.userInterfaceStyle
  }

  // MARK: private

  private func setupViews() {
    if let openingHours = placePageInfoData.openingHours {
      openingHoursViewController.openingHours = openingHours
      addChild(openingHoursViewController)
      addToStack(openingHoursViewController.view)
      openingHoursViewController.didMove(toParent: self)
    } else if let openingHoursString = placePageInfoData.openingHoursString {
      createInfoItem(openingHoursString, icon: UIImage(resource: .icPlacepageOpenHours))
    }

    if let cuisine = placePageInfoData.cuisine {
      createInfoItem(cuisine, icon: UIImage(resource: .icPlacepageCuisine))
    }

    if let routes = placePageInfoData.routes, !routes.isEmpty {
      // Routes can repeat the same ref with different from/to (e.g. inbound/outbound directions
      // of the same line). Collapse them in the primary row — the popup still shows all entries.
      var seen = Set<String>()
      let uniqueRefs = routes.compactMap { seen.insert($0.ref).inserted ? $0.ref : nil }
      routeRefsView = createInfoItem(uniqueRefs.joined(separator: " • "),
                                     icon: UIImage.icPlacepageBus,
                                     tapIconHandler: { [weak self] in
                                       self?.showRoutesSelector()
                                     },
                                     style: .link,
                                     accessoryImage: UIImage.icPlacepageChange,
                                     tapHandler: { [weak self] in
                                       self?.showRoutesSelector()
                                     },
                                     accessoryImageTapHandler: { [weak self] in
                                       self?.showRoutesSelector()
                                     })
      selectedRouteRelId = routes.first { $0.ref == FrameworkHelper.activeTransitRouteRef() }?.relId
      updateRouteRefsLabel()
    }

    // @todo Entrance is missing compared with Android. It's shown in title, but anyway ..

    for phone in placePageInfoData.phones {
      var cellStyle: Style = .regular
      if let phoneUrl = phone.url, UIApplication.shared.canOpenURL(phoneUrl) {
        cellStyle = .link
      }
      createInfoItem(phone.phone,
                     icon: UIImage(resource: .icPlacepagePhoneNumber),
                     style: cellStyle,
                     tapHandler: { [weak self] in
                       self?.delegate?.didPressCall(to: phone)
                     },
                     longPressHandler: { [weak self] in
                       self?.delegate?.didCopy(phone.phone)
                     })
    }

    if let ppOperator = placePageInfoData.ppOperator {
      createInfoItem(ppOperator, icon: UIImage(resource: .icPlacepageOperator))
    }

    if let network = placePageInfoData.network {
      createInfoItem(network, icon: UIImage(resource: .icPlacepageNetwork))
    }

    // Strip website url only when the value is displayed, to avoid issues when it's opened or edited.
    createLinkItem(placePageInfoData.website,
                   displayed: placePageInfoData.website.map { stripUrl(str: $0) },
                   icon: UIImage(resource: .icPlacepageWebsite)) { $0.didPressWebsite() }
    createLinkItem(placePageInfoData.heritageWebsite,
                   displayed: placePageInfoData.heritageWebsite.map { stripUrl(str: $0) },
                   icon: UIImage(resource: .icPlacepageWebsite)) { $0.didPressHeritageWebsite() }
    createLinkItem(placePageInfoData.websiteMenu,
                   displayed: L("website_menu"),
                   icon: UIImage(resource: .icPlacepageWebsiteMenu)) { $0.didPressWebsiteMenu() }
    createLinkItem(placePageInfoData.wikipedia,
                   displayed: L("read_in_wikipedia"),
                   icon: UIImage(resource: .icPlacepageWiki)) { $0.didPressWikipedia() }
    createLinkItem(placePageInfoData.wikimediaCommons,
                   displayed: L("wikimedia_commons"),
                   icon: UIImage(resource: .icPlacepageWikimediaCommons)) { $0.didPressWikimediaCommons() }

    if let wifi = placePageInfoData.wifiAvailable {
      createInfoItem(wifi, icon: UIImage(resource: .icPlacepageWifi))
    }

    if let atm = placePageInfoData.atm {
      createInfoItem(atm, icon: UIImage(resource: .icPlacepageAtm))
    }

    if let level = placePageInfoData.level {
      createInfoItem(level, icon: UIImage(resource: .icPlacepageLevel))
    }

    if let capacity = placePageInfoData.capacity {
      createInfoItem(capacity, icon: UIImage(resource: .icPlacepageCapacity))
    }

    if let wheelchair = placePageInfoData.wheelchair {
      createInfoItem(wheelchair, icon: UIImage(resource: .icPlacepageWheelchair))
    }

    if let selfService = placePageInfoData.selfService {
      createInfoItem(selfService, icon: UIImage(resource: .icPlacepageSelfService))
    }

    if let outdoorSeating = placePageInfoData.outdoorSeating {
      createInfoItem(outdoorSeating, icon: UIImage(resource: .icPlacepageOutdoorSeating))
    }

    if let driveThrough = placePageInfoData.driveThrough {
      createInfoItem(driveThrough, icon: UIImage(resource: .icPlacepageDriveThrough))
    }

    createLinkItem(placePageInfoData.email, icon: UIImage(resource: .icPlacepageEmail)) { $0.didPressEmail() }
    createLinkItem(placePageInfoData.facebook, icon: UIImage(resource: .icPlacepageFacebook)) { $0.didPressFacebook() }
    createLinkItem(placePageInfoData.instagram,
                   icon: UIImage(resource: .icPlacepageInstagram)) { $0.didPressInstagram() }
    createLinkItem(placePageInfoData.twitter, icon: UIImage(resource: .icPlacepageTwitter)) { $0.didPressTwitter() }
    createLinkItem(placePageInfoData.vk, icon: UIImage(resource: .icPlacepageVk)) { $0.didPressVk() }
    createLinkItem(placePageInfoData.line, icon: UIImage(resource: .icPlacepageLine)) { $0.didPressLine() }

    if let address = placePageInfoData.address {
      createInfoItem(address,
                     icon: UIImage(resource: .icPlacepageAddress),
                     longPressHandler: { [weak self] in
                       self?.delegate?.didCopy(address)
                     })
    }

    setupCoordinatesView()
    setupOpenWithAppView()
  }

  private func setupCoordinatesView() {
    guard let coordFormats = placePageInfoData.coordFormats as? [String] else { return }
    // The saved format may be unavailable here (e.g. OS Grid outside Great Britain); its entry is
    // empty. Show the next available format without overwriting the saved preference, so it is
    // restored when the user returns to a supported area.
    let displayId = effectiveFormatId(in: coordFormats)
    coordinatesView = createInfoItem(coordFormats[displayId],
                                     icon: UIImage(resource: .icPlacepageCoordinate),
                                     style: .link,
                                     accessoryImage: UIImage(resource: .icPlacepageChange),
                                     tapHandler: { [weak self] in
                                       guard let self else { return }
                                       let formatId = self.nextAvailableFormatId(after: self.effectiveFormatId(in: coordFormats), in: coordFormats)
                                       self.setCoordinatesSelected(formatId: formatId)
                                     },
                                     longPressHandler: { [weak self] in
                                       self?.copyCoordinatesToPasteboard()
                                     })
    let menu = UIMenu(children: coordFormats.enumerated().compactMap { index, format in
      format.isEmpty ? nil : UIAction(title: format, handler: { [weak self] _ in
        self?.setCoordinatesSelected(formatId: index)
        self?.copyCoordinatesToPasteboard()
      })
    })
    coordinatesView?.setAccessoryMenu(menu)
  }

  /// The format index actually shown: the saved one if available here, otherwise the next available.
  /// Does not change the saved preference (`coordinatesFormatId`).
  private func effectiveFormatId(in formats: [String]) -> Int {
    let saved = coordinatesFormatId
    let id = (saved >= 0 && saved < formats.count) ? saved : 0
    return formats[id].isEmpty ? nextAvailableFormatId(after: id, in: formats) : id
  }

  /// Returns the format index following `current` whose value is non-empty (available at this location).
  /// Falls back to `current` if none are available; the decimal formats are always present.
  private func nextAvailableFormatId(after current: Int, in formats: [String]) -> Int {
    for step in 1 ... formats.count {
      let candidate = (current + step) % formats.count
      if !formats[candidate].isEmpty {
        return candidate
      }
    }
    return current
  }

  private func setCoordinatesSelected(formatId: Int) {
    guard let coordFormats = placePageInfoData.coordFormats as? [String] else { return }
    coordinatesFormatId = formatId
    let coordinates: String = coordFormats[formatId]
    coordinatesView?.setTitle(coordinates, style: .link)
  }

  private func copyCoordinatesToPasteboard() {
    guard let coordFormats = placePageInfoData.coordFormats as? [String] else { return }
    let coordinates: String = coordFormats[effectiveFormatId(in: coordFormats)]
    delegate?.didCopy(coordinates)
  }

  private func showRoutesSelector() {
    guard let routeRefsView, let routes = placePageInfoData.routes else { return }

    if routes.count == 1 {
      selectRoute(routes[0])
      return
    }

    let popoverDataSource = routes.map { route in
      PopoverListSelectorViewController.RowViewModel(title: .attributed(routeMenuLabel(route)),
                                                     color: route.color,
                                                     isSelected: route.relId == selectedRouteRelId,
                                                     selectionHandler: { [weak self] in
                                                       self?.dismiss(animated: true, completion: { [weak self] in
                                                         self?.selectRoute(route)
                                                       })
                                                     })
    }
    let viewController = PopoverListSelectorBuilder(dataSource: popoverDataSource,
                                                    style: .background,
                                                    sourceView: routeRefsView,
                                                    sourceRect: routeRefsView.bounds,
                                                    userInterfaceStyle: traitCollection.userInterfaceStyle)
      .build()
    routesSelectorViewController = viewController
    present(viewController, animated: true)
  }

  private func routeMenuLabel(_ route: PlacePageRoute) -> NSAttributedString {
    let baseAttributes: [NSAttributedString.Key: Any] = [
      .font: UIFont.regular14.dynamic,
      .foregroundColor: UIColor.blackPrimaryText,
    ]
    let boldAttributes: [NSAttributedString.Key: Any] = [
      .font: UIFont.bold14.dynamic,
      .foregroundColor: UIColor.blackPrimaryText,
    ]
    let label = NSMutableAttributedString(string: route.ref, attributes: boldAttributes)
    if !route.from.isEmpty || !route.to.isEmpty {
      label.append(NSAttributedString(string: ": \(route.from)", attributes: baseAttributes))
      if !route.to.isEmpty {
        label.append(NSAttributedString(string: " → \(route.to)", attributes: baseAttributes))
      }
    }
    return label
  }

  private func selectRoute(_ route: PlacePageRoute) {
    FrameworkHelper.showRouteTransit(route.relId)
    selectedRouteRelId = route.relId
    updateRouteRefsLabel()
    if let routeRefsView {
      delegate?.didSelectPublicTransportRoute(scrollAnchor: routeRefsView)
    }
  }

  /// Rebuilds the primary refs string, bolding and underlining the selected route's ref.
  /// Inherits the current label's font and color as the baseline style.
  /// Routes that share a ref are collapsed (the popup still shows all entries).
  private func updateRouteRefsLabel() {
    guard let label = routeRefsView?.textLabel, let routes = placePageInfoData.routes else { return }
    let baseFont = label.font ?? UIFont.systemFont(ofSize: 16)
    let baseColor = label.textColor ?? UIColor.black
    let boldFont = UIFont.boldSystemFont(ofSize: baseFont.pointSize)
    let baseAttrs: [NSAttributedString.Key: Any] = [.font: baseFont, .foregroundColor: baseColor]
    let selectedRouteRef = selectedRouteRelId.flatMap { selectedRelId in
      routes.first { $0.relId == selectedRelId }?.ref
    }

    var seen = Set<String>()
    let result = NSMutableAttributedString()
    for route in routes {
      guard seen.insert(route.ref).inserted else { continue }
      if result.length > 0 {
        result.append(NSAttributedString(string: " • ", attributes: baseAttrs))
      }
      let start = result.length
      result.append(NSAttributedString(string: route.ref, attributes: baseAttrs))
      if route.ref == selectedRouteRef {
        let range = NSRange(location: start, length: (route.ref as NSString).length)
        result.addAttributes([
          .font: boldFont,
          .underlineStyle: NSUnderlineStyle.single.rawValue,
        ], range: range)
      }
    }
    label.attributedText = result
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

  /// Adds a link row for an optional `value`. `displayed` overrides the shown text;
  /// the long press always copies the raw `value`.
  private func createLinkItem(_ value: String?,
                              displayed: String? = nil,
                              icon: UIImage,
                              onTap: @escaping (PlacePageInfoViewControllerDelegate) -> Void) {
    guard let value else { return }
    createInfoItem(displayed ?? value,
                   icon: icon,
                   style: .link,
                   tapHandler: { [weak self] in
                     guard let delegate = self?.delegate else { return }
                     onTap(delegate)
                   },
                   longPressHandler: { [weak self] in
                     self?.delegate?.didCopy(value)
                   })
  }

  @discardableResult
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
      : (str.hasPrefix(PlacePageInfoViewController.kHttp) ? PlacePageInfoViewController.kHttp.count : 0)
    let dropFromEnd = str.hasSuffix("/") ? 1 : 0
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
