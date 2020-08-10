@objc class DownloadBannerViewController: UIViewController {
  private let tapHandler: MWMVoidBlock

  @objc init(tapHandler: @escaping MWMVoidBlock) {
    self.tapHandler = tapHandler
    super.init(nibName: nil, bundle: nil)
  }

  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  @IBAction func onButtonTap(_ sender: UIButton) {
    tapHandler()
  }
}
