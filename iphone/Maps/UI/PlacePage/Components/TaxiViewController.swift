protocol TaxiViewControllerDelegate: AnyObject {
  func didPressOrder()
  func didShowTaxi()
}

class TaxiViewController: UIViewController {
  @IBOutlet var taxiImageView: UIImageView!
  @IBOutlet var taxiNameLabel: UILabel!

  var taxiProvider: PlacePageTaxiProvider = .none
  weak var delegate: TaxiViewControllerDelegate?

  override func viewDidLoad() {
    super.viewDidLoad()

    switch taxiProvider {
    case .none:
      assertionFailure()
    case .uber:
      taxiImageView.image = UIImage(named: "icTaxiUber")
      taxiNameLabel.text = L("uber")
    case .yandex:
      taxiImageView.image = UIImage(named: "ic_taxi_logo_yandex")
      taxiNameLabel.text = L("yandex_taxi_title")
    case .maxim:
      taxiImageView.image = UIImage(named: "ic_taxi_logo_maksim")
      taxiNameLabel.text = L("maxim_taxi_title")
    case .rutaxi:
      taxiImageView.image = UIImage(named: "ic_taxi_logo_vezet")
      taxiNameLabel.text = L("vezet_taxi")
    case .freenow:
      taxiImageView.image = UIImage(named: "ic_logo_freenow")
      taxiNameLabel.text = L("freenow_taxi_title")
    case .yango:
      taxiImageView.image = UIImage(named: "ic_taxi_logo_yango")
      taxiNameLabel.text = L("yango_taxi_title")
    case .citymobil:
      taxiImageView.image = UIImage(named: "ic_taxi_logo_citymobil_light")
      taxiNameLabel.text = L("citymobil_taxi_title")
    @unknown default:
      fatalError()
    }
    
    delegate?.didShowTaxi()
  }

  @IBAction func onOrder(_ sender: UIButton) {
    delegate?.didPressOrder()
  }
}
