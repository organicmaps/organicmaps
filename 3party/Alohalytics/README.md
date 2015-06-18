Aloha, developer!

Alohalytics statistics engine solves following problems:
- Track any event from iOS/Android/C++ code (e.g. events from Java and native C++ code will be collected perfectly).
- Track basic events like iOS/Android app installs/updates/launches.
- Track basic devices information including IDFA and Google Play Advertising IDs.
- Collect all events while device is offline.

More features are coming soon!

If you have any questions, please contact Alex Zolotarev <me@alex.bio> from Minsk, Belarus.

iOS
======
Minimum supported iOS version is 5.1.

If you target iOS 5.1, then add an optional 'AdSupport.framework' to your binary to access IDFA (for iOS 6+ you can add it as 'required').

If your app uses background fetch, you can use ```[Alohalytics forceUpload];``` from ```application:performFetchWithCompletionHandler``` in your app delegate to improve events delivery.

App updates are detected by changing CURRENT_PROJECT_VERSION in CFBundleShortVersionString key in the plist file.

Built-in logged events
------
- $install
- $launch
- $update
- $iosDeviceIds
- $iosDeviceInfo
- $browserUserAgent
- $applicationDidBecomeActive
- $applicationWillResignActive
- $applicationWillEnterForeground
- $applicationDidEnterBackground
- $applicationWillTerminate


Android
======
Minimum supported Android version is 2.3 (API level 9).

Built-in logged events
------
- $install
- $launch
- $update
- $androidIds
- $androidDeviceInfo


Other platforms
======
Mac OS X should work perfectly. Linux has (untested) HTTP transport support via curl. Windows does not have native HTTP transport support.
C++ core requires C++11 compiler support.

nginx server setup example
======
```
http {
  log_format alohalytics  '$remote_addr [$time_local] "$request" $status $content_length "$http_user_agent" $content_type $http_content_encoding';
  server {
    listen 8080;              # <-- Change to actual server port.
    server_name localhost;    # <-- Change to actual server name.
    # To hide nginx version.
    server_tokens off;

    # Location starts with os version prefix to filter out some random web requests.
    location ~ ^/(ios|android|mac)/(.+)/(.+) {
      # Store for later use.
      set $OS $1;

      # Most filtering can be easily done on nginx side:
      # Our clients send only POST requests.
      limit_except POST { deny all; }
      # Content-Length should be valid, but it is checked anyway on FastCGI app's code side.
      # Content-Type should be "application/alohalytics-binary-blob"
      if ($content_type != "application/alohalytics-binary-blob") {
        return 415; # Unsupported Media Type
      }
      # Content-Encoding should be "gzip"
      if ($http_content_encoding != "gzip") {
        return 400; # Bad Request
      }
      client_body_buffer_size 1M;
      client_body_temp_path /tmp 2;     # <-- Change to writable directory which can temporarily store large POST bodies (rare case).
      client_max_body_size 100M;

      access_log /tmp/aloha-$OS-access.log alohalytics;       # <-- Change to actual log directory.
      # Unfortunately, error_log does not support variables.
      error_log  /tmp/aloha-error.log notice;                 # <-- Change to actual log directory.

      fastcgi_pass_request_headers on;
      fastcgi_param REMOTE_ADDR $remote_addr;
      fastcgi_param REQUEST_URI $request_uri;
      fastcgi_pass 127.0.0.1:8888;                            # <-- Change to actual FastCGI app address and port.
    }
  }
}

```
