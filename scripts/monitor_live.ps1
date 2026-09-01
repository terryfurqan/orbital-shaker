$sp = New-Object System.IO.Ports.SerialPort "COM5", 115200, "None", 8, "One"
$sp.ReadTimeout = 500
$sp.WriteTimeout = 500
$sp.DtrEnable = $true

Write-Host "Opening COM5 to monitor physical button presses for 10 seconds..."
$sp.Open()
Start-Sleep -Milliseconds 1500

$startTime = [DateTime]::Now
while (([DateTime]::Now - $startTime).TotalSeconds -lt 10) {
    try {
        if ($sp.BytesToRead -gt 0) {
            $line = $sp.ReadLine()
            Write-Host "[$(Get-Date -Format 'HH:mm:ss.fff')] $line"
        }
    } catch {}
    Start-Sleep -Milliseconds 20
}

$sp.Close()
Write-Host "Monitor finished."
