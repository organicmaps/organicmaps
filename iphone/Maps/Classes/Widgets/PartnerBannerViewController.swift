@objc
final class PartnerBannerViewController: MWMDownloadBannerViewController {

  @IBOutlet weak var icon: UIImageView!
  @IBOutlet weak var message: UILabel!
  @IBOutlet weak var button: UIButton!
  
  var bannerType: MWMBannerType = .none
  
  @objc func config(type: MWMBannerType) {
    bannerType = type
  }
  
  override func viewDidLoad() {
    let iconImage = { () -> UIImage in
      switch self.bannerType {
       case .tinkoffAllAirlines: return #imageLiteral(resourceName: "ic_logo_tinkoff")
       case .tinkoffInsurance: return #imageLiteral(resourceName: "ic_logo_tinkoff")
       case .mts: return #imageLiteral(resourceName: "ic_logo_mts")
       case .skyeng: return #imageLiteral(resourceName: "ic_logo_skyeng")
       default: fatalError()
       }
     }
     
     let messageString = { () -> String in
      switch self.bannerType {
       case .tinkoffAllAirlines: return L("tinkoff_allairlines_map_downloader_title")
       case .tinkoffInsurance: return L("tinkoff_insurance_map_downloader_title")
       case .mts: return L("mts_map_downloader_title")
       case .skyeng: return L("skyeng_map_downloader_title")
       default: fatalError()
       }
     }
     
     let buttonString = { () -> String in
      switch self.bannerType {
       case .tinkoffAllAirlines: return L("tinkoff_allairlines_map_downloader_cta_button")
       case .tinkoffInsurance: return L("tinkoff_insurance_map_downloader_cta_button")
       case .mts: return L("mts_map_downloader_cta_button")
       case .skyeng: return L("skyeng_map_downloader_cta_button")
       default: fatalError()
       }
     }
     
     let buttonBackgroundColor = { () -> UIColor in
      switch self.bannerType {
       case .tinkoffAllAirlines: return UIColor.init(fromHexString: "FFDD2D")
       case .tinkoffInsurance: return UIColor.init(fromHexString: "FFDD2D")
       case .mts: return UIColor.init(fromHexString: "E30611")
       case .skyeng: return UIColor.init(fromHexString: "4287DF")
       default: fatalError()
       }
     }
     
     let buttonTextColor = { () -> UIColor in
      switch self.bannerType {
       case .tinkoffAllAirlines: return .black
       case .tinkoffInsurance: return .black
       case .mts: return .white
       case .skyeng: return .white
       default: fatalError()
       }
     }
     
     icon.image = iconImage()
     message.text = messageString()
     button.localizedText = buttonString()
     button.backgroundColor = buttonBackgroundColor()
     button.titleLabel?.textColor = buttonTextColor()
}
}
