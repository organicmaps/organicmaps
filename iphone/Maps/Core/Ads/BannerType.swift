enum BannerType {
  case none
  case facebook(String)
  case rb(String)
  case mopub(String)

  var banner: Banner? {
    switch self {
    case .none: return nil
    case let .facebook(id): return FacebookBanner(bannerID: id)
    case let .rb(id): return RBBanner(bannerID: id)
    case let .mopub(id): return MopubBanner(bannerID: id)
    }
  }

  var mwmType: MWMBannerType {
    switch self {
    case .none: return .none
    case .facebook: return .facebook
    case .rb: return .rb
    case .mopub: return .mopub
    }
  }

  init(type: MWMBannerType, id: String, query: String = "") {
    switch type {
    case .facebook: self = .facebook(id)
    case .rb: self = .rb(id)
    case .mopub: self = .mopub(id)
    default: self = .none
    }
  }
}

extension BannerType: Equatable {
  static func ==(lhs: BannerType, rhs: BannerType) -> Bool {
    switch (lhs, rhs) {
    case (.none, .none): return true
    case let (.facebook(l), .facebook(r)): return l == r
    case let (.rb(l), .rb(r)): return l == r
    case let (.mopub(l), .mopub(r)): return l == r
    case (.none, _),
         (.facebook, _),
         (.rb, _),
         (.mopub, _): return false
    }
  }
}

extension BannerType: Hashable {
  func hash(into hasher: inout Hasher) {
    switch self {
    case .none: hasher.combine(mwmType.hashValue)
    case let .facebook(id): hasher.combine(mwmType.hashValue ^ id.hashValue)
    case let .rb(id): hasher.combine(mwmType.hashValue ^ id.hashValue)
    case let .mopub(id): hasher.combine(mwmType.hashValue ^ id.hashValue)
    }
  }
}
