protocol ModalPresentationStepStrategy<Step> : Equatable {
  associatedtype Step

  func upperTo(_ step: Step) -> Step
  func lowerTo(_ step: Step) -> Step
  func frame(_ step: Step, for presentedView: UIView, in containerViewController: UIViewController) -> CGRect
}
