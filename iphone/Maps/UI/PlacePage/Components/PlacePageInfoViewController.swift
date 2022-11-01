class InfoItemViewController: UIViewController {
  enum Style {
    case regular
    case link
  }

  typealias TapHandler = () -> Void

  @IBOutlet var imageView: UIImageView!
  @IBOutlet var infoLabel: UILabel!
  @IBOutlet var accessoryImage: UIImageView!
  @IBOutlet var separatorView: UIView!
  @IBOutlet var tapGestureRecognizer: UITapGestureRecognizer!

  var tapHandler: TapHandler?
  var style: Style = .regular {
    didSet {
      switch style {
      case .regular:
        imageView?.styleName = "MWMBlack"
        infoLabel?.styleName = "blackPrimaryText"
      case .link:
        imageView?.styleName = "MWMBlue"
        infoLabel?.styleName = "linkBlueText"
      }
    }
  }
  var canShowMenu = false

  @IBAction func onTap(_ sender: UITapGestureRecognizer) {
    tapHandler?()
  }

  @IBAction func onLongPress(_ sender: UILongPressGestureRecognizer) {
    guard sender.state == .began, canShowMenu else { return }
    let menuController = UIMenuController.shared
    menuController.setTargetRect(infoLabel.frame, in: self.view)
    infoLabel.becomeFirstResponder()
    menuController.setMenuVisible(true, animated: true)
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    if style == .link {
      imageView.styleName = "MWMBlue"
      infoLabel.styleName = "linkBlueText"
    }
  }
}

protocol PlacePageInfoViewControllerDelegate: AnyObject {
  func didPressCall()
  func didPressWebsite()
  func didPressWikipedia()
  func didPressWikimediaCommons()
  func didPressFacebook()
  func didPressInstagram()
  func didPressTwitter()
  func didPressVk()
  func didPressEmail()
}

class PlacePageInfoViewController: UIViewController {
  private struct Const {
    static let coordinatesKey = "PlacePageInfoViewController_coordinatesKey"
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
  private var wikipediaView: InfoItemViewController?
  private var wikimediaCommonsView: InfoItemViewController?
  private var emailView: InfoItemViewController?
  private var facebookView: InfoItemViewController?
  private var instagramView: InfoItemViewController?
  private var twitterView: InfoItemViewController?
  private var vkView: InfoItemViewController?
  private var cuisineView: InfoItemViewController?
  private var operatorView: InfoItemViewController?
  private var wifiView: InfoItemViewController?
  private var addressView: InfoItemViewController?
  private var levelView: InfoItemViewController?
  private var coordinatesView: InfoItemViewController?

  var placePageInfoData: PlacePageInfoData!
  weak var delegate: PlacePageInfoViewControllerDelegate?
  var showFormattedCoordinates: Bool {
    get {
      UserDefaults.standard.bool(forKey: Const.coordinatesKey)
    }
    set {
      UserDefaults.standard.set(newValue, forKey: Const.coordinatesKey)
    }
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()

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
      phoneView = createInfoItem(phone, icon: UIImage(named: "ic_placepage_phone_number"), style: cellStyle) { [weak self] in
        self?.delegate?.didPressCall()
      }
    }

    if let ppOperator = placePageInfoData.ppOperator {
      operatorView = createInfoItem(ppOperator, icon: UIImage(named: "ic_placepage_operator"))
    }

    if let website = placePageInfoData.website {
      websiteView = createInfoItem(website, icon: UIImage(named: "ic_placepage_website"), style: .link) { [weak self] in
        self?.delegate?.didPressWebsite()
      }
    }
    
    if let wikipedia = placePageInfoData.wikipedia {
      wikipediaView = createInfoItem(L("read_in_wikipedia"), icon: UIImage(named: "ic_placepage_wiki"), style: .link) { [weak self] in
        self?.delegate?.didPressWikipedia()
      }
    }
    
