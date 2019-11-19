protocol IWelcomeConfig {
  var image: UIImage? { get }
  var title: String { get }
  var text: String { get }
  var buttonNextTitle: String { get }
  var isCloseButtonHidden: Bool { get }
}

protocol IWelcomePresenter: class {
  func configure()
  func onAppear()
  func onNext()
  func onClose()
}
