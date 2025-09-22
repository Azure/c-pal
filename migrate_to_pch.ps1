#!/usr/bin/env pwsh
<#
.SYNOPSIS
Migrates unit test projects from MOCK_PRECOMPILE_HEADERS to ENABLE_TEST_FILES_PRECOMPILED_HEADERS

.DESCRIPTION
This script processes unit test projects, extracts includes from _ut.c files into separate PCH headers,
moves MOCKABLE_FUNCTION sections to proper locations, and updates CMakeLists.txt files accordingly.

.PARAMETER TestMode
Process only a single test project for validation (default: $false)

.PARAMETER ProjectPath
Specific project path to process when TestMode is enabled
#>

param(
    [switch]$TestMode = $false,
    [string]$ProjectPath = ""
)

# Set error action preference
$ErrorActionPreference = "Stop"

function Write-Log {
    param([string]$Message, [string]$Level = "INFO")
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    Write-Host "[$timestamp] [$Level] $Message"
}

function Find-UnitTestProjects {
    param([string]$RootPath)
    
    Write-Log "Finding unit test projects..."
    $projects = @()
    
    # Find all _ut.c files excluding deps folder
    $utFiles = Get-ChildItem -Path $RootPath -Recurse -Filter "*_ut.c" | Where-Object {
        $_.DirectoryName -notlike "*\deps\*"
    }
    
    foreach ($utFile in $utFiles) {
        $projectDir = $utFile.Directory
        $cmakeFile = Join-Path $projectDir "CMakeLists.txt"
        
        if (Test-Path $cmakeFile) {
            $projects += @{
                ProjectDir = $projectDir.FullName
                UtFile = $utFile.FullName
                CmakeFile = $cmakeFile
                TestName = [System.IO.Path]::GetFileNameWithoutExtension($utFile.Name)
            }
        }
    }
    
    Write-Log "Found $($projects.Count) unit test projects"
    return $projects
}

