$sp = New-Object System.IO.Ports.SerialPort "COM5", 115200, "None", 8, "One"
$sp.ReadTimeout = 1000
$sp.WriteTimeout = 1000
$sp.DtrEnable = $true

Write-Host "Opening COM5 at 115200..."
$sp.Open()
Start-Sleep -Milliseconds 1800

# Read initial splash
while ($sp.BytesToRead -gt 0) {
    try {
        $line = $sp.ReadLine()
        Write-Host "ARDUINO: $line"
    } catch {
        break
    }
}

# Send STATUS command
Write-Host "`n>>> Sending STATUS command..."
$sp.WriteLine("STATUS")
Start-Sleep -Milliseconds 400

while ($sp.BytesToRead -gt 0) {
    try {
        $line = $sp.ReadLine()
        Write-Host "ARDUINO: $line"
    } catch {
        break
    }
}

# Send START command
Write-Host "`n>>> Sending START command (Motor will rotate continuously at 30 RPM)..."
$sp.WriteLine("START")
Start-Sleep -Milliseconds 1500

while ($sp.BytesToRead -gt 0) {
    try {
        $line = $sp.ReadLine()
        Write-Host "ARDUINO: $line"
    } catch {
        break
    }
}

# Send STOP command
Write-Host "`n>>> Sending STOP command..."
$sp.WriteLine("STOP")
Start-Sleep -Milliseconds 500

while ($sp.BytesToRead -gt 0) {
    try {
        $line = $sp.ReadLine()
        Write-Host "ARDUINO: $line"
    } catch {
        break
    }
}

$sp.Close()
Write-Host "`nTest complete, serial port closed safely."
