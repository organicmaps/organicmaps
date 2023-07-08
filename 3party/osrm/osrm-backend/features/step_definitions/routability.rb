def test_routability_row i
  result = {}
  ['forw','backw'].each do |direction|
    a = Location.new @origin[0]+(1+WAY_SPACING*i)*@zoom, @origin[1]
    b = Location.new @origin[0]+(3+WAY_SPACING*i)*@zoom, @origin[1]
    r = {}
    r[:response] = request_route (direction=='forw' ? [a,b] : [b,a]), @query_params
    r[:query] = @query
    r[:json] = JSON.parse(r[:response].body)

    r[:status] = route_status r[:response]
    if r[:status].empty? == false
      r[:route] = way_list r[:json]['route_instructions']

      if r[:route]=="w#{i}"
        r[:time] = r[:json]['route_summary']['total_time']
        r[:distance] = r[:json]['route_summary']['total_distance']
        r[:speed] = r[:time]>0 ? (3.6*r[:distance]/r[:time]).to_i : nil
      else
        # if we hit the wrong way segment, we assume it's
        # because the one we tested was not unroutable
        r[:status] = nil
      end
    end
    result[direction] = r
  end

  # check if forw and backw returned the same values
  result['bothw'] = {}
  [:status,:time,:distance,:speed].each do |key|
    if result['forw'][key] == result['backw'][key]
      result['bothw'][key] = result['forw'][key]
    else
      result['bothw'][key] = 'diff'
    end
  end
  result
end

Then /^routability should be$/ do |table|
  build_ways_from_table table
  reprocess
  actual = []
  if table.headers&["forw","backw","bothw"] == []
    raise "*** routability tabel must contain either 'forw', 'backw' or 'bothw' column"
  end
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    table.hashes.each_with_index do |row,i|
      output_row = row.dup
      attempts = []
      result = test_routability_row i
      directions = ['forw','backw','bothw']
      (directions & table.headers).each do |direction|
        want = shortcuts_hash[row[direction]] || row[direction]     #expand shortcuts
        case want
        when '', 'x'
          output_row[direction] = result[direction][:status] ? result[direction][:status].to_s : ''
        when /^\d+s/
          output_row[direction] = result[direction][:time] ? "#{result[direction][:time]}s" : ''
        when /^\d+ km\/h/
          output_row[direction] = result[direction][:speed] ? "#{result[direction][:speed]} km/h" : ''
        else
          raise "*** Unknown expectation format: #{want}"
        end

        if FuzzyMatch.match output_row[direction], want
          output_row[direction] = row[direction]
        end
      end

      if output_row != row
        log_fail row,output_row,result
      end
      actual << output_row
    end
  end
  table.routing_diff! actual
end
