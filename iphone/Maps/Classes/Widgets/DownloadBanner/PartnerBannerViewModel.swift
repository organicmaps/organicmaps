struct PartnerBannerViewModel {
  enum BannerType {
    case single
    case multiple
  }

  let images: [String]
  let message: String
  let button: String
  let style: String
  let type: BannerType
}

extension PartnerBannerViewModel {
  init? (type: MWMBannerType) {
    switch type {
    case .tinkoffAllAirlines:
      self.init(images: ["ic_logo_tinkoff"],
                message: L("tinkoff_allairlines_map_downloader_title"),
                button: L("tinkoff_allairlines_map_downloader_cta_button"),
                style: "Tinkoff",
                type: .single)
    case .tinkoffInsurance:
      self.init(images: ["ic_logo_tinkoff"],
                message: L("tinkoff_insurance_map_downloader_title"),
                button: L("tinkoff_insurance_map_downloader_cta_button"),
                style: "Tinkoff",
                type: .single)
    case .mts:
      self.init(images: ["ic_logo_mts"],
                message: L("mts_map_downloader_title"),
                button: L("mts_map_downloader_cta_button"),
                style: "Mts",
                type: .single)
    case .skyeng:
      self.init(images: ["ic_logo_skyeng"],
                message: L("skyeng_map_downloader_title"),
                button: L("skyeng_map_downloader_cta_button"),
                style: "Skyeng",
                type: .single)
    case .mastercardSberbank:
      self.init(images: ["ic_logo_sberbank", "ic_logo_mastercard"],
                message: L("sberbank_map_downloader_title"),
                button: L("sberbank_map_downloader_cta_button"),
                style: "Sberbank",
                type: .multiple)
    case .arsenalMedic:
      self.init(images: ["ic_logo_arsenal"],
                message: L("arsenal_telemed_map_downloader_title"),
                button: L("arsenal_cta_button"),
                style: "Arsenal",
                type: .single)
    case . arsenalFlat:
      self.init(images: ["ic_logo_arsenal"],
                message: L("arsenal_flat_map_downloader_title"),
                button: L("arsenal_cta_button"),
                style: "Arsenal",
                type: .single)
    case . arsenalInsuranceCrimea:
      self.init(images: ["ic_logo_arsenal"],
                message: L("arsenal_crimea_map_downloader_title"),
                button: L("arsenal_cta_button"),
                style: "Arsenal",
                type: .single)
    case . arsenalInsuranceRussia:
      self.init(images: ["ic_logo_arsenal"],
                message: L("arsenal_russia_map_downloader_title"),
                button: L("arsenal_cta_button"),
                style: "Arsenal",
                type: .single)
    case . arsenalInsuranceWorld:
      self.init(images: ["ic_logo_arsenal"],
                message: L("arsenal_abroad_map_downloader_title"),
                button: L("arsenal_cta_button"),
                style: "Arsenal",
                type: .single)
    default:
      return nil
    }
  }
}
