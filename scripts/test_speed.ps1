$sp = New-Object System.IO.Ports.SerialPort "COM5", 115200, "None", 8, "One"
$sp.ReadTimeout = 1000
$sp.WriteTimeout = 1000
$sp.DtrEnable = $true

Write-Host "Opening COM5..."
$sp.Open()
Start-Sleep -Milliseconds 1800
while ($sp.BytesToRead -gt 0) { try { [void]$sp.ReadLine() } catch { break } }

function Send-Cmd($cmd) {
    Write-Host "`n>>> SEND: $cmd"
    $sp.WriteLine($cmd)
    Start-Sleep -Milliseconds 300
    while ($sp.BytesToRead -gt 0) {
        try {
            $l = $sp.ReadLine()
            Write-Host "ARDUINO: $l"
        } catch { break }
    }
}

Send-Cmd "STATUS"
Send-Cmd "RPM=60"
Send-Cmd "STATUS"
Send-Cmd "RPM=30"
Send-Cmd "STATUS"

$sp.Close()
Write-Host "`nSpeed test completed successfully!"
