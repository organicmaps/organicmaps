class TabViewController: MWMViewController {
  var viewControllers: [UIViewController] = [] {
    didSet {
      viewControllers.forEach {
        self.addChild($0)
        $0.didMove(toParent: self)
      }
    }
  }

  var tabView: TabView {
    get {
      return view as! TabView
    }
  }

  override func loadView() {
    let v = TabView()
    v.dataSource = self
    view = v
  }
}

extension TabViewController: TabViewDataSource {
  func numberOfPages(in tabView: TabView) -> Int {
    return viewControllers.count
  }

  func tabView(_ tabView: TabView, viewAt index: Int) -> UIView {
    return viewControllers[index].view
  }

  func tabView(_ tabView: TabView, titleAt index: Int) -> String? {
    return viewControllers[index].title
  }
}
