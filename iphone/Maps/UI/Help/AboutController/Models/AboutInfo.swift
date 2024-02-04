enum AboutInfo {
  case faq
  case reportABug
  case reportMapDataProblem
  case volunteer
  case news
  case rateTheApp
  case noTracking
  case noWifi
  case community

  var title: String {
    switch self {

    case .faq:
      return L("faq")
    case .reportABug:
      return L("report_a_bug")
    case .reportMapDataProblem:
      return L("report_incorrect_map_bug")
    case .volunteer:
      return L("volunteer")
    case .news:
      return L("news")
    case .rateTheApp:
      return L("rate_the_app")
    case .noTracking:
      return L("about_proposition_1")
    case .noWifi:
      return L("about_proposition_2")
    case .community:
      return L("about_proposition_3")
    }
  }

  var image: UIImage? {
    switch self {
    case .faq:
      return UIImage(named: "ic_about_faq")!
    case .reportABug:
      return UIImage(named: "ic_about_report_bug")!
    case .reportMapDataProblem:
      return UIImage(named: "ic_about_report_osm")!
    case .volunteer:
      return UIImage(named: "ic_about_volunteer")!
    case .news:
      return UIImage(named: "ic_about_news")!
    case .rateTheApp:
      return UIImage(named: "ic_about_rate_app")!
    case .noTracking, .noWifi, .community:
      // Dots are used for these cases
      return nil
    }
  }

  var link: String? {
    switch self {
    case .faq, .rateTheApp, .noTracking, .noWifi, .community:
      // These cases don't provide redirection to the web
      return nil
    case .reportABug:
      return "ios@organicmaps.app"
    case .reportMapDataProblem:
      return "https://www.openstreetmap.org/fixthemap"
    case .volunteer:
      return L("translated_om_site_url") + "support-us/"
    case .news:
      return L("translated_om_site_url") + "news/"
    }
  }
}