function Parse-IncludeSection {
    param([string]$FilePath)
    
    Write-Log "Parsing include section from: $FilePath"
    
    $lines = Get-Content -Path $FilePath
    
    $includeLines = @()
    $mockableFunctions = @()
    $afterIncludesLines = @()
    
    $inIncludeSection = $true
    $inMockableFunction = $false
    $inEnableMocksSection = $false
    $inMockFunctionWithCode = $false
    $mockableFunctionBuffer = @()
    $enableMocksDepth = 0
    
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $trimmedLine = $line.Trim()
        
        if ($inIncludeSection) {
            # Check if we're entering an ENABLE_MOCKS section
            if ($trimmedLine -eq '#define ENABLE_MOCKS') {
                $inEnableMocksSection = $true
                $enableMocksDepth++
                $includeLines += $line
                continue
            }
            
            # Check if we're exiting an ENABLE_MOCKS section
            if ($trimmedLine -eq '#undef ENABLE_MOCKS' -and $inEnableMocksSection) {
                $enableMocksDepth--
                if ($enableMocksDepth -eq 0) {
                    $inEnableMocksSection = $false
                }
                $includeLines += $line
                continue
            }
            
            # Handle MOCKABLE_FUNCTION blocks - ALWAYS extract them as mockable functions
            if ($trimmedLine -match '^MOCKABLE_FUNCTION') {
                $inMockableFunction = $true
                $mockableFunctionBuffer = @($line)
                # Check if this is a single-line MOCKABLE_FUNCTION
                if ($trimmedLine -match '\);?\s*$') {
                    $mockableFunctions += $mockableFunctionBuffer -join "`r`n"
                    $mockableFunctionBuffer = @()
                    $inMockableFunction = $false
                }
                continue
            }
            
            if ($inMockableFunction) {
                $mockableFunctionBuffer += $line
                
                # For regular functions (not MOCKABLE_FUNCTION), we need to track brace balance
                if ($mockableFunctionBuffer[0].Trim() -notmatch '^MOCKABLE_FUNCTION') {
                    # Look for opening brace if we haven't seen one yet
                    if ($trimmedLine -match '\{') {
                        # Start tracking braces
                        $openBraces = ($trimmedLine.ToCharArray() | Where-Object { $_ -eq '{' } | Measure-Object).Count
                        $closeBraces = ($trimmedLine.ToCharArray() | Where-Object { $_ -eq '}' } | Measure-Object).Count
                        
                        # If braces are balanced on this line, function is complete
                        if ($openBraces -eq $closeBraces) {
                            $mockableFunctions += $mockableFunctionBuffer -join "`r`n"
                            $mockableFunctionBuffer = @()
                            $inMockableFunction = $false
                        }
                    } elseif ($trimmedLine -match '\}') {
                        # We're at the end of a function (closing brace)
                        $mockableFunctions += $mockableFunctionBuffer -join "`r`n"
                        $mockableFunctionBuffer = @()
                        $inMockableFunction = $false
                    }
                } else {
                    # For MOCKABLE_FUNCTION, use the original logic
                    if ($trimmedLine -match '\);?\s*$') {
                        $mockableFunctions += $mockableFunctionBuffer -join "`r`n"
                        $mockableFunctionBuffer = @()
                        $inMockableFunction = $false
                    }
                }
                continue
            }
            
            # Handle MOCK_FUNCTION_WITH_CODE blocks - ALWAYS extract them as mockable functions
            if ($trimmedLine -match '^MOCK_FUNCTION_WITH_CODE\(') {
                $inMockFunctionWithCode = $true
                $mockableFunctionBuffer = @($line)
                continue
            }
            
            if ($inMockFunctionWithCode) {
                $mockableFunctionBuffer += $line
                if ($trimmedLine -match '^MOCK_FUNCTION_END\(') {
                    $mockableFunctions += $mockableFunctionBuffer -join "`r`n"
                    $mockableFunctionBuffer = @()
                    $inMockFunctionWithCode = $false
                }
                continue
            }
            
            # Check if we've reached actual code (not comments, includes, defines, etc.)
            # Also detect function definitions by looking for function signature patterns
            if (($trimmedLine -match '^\w+\*?\s+\w+\s*\([^)]*\)\s*$' -or $trimmedLine -match '\{.*') -and -not $inEnableMocksSection) {
                # This looks like a function definition - extract it as a mockable function
                $inMockableFunction = $true
                $mockableFunctionBuffer = @($line)
                
                # If this line contains an opening brace, count braces for balance tracking
                if ($trimmedLine -match '\{.*') {
                    $openBraces = ($trimmedLine.ToCharArray() | Where-Object { $_ -eq '{' } | Measure-Object).Count
                    $closeBraces = ($trimmedLine.ToCharArray() | Where-Object { $_ -eq '}' } | Measure-Object).Count
                    
                    # If braces are balanced on the same line, it's a complete function
                    if ($openBraces -eq $closeBraces) {
                        $mockableFunctions += $mockableFunctionBuffer -join "`r`n"
                        $mockableFunctionBuffer = @()
                        $inMockableFunction = $false
                    }
                }
                continue
            }
            
            # Check if this line indicates the end of the include section
            $isEndOfIncludes = $false
            
            # Pattern 1: Common definitions and declarations
            if ($trimmedLine -match '^(typedef|struct|enum|static|extern|MU_DEFINE_|TEST_DEFINE_|IMPLEMENT_|DECLARE_(?!ASYNC_TYPE_HELPER)|DEFINE_(?!ASYNC_TYPE_HELPER)|BEGIN_TEST_SUITE|THANDLE_TYPE_)') {
                $isEndOfIncludes = $true
            }
            
            # Pattern 2: Function definitions (return_type function_name(...))
            if ($trimmedLine -match '^\w+(?:\s+\w+)*\s+\w+\s*\([^)]*\)\s*$' -and $trimmedLine -notmatch '^#') {
                $isEndOfIncludes = $true
            }
            
            # Pattern 3: Function definitions starting with opening brace on same line
            if ($trimmedLine -match '^\w+(?:\s+\w+)*\s+\w+\s*\([^)]*\)\s*\{') {
                $isEndOfIncludes = $true
            }
            
            if ($isEndOfIncludes -and 
                $trimmedLine -ne '' -and 
                -not $trimmedLine.StartsWith('//') -and
                -not $trimmedLine.StartsWith('/*') -and
                -not $inEnableMocksSection) {
                
                $inIncludeSection = $false
                $afterIncludesLines += $line
                continue
            }
            
            # Only add to includeLines if we're not currently processing a mockable function
            if (-not $inMockableFunction -and -not $inMockFunctionWithCode) {
                # Don't include .c files in PCH headers - they contain implementations
                # Keep them as includes in the UT file instead, wrapped in ENABLE_MOCKS
                if ($trimmedLine -match '#include\s+"[^"]*\.c"' -or $trimmedLine -match "#include\s+'[^']*\.c'") {
                    # This is a .c file include - wrap it in ENABLE_MOCKS and add to afterIncludes
                    $afterIncludesLines += "#define ENABLE_MOCKS"
                    $afterIncludesLines += "#undef ENABLE_MOCKS_DECL"
                    $afterIncludesLines += "#include `"umock_c/umock_c_prod.h`""
                    $afterIncludesLines += $line
                    $afterIncludesLines += "#undef ENABLE_MOCKS"
                    $afterIncludesLines += ""
                } else {
                    $includeLines += $line
                }
            }
        } else {
            $afterIncludesLines += $line
        }
    }
    
    # Remove trailing empty lines from include section
    while ($includeLines.Count -gt 0 -and $includeLines[-1].Trim() -eq '') {
        $includeLines = $includeLines[0..($includeLines.Count - 2)]
    }
    
    return @{
        IncludeLines = $includeLines
        MockableFunctions = $mockableFunctions
        AfterIncludesLines = $afterIncludesLines
    }
}

function Create-PchHeader {
    param(
        [string]$ProjectDir,
        [string]$TestName,
        [string[]]$IncludeLines,
        [string]$UtFilePath
    )
    
    $pchFileName = "${TestName}_pch.h"
    $pchFilePath = Join-Path $ProjectDir $pchFileName
    
    Write-Log "Creating PCH header: $pchFilePath"
    
    # Extract original copyright notice from the UT file
    $originalLines = Get-Content -Path $UtFilePath
    $copyrightLines = @()
    
    foreach ($line in $originalLines) {
        $trimmedLine = $line.Trim()
        if ($trimmedLine.StartsWith('// Copyright') -or 
            $trimmedLine.StartsWith('// Licensed under') -or 
            ($copyrightLines.Count -gt 0 -and $trimmedLine -eq '')) {
            $copyrightLines += $line
            if ($trimmedLine.StartsWith('// Licensed under')) {
                break
            }
        } elseif ($copyrightLines.Count -gt 0) {
            break
        }
    }
    
    # Clean the include lines to remove all copyright/license lines
    $cleanIncludeLines = @()
    $skipCopyrightSection = $true
    
    foreach ($line in $IncludeLines) {
        $trimmedLine = $line.Trim()
        
        # Skip copyright and license lines at the beginning
        if ($skipCopyrightSection) {
            if ($trimmedLine.StartsWith('// Copyright') -or 
                $trimmedLine.StartsWith('// Licensed under') -or 
                $trimmedLine -eq '') {
                continue
            } else {
                $skipCopyrightSection = $false
            }
        }
        
        $cleanIncludeLines += $line
    }
    
    # Build PCH content with original copyright and cleaned includes
    $pchContent = ($copyrightLines -join "`r`n") + "`r`n`r`n// Precompiled header for $TestName`r`n`r`n" + ($cleanIncludeLines -join "`r`n")
    
    # Write with Windows line endings (CRLF)
    [System.IO.File]::WriteAllText($pchFilePath, $pchContent, [System.Text.Encoding]::UTF8)
    
    return $pchFileName
}

function Update-UtFile {
    param(
        [string]$UtFilePath,
        [string]$PchFileName,
        [string[]]$MockableFunctions,
        [string[]]$AfterIncludesLines
    )
    
    Write-Log "Updating UT file: $UtFilePath"
    
    # Extract original copyright notice from the UT file
    $originalLines = Get-Content -Path $UtFilePath
    $copyrightLines = @()
    
    foreach ($line in $originalLines) {
        $trimmedLine = $line.Trim()
        if ($trimmedLine.StartsWith('// Copyright') -or 
            $trimmedLine.StartsWith('// Licensed under') -or 
            ($copyrightLines.Count -gt 0 -and $trimmedLine -eq '')) {
            $copyrightLines += $line
            if ($trimmedLine.StartsWith('// Licensed under')) {
                break
            }
        } elseif ($copyrightLines.Count -gt 0) {
            break
        }
    }
    
    # Build new content starting with original copyright with Windows line endings
    $newContent = ($copyrightLines -join "`r`n") + "`r`n`r`n#include `"$PchFileName`"`r`n`r`n"
    
    if ($MockableFunctions.Count -gt 0) {
        $newContent += "#define ENABLE_MOCKS`r`n#undef ENABLE_MOCKS_DECL`r`n#include `"umock_c/umock_c_prod.h`"`r`n" + ($MockableFunctions -join "`r`n`r`n") + "`r`n#undef ENABLE_MOCKS`r`n`r`n"
    }
    
    $newContent += ($AfterIncludesLines -join "`r`n")
    
    # Write with Windows line endings (CRLF)
    [System.IO.File]::WriteAllText($UtFilePath, $newContent, [System.Text.Encoding]::UTF8)
}

function Update-CmakeFile {
    param(
        [string]$CmakeFilePath,
        [string]$TestName,
        [string]$PchFileName
    )
    
    Write-Log "Updating CMake file: $CmakeFilePath"
    
    $lines = Get-Content -Path $CmakeFilePath
    $newLines = @()
    $inBuildTestArtifacts = $false
    $foundMockHeaders = $false
    
    for ($i = 0; $i -lt $lines.Count; $i++) {
        $line = $lines[$i]
        $trimmedLine = $line.Trim()
        
        # Detect start of build_test_artifacts call
        if ($trimmedLine -match '^build_test_artifacts\(') {
            $inBuildTestArtifacts = $true
            $newLines += $line
            
            # Check if this is a single-line call that ends with )
            if ($trimmedLine -match '\)$') {
                # Add ENABLE_TEST_FILES_PRECOMPILED_HEADERS before the closing parenthesis
                $modifiedLine = $line -replace '\)$', " `r`n    ENABLE_TEST_FILES_PRECOMPILED_HEADERS `"`${CMAKE_CURRENT_LIST_DIR}/$PchFileName`"`r`n)"
                $newLines[-1] = $modifiedLine
                $inBuildTestArtifacts = $false
            }
            continue
        }
        
        # Detect end of build_test_artifacts call
        if ($inBuildTestArtifacts -and $trimmedLine -eq ')') {
            # Add ENABLE_TEST_FILES_PRECOMPILED_HEADERS before closing
            $indent = if ($newLines[-1] -match '^(\s+)') { $matches[1] } else { "    " }
            $newLines += "$indent" + "ENABLE_TEST_FILES_PRECOMPILED_HEADERS `"`${CMAKE_CURRENT_LIST_DIR}/$PchFileName`""
            $newLines += $line
            $inBuildTestArtifacts = $false
            continue
        }
        
        # Skip lines related to MOCK_PRECOMPILE_HEADERS and NO_MOCK_PRECOMPILE_HEADERS
        if ($inBuildTestArtifacts) {
            if ($trimmedLine -eq 'NO_MOCK_PRECOMPILE_HEADERS' -or $trimmedLine -eq 'MOCK_PRECOMPILE_HEADERS') {
                $foundMockHeaders = $true
                continue
            }
            
            # Skip header file references when we found mock headers (both quoted and unquoted)
            if ($foundMockHeaders -and ($trimmedLine -match '^".*\.h"$' -or $trimmedLine -match '^[\w_/]+\.h$')) {
                continue
            }
            
            # Reset foundMockHeaders when we encounter other parameters
            if ($foundMockHeaders -and -not ($trimmedLine -match '^".*\.h"$' -or $trimmedLine -match '^[\w_/]+\.h$') -and $trimmedLine -ne '') {
                $foundMockHeaders = $false
            }
        }
        
        $newLines += $line
    }
    
    $content = $newLines -join "`r`n"
    # Write with Windows line endings (CRLF)
    [System.IO.File]::WriteAllText($CmakeFilePath, $content, [System.Text.Encoding]::UTF8)
}

