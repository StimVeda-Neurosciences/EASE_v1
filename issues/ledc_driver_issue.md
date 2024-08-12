###### ledc driver hardware issue 

- ledc driver hardware FSM will not be in perfect state if we restart it with esp_osftware restart , this cause the ledc driver to be in malformed/unintentional state. and also after that the driver also not work correctly 
<br>
- ledc_driver stop function cause driver to be stop working after 3-4 iterations 
if ledc driver stop code is  in  reseting led driver, then it cauese the ledc driver to not work correctly and again the same no light problem  
<!-- ---this might be hardware issue but not sure  -->

#### ledc driver software issue 
- the wait_for_completion not work perfectly in a fade down sceanario as suppose there are two led on with diffrent duty cycle as 
20 % and 60 % initailly then in fading the higher duty takes more time or lower duty fade down quickly, have to find a workaround 