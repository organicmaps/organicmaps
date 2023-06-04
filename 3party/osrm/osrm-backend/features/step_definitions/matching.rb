When /^I match I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      if row['request']
        got = {'request' => row['request'] }
        response = request_url row['request']
      else
        params = @query_params
        trace = []
        timestamps = []
        if row['trace']
          row['trace'].each_char do |n|
            node = find_node_by_name(n.strip)
            raise "*** unknown waypoint node '#{n.strip}" unless node
            trace << node
          end
          if row['timestamps']
              timestamps = row['timestamps'].split(" ").compact.map { |t| t.to_i}
          end
          got = {'trace' => row['trace'] }
          response = request_matching trace, timestamps, params
        else
          raise "*** no trace"
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

      if table.headers.include? 'status'
        got['status'] = json['status'].to_s
      end
      if table.headers.include? 'message'
        got['message'] = json['status_message']
      end
      if table.headers.include? '#'   # comment column
        got['#'] = row['#']           # copy value so it always match
      end

      sub_matchings = []
      if response.code == "200"
        if table.headers.include? 'matchings'
          sub_matchings = json['matchings'].compact.map { |sub| sub['matched_points']}
        end
      end

      ok = true
      encoded_result = ""
      extended_target = ""
      row['matchings'].split(',').each_with_index do |sub, sub_idx|
        if sub_idx >= sub_matchings.length
          ok = false
          break
        end
        sub.length.times do |node_idx|
          node = find_node_by_name(sub[node_idx])
          out_node = sub_matchings[sub_idx][node_idx]
          if FuzzyMatch.match_location out_node, node
            encoded_result += sub[node_idx]
            extended_target += sub[node_idx]
          else
            encoded_result += "? [#{out_node[0]},#{out_node[1]}]"
            extended_target += "#{sub[node_idx]} [#{node.lat},#{node.lon}]"
            ok = false
          end
        end
      end
      if ok
        got['matchings'] = row['matchings']
        got['timestamps'] = row['timestamps']
      else
        got['matchings'] = encoded_result
        row['matchings'] = extended_target
        log_fail row,got, { 'matching' => {:query => @query, :response => response} }
      end

      actual << got
    end
  end
  table.routing_diff! actual
end