function Process-Project {
    param([hashtable]$Project)
    
    Write-Log "Processing project: $($Project.TestName)"
    
    try {
        # Check if file has already been processed (contains PCH include)
        $fileContent = Get-Content -Path $Project.UtFile -Raw
        $expectedPchInclude = "#include ""$($Project.TestName)_pch.h"""
        
        if ($fileContent -match [regex]::Escape($expectedPchInclude)) {
            Write-Log "File already processed (contains PCH include): $($Project.TestName)" -Level "INFO"
            return $true
        }
        
        # Parse the UT file
        $parsed = Parse-IncludeSection -FilePath $Project.UtFile
        
        # Create PCH header
        $pchFileName = Create-PchHeader -ProjectDir $Project.ProjectDir -TestName $Project.TestName -IncludeLines $parsed.IncludeLines -UtFilePath $Project.UtFile
        
        # Update UT file
        Update-UtFile -UtFilePath $Project.UtFile -PchFileName $pchFileName -MockableFunctions $parsed.MockableFunctions -AfterIncludesLines $parsed.AfterIncludesLines
        
        # Update CMake file
        Update-CmakeFile -CmakeFilePath $Project.CmakeFile -TestName $Project.TestName -PchFileName $pchFileName
        
        Write-Log "Successfully processed: $($Project.TestName)" -Level "SUCCESS"
        return $true
    }
    catch {
        Write-Log "Failed to process $($Project.TestName): $($_.Exception.Message)" -Level "ERROR"
        return $false
    }
}

