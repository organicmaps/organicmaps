android_resource(
  name = 'res',
  res = 'res',
  package = 'com.facebook',
  deps = [
    '//facebook:android-sdk',
  ],
)

android_library(
  name = 'lib',
  srcs = glob(['src/**/*.java']),
  deps = [
    ':res',
    '//facebook:android-sdk',
    '//libs:android-support-v4',
  ],
)

android_binary(
  name = 'app',
  manifest = 'AndroidManifest.xml',
  target = 'android-16',
  keystore = '//keystores:debug',
  package_type = 'debug',
  deps = [
    ':lib',
    '//facebook:android-sdk',
    '//libs:android-support-v4',
  ],
)
