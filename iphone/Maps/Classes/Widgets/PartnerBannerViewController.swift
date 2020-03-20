@objc
final class PartnerBannerViewController: MWMDownloadBannerViewController {

  @IBOutlet var icon: UIImageView!
  @IBOutlet var message: UILabel!
  @IBOutlet var button: UIButton!
  
  var bannerType: MWMBannerType = .none
  
  @objc func config(type: MWMBannerType) {
    bannerType = type
  }
  
  override func viewDidLoad() {
    let iconImage = { () -> UIImage in
      switch self.bannerType {
       case .tinkoffAllAirlines: return UIImage(named: "ic_logo_tinkoff")!
       case .tinkoffInsurance: return UIImage(named: "ic_logo_tinkoff")!
       case .mts: return UIImage(named: "ic_logo_mts")!
       case .skyeng: return UIImage(named: "ic_logo_skyeng")!
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
     
     let getStyleName = { () -> String in
      switch self.bannerType {
       case .tinkoffAllAirlines: return "Tinkoff"
       case .tinkoffInsurance: return "Tinkoff"
       case .mts: return "Mts"
       case .skyeng: return "Skyeng"
       default: fatalError()
       }
     }
     
     icon.image = iconImage()
     message.text = messageString()
     button.localizedText = buttonString()
     button.styleName = getStyleName()
}
}
