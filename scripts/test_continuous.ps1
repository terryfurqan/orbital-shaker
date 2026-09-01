$sp = New-Object System.IO.Ports.SerialPort "COM5", 115200, "None", 8, "One"
$sp.ReadTimeout = 1000
$sp.WriteTimeout = 1000
$sp.DtrEnable = $true

Write-Host "Opening COM5..."
$sp.Open()
Start-Sleep -Milliseconds 1800
while ($sp.BytesToRead -gt 0) { try { [void]$sp.ReadLine() } catch { break } }

Write-Host "`n>>> Sending START command via Serial (Bypassing A0 button)..."
$sp.WriteLine("START")
Start-Sleep -Milliseconds 100

for ($i = 0; $i -lt 10; $i++) {
    $sp.WriteLine("STATUS")
    Start-Sleep -Milliseconds 400
    while ($sp.BytesToRead -gt 0) {
        try {
            $l = $sp.ReadLine()
            Write-Host "ARDUINO: $l"
        } catch { break }
    }
}

Write-Host "`n>>> Sending STOP..."
$sp.WriteLine("STOP")
Start-Sleep -Milliseconds 500
while ($sp.BytesToRead -gt 0) {
    try {
        $l = $sp.ReadLine()
        Write-Host "ARDUINO: $l"
    } catch { break }
}

$sp.Close()
