enum BannerType {
  case none
  case facebook(String)
  case rb(String)
  case mopub(String)
  case google(String)

  var banner: Banner? {
    switch self {
    case .none: return nil
    case .facebook(let id): return FacebookBanner(bannerID: id)
    case .rb(let id): return RBBanner(bannerID: id)
    case .mopub(let id): return MopubBanner(bannerID: id)
    case .google: return nil
    }
  }

  var mwmType: MWMBannerType {
    switch self {
    case .none: return .none
    case .facebook: return .facebook
    case .rb: return .rb
    case .mopub: return .mopub
    case .google: return .google
    }
  }

  init(type: MWMBannerType, id: String) {
    switch type {
    case .none: self = .none
    case .facebook: self = .facebook(id)
    case .rb: self = .rb(id)
    case .mopub: self = .mopub(id)
    case .google: self = .google(id)
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
    case let (.google(l), .google(r)): return l == r
    case (.none, _),
         (.facebook, _),
         (.rb, _),
         (.mopub, _),
         (.google, _): return false
    }
  }
}

extension BannerType: Hashable {
  var hashValue: Int {
    switch self {
    case .none: return mwmType.hashValue
    case .facebook(let id): return mwmType.hashValue ^ id.hashValue
    case .rb(let id): return mwmType.hashValue ^ id.hashValue
    case .mopub(let id): return mwmType.hashValue ^ id.hashValue
    case .google(let id): return mwmType.hashValue ^ id.hashValue
    }
  }
}
