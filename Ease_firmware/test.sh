# this is a comment 


# @REM # Loop to continuously read keyboard input
while ($true)
{
    # Prompt the user for input
    Write-Host "Enter a command (type 'exit' to quit):"
    $input = Read-Host

     # Check if the user wants to exit the loop
    if ($input -eq 'exit') 
    {
        Write-Host "Exiting the script."
        break
    }

    else if ($input -eq 'hello')
    {
        print($who)
    }

     # Execute the shell command based on the user input
    try 
    {
        # Use the "&" symbol to run the shell command
        & $input
    }
    catch 
    {
        Write-Host "Error executing the command: $_"
    }
}

Write-Host "Script completed."