    if let wikimediaCommons = placePageInfoData.wikimediaCommons {
      wikimediaCommonsView = createInfoItem(L("wikimedia_commons"), icon: UIImage(named: "ic_placepage_wikimedia_commons"), style: .link) { [weak self] in
        self?.delegate?.didPressWikimediaCommons()
      }
    }

    if let wifi = placePageInfoData.wifiAvailable {
      wifiView = createInfoItem(wifi, icon: UIImage(named: "ic_placepage_wifi"))
    }

    if let level = placePageInfoData.level {
      levelView = createInfoItem(level, icon: UIImage(named: "ic_placepage_level"))
    }

    if let email = placePageInfoData.email {
      emailView = createInfoItem(email, icon: UIImage(named: "ic_placepage_email"), style: .link) { [weak self] in
        self?.delegate?.didPressEmail()
      }
    }
    
    if let facebook = placePageInfoData.facebook {
      facebookView = createInfoItem("@" + facebook, icon: UIImage(named: "ic_placepage_facebook"), style: .link) { [weak self] in
        self?.delegate?.didPressFacebook()
      }
    }
    
    if let instagram = placePageInfoData.instagram {
      instagramView = createInfoItem("@" + instagram, icon: UIImage(named: "ic_placepage_instagram"), style: .link) { [weak self] in
        self?.delegate?.didPressInstagram()
      }
    }
    
    if let twitter = placePageInfoData.twitter {
      twitterView = createInfoItem("@" + twitter, icon: UIImage(named: "ic_placepage_twitter"), style: .link) { [weak self] in
        self?.delegate?.didPressTwitter()
      }
    }
    
    if let vk = placePageInfoData.vk {
      vkView = createInfoItem("@" + vk, icon: UIImage(named: "ic_placepage_vk"), style: .link) { [weak self] in
        self?.delegate?.didPressVk()
      }
    }

    if let address = placePageInfoData.address {
      addressView = createInfoItem(address, icon: UIImage(named: "ic_placepage_adress"))
      addressView?.canShowMenu = true
    }

    if let formattedCoordinates = placePageInfoData.formattedCoordinates,
      let rawCoordinates = placePageInfoData.rawCoordinates {
      let coordinates = showFormattedCoordinates ? formattedCoordinates : rawCoordinates
      coordinatesView = createInfoItem(coordinates, icon: UIImage(named: "ic_placepage_coordinate")) {
        [unowned self] in
        self.showFormattedCoordinates = !self.showFormattedCoordinates
        let coordinates = self.showFormattedCoordinates ? formattedCoordinates : rawCoordinates
        self.coordinatesView?.infoLabel.text = coordinates
      }
    } else if let formattedCoordinates = placePageInfoData.formattedCoordinates {
      coordinatesView = createInfoItem(formattedCoordinates, icon: UIImage(named: "ic_placepage_coordinate"))
    } else if let rawCoordinates = placePageInfoData.rawCoordinates {
      coordinatesView = createInfoItem(rawCoordinates, icon: UIImage(named: "ic_placepage_coordinate"))
    }

    coordinatesView?.accessoryImage.image = UIImage(named: "ic_placepage_change")
    coordinatesView?.accessoryImage.isHidden = false
    coordinatesView?.canShowMenu = true
  }

  // MARK: private

  private func createInfoItem(_ info: String,
                              icon: UIImage?,
                              style: Style = .regular,
                              tapHandler: TapHandler? = nil) -> InfoItemViewController {
    let vc = storyboard!.instantiateViewController(ofType: InfoItemViewController.self)
    addToStack(vc)
    vc.imageView.image = icon
    vc.infoLabel.text = info
    vc.style = style
    vc.tapHandler = tapHandler
    return vc;
  }

  private func addToStack(_ viewController: UIViewController) {
    addChild(viewController)
    stackView.addArrangedSubview(viewController.view)
    viewController.didMove(toParent: self)
  }
}
