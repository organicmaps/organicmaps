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
        imageView?.mwm_coloring = .black
        infoLabel?.textColor = UIColor.blackPrimaryText()
      case .link:
        imageView?.mwm_coloring = .blue
        infoLabel?.textColor = UIColor.linkBlue()
      }
    }
  }

  @IBAction func onTap(_ sender: UITapGestureRecognizer) {
    tapHandler?()
  }

  override func viewDidLoad() {
    super.viewDidLoad()

    if style == .link {
      imageView.mwm_coloring = .blue
      infoLabel.textColor = UIColor.linkBlue()
    }
  }
}

protocol PlacePageInfoViewControllerDelegate: AnyObject {
  func didPressCall()
  func didPressWebsite()
  func didPressEmail()
  func didPressLocalAd()
}

class PlacePageInfoViewController: UIViewController {
  private typealias TapHandler = InfoItemViewController.TapHandler
  private typealias Style = InfoItemViewController.Style

  @IBOutlet var stackView: UIStackView!

  private var openingHoursView: InfoItemViewController?
  private var phoneView: InfoItemViewController?
  private var websiteView: InfoItemViewController?
  private var emailView: InfoItemViewController?
  private var cuisineView: InfoItemViewController?
  private var operatorView: InfoItemViewController?
  private var wifiView: InfoItemViewController?
  private var addressView: InfoItemViewController?
  private var coordinatesView: InfoItemViewController?
  private var localAdsButton: UIButton?

  var placePageInfoData: PlacePageInfoData!
  weak var delegate: PlacePageInfoViewControllerDelegate?
  
  override func viewDidLoad() {
    super.viewDidLoad()

    if let openingHoursString = placePageInfoData.openingHoursString {
      openingHoursView = createInfoItem(openingHoursString, icon: UIImage(named: "ic_placepage_open_hours")) {

      }
      openingHoursView?.accessoryImage.image = UIImage(named: "ic_arrow_gray_down")
      openingHoursView?.accessoryImage.isHidden = false
    }

    if let phone = placePageInfoData.phone {
      var cellStyle: Style = .regular
      if let phoneUrl = placePageInfoData.phoneUrl, UIApplication.shared.canOpenURL(phoneUrl) {
        cellStyle = .link
      }
      phoneView = createInfoItem(phone, icon: UIImage(named: "ic_placepage_phone_number"), style: cellStyle) { [weak self] in
        self?.delegate?.didPressCall()
      }
    }

    if let website = placePageInfoData.website {
      websiteView = createInfoItem(website, icon: UIImage(named: "ic_placepage_website"), style: .link) { [weak self] in
        self?.delegate?.didPressWebsite()
      }
    }

    if let email = placePageInfoData.email {
      emailView = createInfoItem(email, icon: UIImage(named: "ic_placepage_email"), style: .link) { [weak self] in
        self?.delegate?.didPressEmail()
      }
    }

    if let cuisine = placePageInfoData.cuisine {
      cuisineView = createInfoItem(cuisine, icon: UIImage(named: "ic_placepage_cuisine"))
    }

    if let ppOperator = placePageInfoData.ppOperator {
      operatorView = createInfoItem(ppOperator, icon: UIImage(named: "ic_placepage_operator"))
    }

    if placePageInfoData.wifiAvailable {
      wifiView = createInfoItem(L("WiFi_available"), icon: UIImage(named: "ic_placepage_wifi"))
    }

    if let address = placePageInfoData.address {
      addressView = createInfoItem(address, icon: UIImage(named: "ic_placepage_adress"))
    }

    if let coordinates = placePageInfoData.formattedCoordinates {
      coordinatesView = createInfoItem(coordinates, icon: UIImage(named: "ic_placepage_coordinate")) {

      }
      coordinatesView?.accessoryImage.image = UIImage(named: "ic_placepage_change")
      coordinatesView?.accessoryImage.isHidden = false
    }

    switch placePageInfoData.localAdsStatus {
    case .candidate:
      localAdsButton = createLocalAdsButton(L("create_campaign_button"))
    case .customer:
      localAdsButton = createLocalAdsButton(L("view_campaign_button"))
    case .notAvailable, .hidden:
      coordinatesView?.separatorView.isHidden = true
    @unknown default:
      fatalError()
    }
  }

  // MARK: private

  @objc private func onLocalAdsButton(_ sender: UIButton) {
    delegate?.didPressLocalAd()
  }

  private func createLocalAdsButton(_ title: String) -> UIButton {
    let button = UIButton()
    button.setTitle(title, for: .normal)
    button.titleLabel?.font = UIFont.regular17()
    button.setTitleColor(UIColor.linkBlue(), for: .normal)
    button.heightAnchor.constraint(equalToConstant: 44).isActive = true
    stackView.addArrangedSubview(button)
    button.addTarget(self, action: #selector(onLocalAdsButton(_:)), for: .touchUpInside)
    return button
  }

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
