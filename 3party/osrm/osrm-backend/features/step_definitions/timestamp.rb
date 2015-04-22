Then /^I should get a valid timestamp/ do
  step "I should get a response"
  step "response should be valid JSON"
  step "response should be well-formed"
  expect(@json['timestamp'].class).to eq(String)
  expect(@json['timestamp']).to eq("2000-01-01T00:00:00Z")
end
