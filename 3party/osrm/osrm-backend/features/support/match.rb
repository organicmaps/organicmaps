require 'net/http'

HOST = "http://127.0.0.1:#{OSRM_PORT}"

def request_matching trace=[], timestamps=[], options={}
  defaults = { 'output' => 'json' }
  locs = trace.compact.map { |w| "loc=#{w.lat},#{w.lon}" }
  ts = timestamps.compact.map { |t| "t=#{t}" }
  if ts.length > 0
    trace_params = locs.zip(ts).map { |a| a.join('&')}
  else
    trace_params = locs
  end
  params = (trace_params + defaults.merge(options).to_param).join('&')
  params = nil if params==""
  uri = URI.parse ["#{HOST}/match", params].compact.join('?')
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

