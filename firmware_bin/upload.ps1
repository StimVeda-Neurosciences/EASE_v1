# $text = "Colorized PowerShell Output!"
# Write-Host "Red text: $text" -ForegroundColor Red
# Write-Host "Green text: $text" -ForegroundColor Green
# Write-Host "Blue text: $text" -ForegroundColor Blue
# Write-Host "Yellow text: $text" -ForegroundColor Yellow
# Write-Host "White text on yellow background: $text" -ForegroundColor White -BackgroundColor Yellow
# Write-Host "Bold blue text: $text" -ForegroundColor Blue -NoNewline
# Write-Host " (non-bold text)" -ForegroundColor White
 

$COM_PORT = "COM3"

Write-Host "COM port " ${COM_PORT}
# python script to execute 
$Upload_cmd = "python -m esptool --chip esp32 -p $COM_PORT -b 460800 --before=default_reset --after=hard_reset write_flash --verify --flash_mode dio --flash_freq 40m --flash_size 4MB 0x1000 bootloader.bin 0x10000 EASE.bin 0x8000 partition-table.bin 0xd000 serial_number.bin"

$serial_python_script = "python change_serial.py"


# Loop to continuously read keyboard input
while ($true) {
    # Prompt the user for input
    Write-Host "Enter a command (type 'e' to quit , 'u' to upload program ):" -ForegroundColor Yellow
    $input = Read-Host

     # Check if the user wants to exit the loop
    if ($input -eq 'e')
    {
        Write-Host "Exiting the script." -ForegroundColor Red
        break
    }
    
    elseif($input -eq 'u')
    {
        # invoke other command to change the serail number in test bin 
    # Execute the command using Invoke-Expression
    Invoke-Expression $serial_python_script
    
    if ($LASTEXITCODE -eq 0) {
        
        Invoke-Expression $Upload_cmd
        
        # check here the execution of second python program 
        if ($LASTEXITCODE -eq 0) 
        {
            Write-Host "done programming Device "  -ForegroundColor Green
        }
        else 
        {
            Write-Host "Error in programming the Device " -ForegroundColor Red
        }

    }
    else
    {
        Write-Host "cant able to generate the Serial number : invalid serial number Exit code-> $LASTEXITCODE" -ForegroundColor Red
    }
        
    }

    else 
    {

    #  Execute the shell command based on the user input
    try {
        # Use the "&" symbol to run the shell command
        & $input
    } catch {
        Write-Host "Error executing the command: $_" -ForegroundColor Red
    }
    }
}

Write-Host "Proggram  Terminated." -ForegroundColor Yellow


