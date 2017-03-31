import Foundation

enum BannerType {
  case none
  case facebook(String)
  case rb(String)

  var banner: Banner? {
    switch self {
    case .none: return nil
    case .facebook(let id): return FacebookBanner(bannerID: id)
    case .rb(let id): return RBBanner(bannerID: id)
    }
  }

  var statisticsDescription: [String: String] {
    switch self {
    case .none: return [:]
    case .facebook(let id): return [kStatBanner: id, kStatProvider: kStatFacebook]
    case .rb(let id): return [kStatBanner: id, kStatProvider: kStatRB]
    }
  }

  var mwmType: MWMBannerType {
    switch self {
    case .none: return .none
    case .facebook: return .facebook
    case .rb: return .rb
    }
  }

  init(type: MWMBannerType, id: String) {
    switch type {
    case .none: self = .none
    case .facebook: self = .facebook(id)
    case .rb: self = .rb(id)
    }
  }
}

extension BannerType: Equatable {
  static func ==(lhs: BannerType, rhs: BannerType) -> Bool {
    switch (lhs, rhs) {
    case (.none, .none): return true
    case let (.facebook(l), .facebook(r)): return l == r
    case let (.rb(l), .rb(r)): return l == r
    case (.none, _),
         (.facebook, _),
         (.rb, _): return false
    }
  }
}

extension BannerType: Hashable {
  var hashValue: Int {
    switch self {
    case .none: return mwmType.hashValue
    case .facebook(let id): return mwmType.hashValue ^ id.hashValue
    case .rb(let id): return mwmType.hashValue ^ id.hashValue
    }
  }
}
