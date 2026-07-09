$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$dashboardRoot = Join-Path $repoRoot "web\northbound-safe-charge-dashboard"

if (-not (Test-Path -LiteralPath $dashboardRoot)) {
    throw "Dashboard directory not found: $dashboardRoot"
}

Set-Location -LiteralPath $dashboardRoot
npm install
npm run dev
