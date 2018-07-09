#
# Be sure to run `pod lib lint MoPub-Facebook-Adapters.podspec' to ensure this is a
# valid spec before submitting.
#

Pod::Spec.new do |s|
s.name             = 'MoPub-FacebookAudienceNetwork-Adapters'
s.version          = '4.28.1.2'
s.summary          = 'Facebook Adapters for mediating through MoPub.'
s.description      = <<-DESC
Supported ad formats: Banners, Interstitial, Rewarded Video and Native.\n
To download and integrate the Facebook SDK, please check https://developers.facebook.com/docs/audience-network/ios/#sdk. \n\n
For inquiries and support, please visit https://developers.facebook.com/products/audience-network/faq/. \n
DESC
s.homepage         = 'https://github.com/mopub/mopub-ios-mediation'
s.license          = { :type => 'New BSD', :file => 'LICENSE' }
s.author           = { 'MoPub' => 'support@mopub.com' }
s.source           = { :git => 'https://github.com/mopub/mopub-ios-mediation.git', :commit => 'master' }
s.ios.deployment_target = '8.0'
s.static_framework = true
s.source_files = 'FacebookAudienceNetwork/*.{h,m}'
s.dependency 'mopub-ios-sdk', '~> 5.0'
s.dependency 'FBAudienceNetwork', '4.28.1'
end
