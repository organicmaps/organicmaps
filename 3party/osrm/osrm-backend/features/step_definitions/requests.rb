When /^I request \/(.*)$/ do |path|
  reprocess
  OSRMLoader.load(self,"#{prepared_file}.osrm") do
    @response = request_path path
  end
end

Then /^I should get a response/ do
  expect(@response.code).to eq("200")
  expect(@response.body).not_to eq(nil)
  expect(@response.body).not_to eq('')
end

Then /^response should be valid JSON$/ do
  @json = JSON.parse @response.body
end

Then /^response should be well-formed$/ do
  expect(@json['status'].class).to eq(Fixnum)
end

Then /^status code should be (\d+)$/ do |code|
  @json = JSON.parse @response.body
  expect(@json['status']).to eq(code.to_i)
end

Then /^status message should be "(.*?)"$/ do |message|
  @json = JSON.parse @response.body
  expect(@json['status_message']).to eq(message)
end

Then /^response should be a well-formed route$/ do
  step "response should be well-formed"
  expect(@json['status_message'].class).to eq(String)
  expect(@json['route_summary'].class).to eq(Hash)
  expect(@json['route_geometry'].class).to eq(String)
  expect(@json['route_instructions'].class).to eq(Array)
  expect(@json['via_points'].class).to eq(Array)
  expect(@json['via_indices'].class).to eq(Array)
end

Then /^"([^"]*)" should return code (\d+)$/ do |binary, code|
  expect(@process_error.is_a?(OSRMError)).to eq(true)
  expect(@process_error.process).to eq(binary)
  expect(@process_error.code.to_i).to eq(code.to_i)
end
