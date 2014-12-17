When /^I request nearest I should get$/ do |table|
  reprocess
  actual = []
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,ri|
      in_node = find_node_by_name row['in']
      raise "*** unknown in-node '#{row['in']}" unless in_node

      out_node = find_node_by_name row['out']
      raise "*** unknown out-node '#{row['out']}" unless out_node

      response = request_nearest("#{in_node.lat},#{in_node.lon}")
      if response.code == "200" && response.body.empty? == false
        json = JSON.parse response.body
        if json['status'] == 0
          coord =  json['mapped_coordinate']
        end
      end

      got = {'in' => row['in'], 'out' => coord }

      ok = true
      row.keys.each do |key|
        if key=='out'
          if FuzzyMatch.match_location coord, out_node
            got[key] = row[key]
          else
            row[key] = "#{row[key]} [#{out_node.lat},#{out_node.lon}]"
            ok = false
          end
        end
      end

      unless ok
        failed = { :attempt => 'nearest', :query => @query, :response => response }
        log_fail row,got,[failed]
      end

      actual << got
    end
  end
  table.routing_diff! actual
end

When /^I request nearest (\d+) times I should get$/ do |n,table|
  ok = true
  n.to_i.times do
    ok = false unless step "I request nearest I should get", table
  end
  ok
end
