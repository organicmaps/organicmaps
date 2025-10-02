
final class SearchNoResultsViewController: MWMViewController {

  static var controller: SearchNoResultsViewController {
    let storyboard = UIStoryboard.instance(.main)
    return storyboard.instantiateViewController(withIdentifier: toString(self)) as! SearchNoResultsViewController
  }

  @IBOutlet private weak var container: UIView!
  @IBOutlet private weak var containerCenterYConstraint: NSLayoutConstraint!

  override func viewDidLoad() {
    super.viewDidLoad()

    container.addSubview(MWMSearchNoResults.view(with: nil,
                                                 title: L("search_not_found"),
                                                 text: L("search_not_found_query")))
    MWMKeyboard.add(self)
    onKeyboardAnimation()
  }
}

extension SearchNoResultsViewController: MWMKeyboardObserver {
  func onKeyboardAnimation() {
    view.animateConstraints {
      self.containerCenterYConstraint.constant = -MWMKeyboard.keyboardHeight() / 3
    }
  }
}
