When /^I request a travel time matrix I should get$/ do |table|
  
  no_route = 2147483647   # MAX_INT
  
  raise "*** Top-left cell of matrix table must be empty" unless table.headers[0]==""
  
  nodes = []
  column_headers = table.headers[1..-1]
  row_headers = table.rows.map { |h| h.first }
  unless column_headers==row_headers
    raise "*** Column and row headers must match in matrix table, got #{column_headers.inspect} and #{row_headers.inspect}"
  end
  column_headers.each do |node_name|
    node = find_node_by_name(node_name)
    raise "*** unknown node '#{node_name}" unless node
    nodes << node
  end
  
  reprocess
  actual = []
  actual << table.headers
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    
    # compute matrix
    params = @query_params
    response = request_table nodes, params
    if response.body.empty? == false
      json = JSON.parse response.body
      result = json['distance_table']
    end
    
    # compare actual and expected result, one row at a time
    table.rows.each_with_index do |row,ri|
      
      # fuzzy match
      ok = true
      0.upto(nodes.size-1) do |i|
        if FuzzyMatch.match result[ri][i], row[i+1]
          result[ri][i] = row[i+1]
        elsif row[i+1]=="" and result[ri][i]==no_route
          result[ri][i] = ""
        else
          result[ri][i] = result[ri][i].to_s
          ok = false
        end
      end
      
      # add row header
      r = [row[0],result[ri]].flatten
      
      # store row for comparison
      actual << r
    end
  end
  table.routing_diff! actual
end
