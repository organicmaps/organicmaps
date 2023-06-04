@objc
final class PartnerBannerViewController: DownloadBannerViewController {
  @IBOutlet var icon: UIImageView!
  @IBOutlet var message: UILabel!
  @IBOutlet var button: UIButton!

  private let model: PartnerBannerViewModel

  init(model: PartnerBannerViewModel, tapHandler: @escaping MWMVoidBlock) {
    self.model = model
    super.init(tapHandler: tapHandler)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    icon.image = UIImage(named: model.images[0])
    message.text = model.message
    button.localizedText = model.button
    button.styleName = model.style
  }
}
