enum SocialMedia {
  case telegram
  case twitter
  case instagram
  case facebook
  case reddit
  case matrix
  case fosstodon
  case linkedin
  case organicMapsEmail
  case github

  var link: String {
    switch self {
    case .telegram:
      return L("telegram_url")
    case .github:
      return "https://github.com/organicmaps/organicmaps/"
    case .linkedin:
      return "https://www.linkedin.com/company/organic-maps/"
    case .organicMapsEmail:
      return "ios@organicmaps.app"
    case .matrix:
      return "https://matrix.to/#/#organicmaps:matrix.org"
    case .fosstodon:
      return "https://fosstodon.org/@organicmaps"
    case .facebook:
      return "https://facebook.com/OrganicMaps"
    case .twitter:
      return "https://twitter.com/OrganicMapsApp"
    case .instagram:
      return L("instagram_url")
    case .reddit:
      return "https://www.reddit.com/r/organicmaps/"
    }
  }

  var image: UIImage {
    switch self {
    case .telegram:
      return UIImage(named: "ic_social_media_telegram")!
    case .github:
      return UIImage(named: "ic_social_media_github")!
    case .linkedin:
      return UIImage(named: "ic_social_media_linkedin")!
    case .organicMapsEmail:
      return UIImage(named: "ic_social_media_mail")!
    case .matrix:
      return UIImage(named: "ic_social_media_matrix")!
    case .fosstodon:
      return UIImage(named: "ic_social_media_fosstodon")!
    case .facebook:
      return UIImage(named: "ic_social_media_facebook")!
    case .twitter:
      return UIImage(named: "ic_social_media_x")!
    case .instagram:
      return UIImage(named: "ic_social_media_instagram")!
    case .reddit:
      return UIImage(named: "ic_social_media_reddit")!
    }
  }
}
