When(/^I run "osrm\-routed\s?(.*?)"$/) do |options|
  begin
    Timeout.timeout(SHUTDOWN_TIMEOUT) { run_bin 'osrm-routed', options }
  rescue Timeout::Error
    raise "*** osrm-routed didn't quit. Maybe the --trial option wasn't used?"
  end
end

When(/^I run "osrm\-extract\s?(.*?)"$/) do |options|
  run_bin 'osrm-extract', options
end

When(/^I run "osrm\-prepare\s?(.*?)"$/) do |options|
  run_bin 'osrm-prepare', options
end

When(/^I run "osrm\-datastore\s?(.*?)"$/) do |options|
  run_bin 'osrm-datastore', options
end

Then /^it should exit with code (\d+)$/ do |code|
  expect(@exit_code).to eq( code.to_i )
end

Then /^stdout should contain "(.*?)"$/ do |str|
  expect(@stdout).to include(str)
end

Then /^stderr should contain "(.*?)"$/ do |str|
  expect(@stderr).to include(str)
end

Then(/^stdout should contain \/(.*)\/$/) do |regex_str|
  regex = Regexp.new regex_str
  expect(@stdout).to match( regex )
end

Then(/^stderr should contain \/(.*)\/$/) do |regex_str|
  regex = Regexp.new regex_str
  expect(@stderr).to match( regex )
end

Then /^stdout should be empty$/ do
  expect(@stdout).to eq("")
end

Then /^stderr should be empty$/ do
  expect(@stderr).to eq("")
end

Then /^stdout should contain (\d+) lines?$/ do |lines|
  expect(@stdout.lines.count).to eq( lines.to_i )
end

Given (/^the query options$/) do |table|
  @query_params = table.rows_hash
end
