import SwiftUI

struct HorizontalSpace: View {
    let width: CGFloat
    
    var body: some View {
        Spacer().frame(width: width)
    }
}

struct VerticalSpace: View {
    let height: CGFloat
    
    var body: some View {
        Spacer().frame(height: height)
    }
}
