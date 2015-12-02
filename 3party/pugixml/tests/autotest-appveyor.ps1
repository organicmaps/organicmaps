function Invoke-CmdScript($scriptName)
{
	$cmdLine = """$scriptName"" $args & set"
	& $Env:SystemRoot\system32\cmd.exe /c $cmdLine |
	select-string '^([^=]*)=(.*)$' | foreach-object {
		$varName = $_.Matches[0].Groups[1].Value
		$varValue = $_.Matches[0].Groups[2].Value
		set-item Env:$varName $varValue
	}
}

$sources = @("src/pugixml.cpp") + (Get-ChildItem -Path "tests/*.cpp" -Exclude "fuzz_*.cpp")
$failed = $FALSE

foreach ($vs in 9,10,11,12,14)
{
	foreach ($arch in "x86","x64")
	{
		Write-Output "# Setting up VS$vs $arch"

		Invoke-CmdScript "C:\Program Files (x86)\Microsoft Visual Studio $vs.0\VC\vcvarsall.bat" $arch
		if (! $?) { throw "Error setting up VS$vs $arch" }

		foreach ($defines in "standard", "PUGIXML_WCHAR_MODE", "PUGIXML_COMPACT")
		{
			$target = "tests_vs${vs}_${arch}_${defines}"
			$deflist = if ($defines -eq "standard") { "" } else { "/D$defines" }

			Add-AppveyorTest $target -Outcome Running

			Write-Output "# Building $target.exe"
			& cmd /c "cl.exe /Fe$target.exe /EHsc /W4 /WX $deflist $sources 2>&1" | Tee-Object -Variable buildOutput

			if ($?)
			{
				Write-Output "# Running $target.exe"

				$sw = [Diagnostics.Stopwatch]::StartNew()

				& .\$target | Tee-Object -Variable testOutput

				if ($?)
				{
					Write-Output "# Passed"

					Update-AppveyorTest $target -Outcome Passed -StdOut ($testOutput | out-string) -Duration $sw.ElapsedMilliseconds
				}
				else
				{
					Write-Output "# Failed"

					Update-AppveyorTest $target -Outcome Failed -StdOut ($testOutput | out-string) -ErrorMessage "Running failed"

					$failed = $TRUE
				}
			}
			else
			{
				Write-Output "# Failed to build"

				Update-AppveyorTest $target -Outcome Failed -StdOut ($buildOutput | out-string) -ErrorMessage "Compilation failed"

				$failed = $TRUE
			}
		}
	}
}

if ($failed) { throw "One or more build steps failed" }

Write-Output "# End"
