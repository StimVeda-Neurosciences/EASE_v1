#### VERSION 1.0.0
- intial firmware  given this to syrma in september for first 25 headbands 
- 


#### VERSION 1.0.1
- bios testing reperform by writing the 1 in the err charactersitcs 
- adding  delay in the ads bios verification process to make the ic ldo stable for reading the device id 
- reseting the tdcs ic in bios verification , so that its current goes to zero and adding a minute delay in that 
- shut down the device automaticaly if the battery goes down to 2 % in advertisement state 

- change the light indication colors  \
--- ======= RED --> Error \
--- ======= YELLOW --> ADV \
--- ======= GREEN --> CONNECTED \
--- ======= PURPLE --> EEG \
--- ======= BLUE --> TDCS \
--- ======= RED_BLINKS --> SHUTTING DOWN \



================ Changes in the late november 2023=========================

- made only one task that is responsible for getting the battery data and other tasks or cpu would access the data through a global structure 
the task have a priority of 2 and this task will autosuspend after 1 iteration , the battery task have to be resume by other task based on their requirements 
- addding saftey in abort checks and also the abort also gives data to bluetooth 
- adding low battery error and it would give by the headband 
- change the battery custom data structure 




### Version 1.0.2
<!-- #### VERSION 1.0.2 -->
- `@todo` have to integrate the headband serial number through a write procedure 
- `@todo` have to make a overwrite protection algorithm for it 


