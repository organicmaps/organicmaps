@objc class PartnerBannerBuilder: NSObject {
  @objc static func build(type: MWMBannerType, tapHandler: @escaping MWMVoidBlock) -> UIViewController {
    guard let viewModel = PartnerBannerViewModel(type: type) else {
      fatalError("Unknown config for: \(type)")
    }

    let viewController: UIViewController
    switch viewModel.type {
    case .single:
      viewController = PartnerBannerViewController(model: viewModel, tapHandler: tapHandler)
    case .multiple:
      viewController = MultiPartnerBannerViewController(model: viewModel, tapHandler: tapHandler)
    }
    return viewController
  }
}
