import UIKit
import SwiftUI
import PhotosUI

class PersonalDataViewController: UIViewController {
  private var profileVM: ProfileViewModel
  
  init(profileVM: ProfileViewModel) {
    self.profileVM = profileVM
    super.init(
      nibName: nil,
      bundle: nil
    )
  }
  
  required init?(coder: NSCoder) {
    fatalError("init(coder:) has not been implemented")
  }
  
  override func viewDidLoad() {
    super.viewDidLoad()
    integrateSwiftUIScreen(PersonalDataScreen(profileVM: profileVM))
  }
}

struct PersonalDataScreen: View {
  @ObservedObject var profileVM: ProfileViewModel
  @Environment(\.presentationMode) var presentationMode: Binding<PresentationMode>
  
  @State private var showImagePicker = false
  
  var body: some View {
    ScrollView {
      VStack(alignment: .leading) {
        AppTopBar(
          title: L("personal_data"),
          onBackClick: {
            presentationMode.wrappedValue.dismiss()
          }
        )
        VerticalSpace(height: 24)
        
        HStack(alignment: .center) {
          // pfp
          Group {
            if profileVM.isImagePickerUsed {
              Image(uiImage: profileVM.pfpToUpload)
                .resizable()
            } else {
              LoadImageView(url: profileVM.pfpFromRemote?.absoluteString)
            }
          }
          .scaledToFill()
          .frame(width: 100, height: 100)
          .background(Color.surface)
          .clipShape(Circle())
          
          Spacer().frame(width: 32)
          
          // photo picker
          Group {
            Image(systemName: "photo.badge.arrow.down")
              .foregroundColor(Color.onBackground)
            Spacer().frame(width: 8)
            
            Text(L("upload_photo"))
              .textStyle(TextStyle.h4)
          }
          .onTapGesture {
            showImagePicker = true
          }
        }
        VerticalSpace(height: 36)
        
        AppTextField(
          value: $profileVM.fullName,
          hint: L("full_name"),
          label: L("full_name")
        )
        VerticalSpace(height: 16)
        
        AppTextField(
          value: $profileVM.email,
          hint: L("email"),
          label: L("email")
        )
        VerticalSpace(height: 8)
        
        if let code = profileVM.countryCodeName {
          Text(L("country"))
            .font(.system(size: 13))
            .foregroundColor(Color.onBackground)
          
          UICountryPickerView(
            code: code,
            onCountryChanged: { code in
              profileVM.setCountryCodeName(code)
            }
          )
            .frame(height: 56)
            .overlay(
              // Underline
              Rectangle()
                .frame(height: 1)
                .foregroundColor(Color.onBackground)
                .padding(.top, 50 / 2)
            )
          
          VerticalSpace(height: 32)
        }
        
        PrimaryButton(
          label: L("save"),
          onClick: {
            profileVM.save()
          }
        )
      }
      .padding()
      .sheet(isPresented: $showImagePicker) {
        ImagePicker(sourceType: .photoLibrary, selectedImage: $profileVM.pfpToUpload)
      }
    }
    .overlay(
      Group {
        if profileVM.shouldShowMessageOnPersonalDataScreen {
          ToastView(message: profileVM.messageToShowOnPersonalDataScreen, isPresented: $profileVM.shouldShowMessageOnPersonalDataScreen)
            .padding(.bottom)
        }
      },
      alignment: .bottom
    )
    .background(Color.background)
  }
}

