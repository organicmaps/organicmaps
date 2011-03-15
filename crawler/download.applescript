on run argv
	-- Load page and wait until it is loaded
	tell application "Google Chrome"
		activate
		set myTab to make new tab at end of tabs of window 1
		tell myTab
			set URL to item 1 of argv -- "http://www.wikipedia.org"
			repeat -- wait completion of loading
				set curStat to loading
				if curStat = false then exit repeat
				delay 0.25
			end repeat
		end tell
	end tell
	
	delay 1
	
	-- Click the save button
	repeat 10 times
		try
			tell application "System Events"
				tell process "Google Chrome"
					set saveButton to button 5 of tool bar 1 of window 1
					click saveButton
					exit repeat
				end tell
			end tell
		on error
			delay 1
		end try
	end repeat
	
	-- Wait for the file created
	-- repeat while not (exists file (item 2 of argv) of application "Finder")
	-- 	delay 1
	-- end repeat
	
	-- Wait for file stopped growing
	-- set resFile to (POSIX file (item 2 of argv))
	-- set size0 to 0
	-- set size1 to size of (info for resFile)
	-- repeat while size0  size1
	-- 	delay 0.25
	-- 	set size0 to size1
	-- 	set size1 to size of (info for resFile)
	-- end repeat
	
	delay 5
	
	tell myTab
		delete
	end tell
end run
