protocol ModalPresentationStepStrategy<Step>: Equatable {
  associatedtype Step: CaseIterable & Equatable

  var steps: [Step] { get }

  func upperTo(_ step: Step) -> Step
  func lowerTo(_ step: Step) -> Step
  func frame(_ step: Step, for presentedView: UIView, in containerViewController: UIViewController) -> CGRect
}

extension ModalPresentationStepStrategy {
  var steps: [Step] {
    Array(Step.allCases)
  }
}
