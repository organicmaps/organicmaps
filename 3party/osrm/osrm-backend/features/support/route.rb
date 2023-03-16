require 'net/http'

HOST = "http://127.0.0.1:#{OSRM_PORT}"
DESTINATION_REACHED = 15      #OSRM instruction code

class Hash
  def to_param(namespace = nil)
    collect do |key, value|
      "#{key}=#{value}"
    end.sort
  end
end

def request_path path, waypoints=[], options={}
  locs = waypoints.compact.map { |w| "loc=#{w.lat},#{w.lon}" }
  params = (locs + options.to_param).join('&')
  params = nil if params==""
  uri = URI.parse ["#{HOST}/#{path}", params].compact.join('?')
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

def request_url path
  uri = URI.parse"#{HOST}/#{path}"
  @query = uri.to_s
  Timeout.timeout(OSRM_TIMEOUT) do
    Net::HTTP.get_response uri
  end
rescue Errno::ECONNREFUSED => e
  raise "*** osrm-routed is not running."
rescue Timeout::Error
  raise "*** osrm-routed did not respond."
end

def request_route waypoints, params={}
  defaults = { 'output' => 'json', 'instructions' => true, 'alt' => false }
  request_path "viaroute", waypoints, defaults.merge(params)
end

def request_table waypoints, params={}
  defaults = { 'output' => 'json' }
  request_path "table", waypoints, defaults.merge(params)
end

def got_route? response
  if response.code == "200" && !response.body.empty?
    json = JSON.parse response.body
    if json['status'] == 0
      return way_list( json['route_instructions']).empty? == false
    end
  end
  false
end

def route_status response
  if response.code == "200" && !response.body.empty?
    json = JSON.parse response.body
    if json['status'] == 0
      if way_list( json['route_instructions']).empty?
        return 'Empty route'
      else
        return 'x'
      end
    elsif json['status'] == 207
      ''
    else
      "Status #{json['status']}"
    end
  else
    "HTTP #{response.code}"
  end
end

def extract_instruction_list instructions, index, postfix=nil
  if instructions
    instructions.reject { |r| r[0].to_s=="#{DESTINATION_REACHED}" }.
    map { |r| r[index] }.
    map { |r| (r=="" || r==nil) ? '""' : "#{r}#{postfix}" }.
    join(',')
  end
end

def way_list instructions
  extract_instruction_list instructions, 1
end

def compass_list instructions
  extract_instruction_list instructions, 6
end

def bearing_list instructions
  extract_instruction_list instructions, 7
end

def turn_list instructions
  if instructions
    types = {
      0 => :none,
      1 => :straight,
      2 => :slight_right,
      3 => :right,
      4 => :sharp_right,
      5 => :u_turn,
      6 => :sharp_left,
      7 => :left,
      8 => :slight_left,
      9 => :via,
      10 => :head,
      11 => :enter_roundabout,
      12 => :leave_roundabout,
      13 => :stay_roundabout,
      14 => :start_end_of_street,
      15 => :destination,
      16 => :enter_contraflow,
      17 => :leave_contraflow
    }
    # replace instructions codes with strings
    # "11-3" (enter roundabout and leave a 3rd exit) gets converted to "enter_roundabout-3"
    instructions.map do |r|
      r[0].to_s.gsub(/^\d*/) do |match|
        types[match.to_i].to_s
      end
    end.join(',')
  end
end

def mode_list instructions
  extract_instruction_list instructions, 8
end

def time_list instructions
  extract_instruction_list instructions, 4, "s"
end

def distance_list instructions
  extract_instruction_list instructions, 2, "m"
end
