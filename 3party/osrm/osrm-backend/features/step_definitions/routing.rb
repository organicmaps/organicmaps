When /^I route I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      if row['request']
        got = {'request' => row['request'] }
        response = request_url row['request']
      else
        params = @query_params
        waypoints = []
        if row['from'] and row['to']
          node = find_node_by_name(row['from'])
          raise "*** unknown from-node '#{row['from']}" unless node
          waypoints << node

          node = find_node_by_name(row['to'])
          raise "*** unknown to-node '#{row['to']}" unless node
          waypoints << node

          got = {'from' => row['from'], 'to' => row['to'] }
          response = request_route waypoints, params
        elsif row['waypoints']
          row['waypoints'].split(',').each do |n|
            node = find_node_by_name(n.strip)
            raise "*** unknown waypoint node '#{n.strip}" unless node
            waypoints << node
          end
          got = {'waypoints' => row['waypoints'] }
          response = request_route waypoints, params
        else
          raise "*** no waypoints"
        end
      end

      row.each_pair do |k,v|
        if k =~ /param:(.*)/
          if v=='(nil)'
            params[$1]=nil
          elsif v!=nil
            params[$1]=v
          end
          got[k]=v
        end
      end

      if response.body.empty? == false
        json = JSON.parse response.body
      end

      if response.body.empty? == false
        if json['status'] == 0
          instructions = way_list json['route_instructions']
          bearings = bearing_list json['route_instructions']
          compasses = compass_list json['route_instructions']
          turns = turn_list json['route_instructions']
          modes = mode_list json['route_instructions']
          times = time_list json['route_instructions']
          distances = distance_list json['route_instructions']
        end
      end

      if table.headers.include? 'status'
        got['status'] = json['status'].to_s
      end
      if table.headers.include? 'message'
        got['message'] = json['status_message']
      end
      if table.headers.include? '#'   # comment column
        got['#'] = row['#']           # copy value so it always match
      end

      if response.code == "200"
        if table.headers.include? 'start'
          got['start'] = instructions ? json['route_summary']['start_point'] : nil
        end
        if table.headers.include? 'end'
          got['end'] = instructions ? json['route_summary']['end_point'] : nil
        end
        if table.headers.include? 'geometry'
            got['geometry'] = json['route_geometry']
        end
        if table.headers.include? 'route'
          got['route'] = (instructions || '').strip
          if table.headers.include?('distance')
            if row['distance']!=''
              raise "*** Distance must be specied in meters. (ex: 250m)" unless row['distance'] =~ /\d+m/
            end
            got['distance'] = instructions ? "#{json['route_summary']['total_distance'].to_s}m" : ''
          end
          if table.headers.include?('time')
            raise "*** Time must be specied in seconds. (ex: 60s)" unless row['time'] =~ /\d+s/
            got['time'] = instructions ? "#{json['route_summary']['total_time'].to_s}s" : ''
          end
          if table.headers.include?('speed')
            if row['speed'] != '' && instructions
              raise "*** Speed must be specied in km/h. (ex: 50 km/h)" unless row['speed'] =~ /\d+ km\/h/
                time = json['route_summary']['total_time']
                distance = json['route_summary']['total_distance']
                speed = time>0 ? (3.6*distance/time).to_i : nil
                got['speed'] =  "#{speed} km/h"
            else
              got['speed'] = ''
            end
          end
          if table.headers.include? 'bearing'
            got['bearing'] = instructions ? bearings : ''
          end
          if table.headers.include? 'compass'
            got['compass'] = instructions ? compasses : ''
          end
          if table.headers.include? 'turns'
            got['turns'] = instructions ? turns : ''
          end
          if table.headers.include? 'modes'
            got['modes'] = instructions ? modes : ''
          end
          if table.headers.include? 'times'
            got['times'] = instructions ? times : ''
          end
          if table.headers.include? 'distances'
            got['distances'] = instructions ? distances : ''
          end
        end
      end

      ok = true
      row.keys.each do |key|
        if FuzzyMatch.match got[key], row[key]
          got[key] = row[key]
        else
          ok = false
        end
      end

      unless ok
        log_fail row,got, { 'route' => {:query => @query, :response => response} }
      end

      actual << got
    end
  end
  table.routing_diff! actual
end

When /^I route (\d+) times I should get$/ do |n,table|
  ok = true
  n.to_i.times do
    ok = false unless step "I route I should get", table
  end
  ok
end
