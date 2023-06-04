require 'net/http'

def request_nearest_url path
  @query = path
  uri = URI.parse "#{HOST}/#{path}"
  Timeout.timeout(OSRM_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

def request_nearest a
  request_nearest_url "nearest?loc=#{a}"
end