# Main execution
try {
    Write-Log "Starting PCH migration script..."
    
    if ($TestMode) {
        if ($ProjectPath -eq "") {
            Write-Log "TestMode enabled but no ProjectPath specified. Using first project found."
            $projects = Find-UnitTestProjects -RootPath "."
            if ($projects.Count -eq 0) {
                throw "No unit test projects found"
            }
            $projectToProcess = $projects[0]
        } else {
            $utFile = Get-ChildItem -Path $ProjectPath -Filter "*_ut.c" | Select-Object -First 1
            if (-not $utFile) {
                throw "No _ut.c file found in specified project path: $ProjectPath"
            }
            
            $projectToProcess = @{
                ProjectDir = $ProjectPath
                UtFile = $utFile.FullName
                CmakeFile = Join-Path $ProjectPath "CMakeLists.txt"
                TestName = [System.IO.Path]::GetFileNameWithoutExtension($utFile.Name)
            }
        }
        
        Write-Log "Test mode: Processing single project: $($projectToProcess.TestName)"
        $success = Process-Project -Project $projectToProcess
        
        if ($success) {
            Write-Log "Test completed successfully!" -Level "SUCCESS"
        } else {
            Write-Log "Test failed!" -Level "ERROR"
            exit 1
        }
    } else {
        $projects = Find-UnitTestProjects -RootPath "."
        $successCount = 0
        $failureCount = 0
        
        foreach ($project in $projects) {
            if (Process-Project -Project $project) {
                $successCount++
            } else {
                $failureCount++
            }
        }
        
        Write-Log "Migration completed: $successCount succeeded, $failureCount failed" -Level "SUCCESS"
    }
}
catch {
    Write-Log "Script failed: $($_.Exception.Message)" -Level "ERROR"
    exit 1
}