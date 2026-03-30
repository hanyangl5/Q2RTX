param(
    [switch]$Quiet
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$androidDir = Join-Path $repoRoot "android"
$wrapperBat = Join-Path $androidDir "gradlew.bat"
$wrapperJar = Join-Path $androidDir "gradle/wrapper/gradle-wrapper.jar"
$wrapperProps = Join-Path $androidDir "gradle/wrapper/gradle-wrapper.properties"

if ((Test-Path $wrapperBat) -and (Test-Path $wrapperJar) -and (Test-Path $wrapperProps)) {
    if (-not $Quiet) {
        Write-Host "Gradle wrapper is ready in $androidDir"
    }
    exit 0
}

throw "Bundled Android Gradle wrapper files are missing from $androidDir. Restore gradlew, gradlew.bat, and gradle/wrapper/* from version control."
