class TabViewController: MWMViewController {
  var viewControllers: [UIViewController] = [] {
    didSet {
      for viewController in viewControllers {
        addChild(viewController)
        viewController.didMove(toParent: self)
      }
    }
  }

  var tabView: TabView {
    view as! TabView
  }

  override func loadView() {
    let v = TabView()
    v.dataSource = self
    view = v
  }
}

extension TabViewController: TabViewDataSource {
  func numberOfPages(in _: TabView) -> Int {
    viewControllers.count
  }

  func tabView(_: TabView, viewAt index: Int) -> UIView {
    viewControllers[index].view
  }

  func tabView(_: TabView, titleAt index: Int) -> String? {
    viewControllers[index].title
  }
}
