import SwiftUI

protocol TransportOptionsViewController: UIViewController {
  func update(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType)
}

@available(iOS 15, *)
final class TransportOptionsSegmentedControlViewModel: ObservableObject {
  @Published var options: [MWMRouterType]
  @Published var selected: MWMRouterType

  init(options: [MWMRouterType], selected: MWMRouterType) {
    self.options = options
    self.selected = selected
  }
}

@available(iOS 15, *)
final class TransportOptionsHostingController: UIHostingController<TransportOptionsSegmentedControlView>, TransportOptionsViewController {
  let viewModel: TransportOptionsSegmentedControlViewModel

  init(options: [MWMRouterType], selected: MWMRouterType, onSelect: @escaping (MWMRouterType) -> Void) {
    self.viewModel = TransportOptionsSegmentedControlViewModel(options: options, selected: selected)
    let view = TransportOptionsSegmentedControlView(viewModel: viewModel, onSelect: onSelect)
    super.init(rootView: view)
  }

  func update(transportOptions: [MWMRouterType], selectedRouterType: MWMRouterType) {
    guard viewModel.options != transportOptions || viewModel.selected != selectedRouterType else { return }
    viewModel.options = transportOptions
    viewModel.selected = selectedRouterType
  }

  @available(*, unavailable)
  required dynamic init?(coder aDecoder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
}

@available(iOS 15, *)
struct TransportOptionsSegmentedControlView: View {
  @ObservedObject var viewModel: TransportOptionsSegmentedControlViewModel
  let onSelect: (MWMRouterType) -> Void

  private enum Constants {
    static let spacing: CGFloat = 4
    static let padding: CGFloat = 6
    static let imagePadding: CGFloat = 4
    static let animationDuration: Double = kDefaultAnimationDuration / 2
  }

  @Namespace private var animation
  private let selectedOptionId = "selectedOptionId"
  private let impactGenerator = UIImpactFeedbackGenerator(style: .medium)

  var body: some View {
    HStack(spacing: Constants.spacing) {
      ForEach(viewModel.options, id: \.rawValue) { type in
        Button {
          impactGenerator.impactOccurred()
          withAnimation {
            viewModel.selected = type
          }
          onSelect(type)
        } label: {
          ZStack {
            if viewModel.selected == type {
              Capsule()
                .fill(Color(uiColor: .linkBlue()))
                .matchedGeometryEffect(id: selectedOptionId, in: animation)
            }
            Image(uiImage: type.image(for: viewModel.selected == type))
              .renderingMode(.template)
              .resizable()
              .scaledToFit()
              .padding(.vertical, Constants.imagePadding)
              .foregroundColor(viewModel.selected == type ? .white : Color(uiColor: .blackSecondaryText()))
          }
          .padding(Constants.padding)
          .frame(maxWidth: .infinity)
          .animation(.easeIn(duration: Constants.animationDuration), value: viewModel.selected)
        }
        .buttonStyle(.plain)
      }
    }
    .frame(maxWidth: .infinity)
    .background(Color(uiColor: .pressBackground()))
    .clipShape(Capsule())
  }
}
