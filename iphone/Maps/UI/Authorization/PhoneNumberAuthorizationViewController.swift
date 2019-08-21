class PhoneNumberAuthorizationViewController: WebViewController {
  var failure: MWMVoidBlock?

  init(success: @escaping MWMStringBlock, failure: @escaping MWMVoidBlock) {
    super.init(authURL: MWMAuthorizationViewModel.phoneAuthURL()!, onSuccessAuth: success, onFailure: failure)!
    self.failure = failure
  }
  
  required init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }

  override func viewDidLoad() {
    super.viewDidLoad()
    let cancelButton = UIBarButtonItem(barButtonSystemItem: .cancel, target: self, action: #selector(onCancel(_:)))
    navigationItem.leftBarButtonItem = cancelButton
  }

  @objc func onCancel(_ sender: UIBarButtonItem) {
    failure?()
  }
}
