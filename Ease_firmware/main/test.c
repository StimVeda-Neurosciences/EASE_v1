#include "test.h"

///////// if the test is not defined this whole file skips from build 
#if defined TEST

uint8_t t_flag = 0;

/////////////// extern variables for eeg data capturing
///////////////////// generic queue handle

extern uint32_t data;

void app_main(void)
{
/////////// to test eeg ic
#if defined test_eeg_serial
    /////////////// init the spi bus
    system_init();
    ads_spi_bus_init();

    printf("running test code for eeg serial\r\n");
    uint64_t time_sec = 0;

    int rate = 0;

    uint64_t cntr = 0;
    ///////////// forever loop
    while (1)
    {

        if (t_flag == 0)
        {
            printf("press a to abort readings anytime\r\n");
            printf("enter rate \r\n");
            while (rate <= 0)
            {
                delay(5000);
                scanf("%d", &rate);
            }
            printf("enter time till the eeg runs \r\n");
            while (time_sec <= 0)
            {
                delay(5000);
                scanf("%lld", &time_sec);
            }
            printf("the time is %lld, rate is %d\r\n", time_sec, rate);
            t_flag = 1;
            cntr = 0;
            delay(2000);
            ads_init(rate);
            read_Allreg();
            time_sec *= 1000; ////////convert seconds into milliseconds
        }
        while (time_sec >= cntr) /////////// compare time in msec
        {

            if (rate == 4)
            {
                uint8_t buff[ads_item_size * 4];
                get_data_r4(buff);

                for (uint8_t i = 0; i < 48; i++)
                {
                    printf("%x\t",buff[i]);
                }
                printf("\r\n");

                char var = 0;
                fscanf(stdin, "%c", &var);
                if (var == 'a')
                {
                    printf("abort readings \r\n");
                    break;
                }
                // if (uxQueueMessagesWaiting(ads_queue) >= 4)

                cntr += 4; ////////// 16 msec is the time when sending
            }
            else if (rate == 2)
            {
                uint8_t buff[ads_item_size * 8];
                get_data_r2(buff);
                for (uint8_t i = 0; i < 8; i++)
                {
                    printf("%f,", data_to_float(buff + 3 * i + 2));
                }
                printf("\r\n");

                char var = 0;
                fscanf(stdin, "%c", &var);
                if (var == 'a')
                {
                    printf("abort readings \r\n");
                    break;
                }

                cntr += 2; ////////// 16 msec is the time when sending
            }
        }

        t_flag = 0;
        time_sec = 0;
        rate = 0;
        ads_deint();
        printf("//////////////////read done ///////////\r\nr\n");
    }
    ////////// code here to test eeg in serial

#elif defined test_eeg_ble

    /////////
    printf("statrting app eeg ble\r\n");
    //////////// code here to test eeg in ble

    ads_spi_bus_init();
    ble_init();
    delay(2000);
    printf("running test code for eeg ble \r\n");
    uint64_t time_sec = 0;

    //////////// PACKET NUmber of the device
    uint32_t packet_no = 0;

    uint32_t rate = 0;
    uint64_t cntr = 0;
    ///////////// forever loop
    while (1)
    {

        if (t_flag == 0) ///////// set the values for the parameters
        {
            data = 0;
            printf("enter rate \r\n");
            while (rate <= 0)
            {
                rate = data;
            }
            data = 0;
            printf("enter time till the eeg runs \r\n");
            while (time_sec <= 0)
            {
                time_sec = data;
            }
            printf("the time is %lld, rate is %d\r\n", time_sec, rate);
            t_flag = 1;
            cntr = 0;
            ads_init(rate);
            read_Allreg();
            time_sec *= 1000; ////////convert seconds into milliseconds

            data = 0;
            packet_no = 0;
            delay(2000);
        }
        while (time_sec >= cntr) /////////// compare time in msec
        {

            if (rate == 4)
            {

                //////////// the buff size is 4 sample club + 4 for the serial counter
                uint8_t buff[sample_size * 4 + 4] = {0};

                if (data == 12)
                {
                    printf("abort readings \r\n");
                    break;
                }
                ///////// 4 message recieved from the queue
                if (uxQueueMessagesWaiting(ads_queue) >= 4)
                {
                    for (uint8_t i = 0; i < 4; i++)
                    {
                        xQueueReceive(ads_queue, (buff + i * sample_size - 1), ticks_wait);
                    }
                    ////////// adding the serial counter to the buffer
                    buff[51] = (packet_no & 0xff);
                    buff[50] = (packet_no >> 8) & 0xff;
                    buff[49] = (packet_no >> 16) & 0xff;
                    buff[48] = (packet_no >> 24) & 0xff;
                    ////printing all the data
                    for (uint8_t i = 0; i < 52; i++)
                    {
                        printf("%x", buff[i]);
                    }
                    printf("\r\n");
                    send_notif_eeg(buff, 52);

                    cntr += 16; ////////// 16 msec is the time when sending
                    packet_no++;
                }
            }
            else if (rate == 2)
            {

                uint8_t buff[sample_size * 8 + 4] = {0};

                if (data == 12)
                {
                    printf("abort readings \r\n");
                    break;
                }
                ////////// as we have to send the samples at 16 msec interval we need to pack until 16 msec
                ///////// 8 message recieved from the queue
                if (uxQueueMessagesWaiting(ads_queue) >= 8)
                {
                    for (uint8_t i = 0; i < 8; i++)
                    {
                        xQueueReceive(ads_queue, (buff + i * sample_size - 1), ticks_wait);
                    }

                    ////////// adding the serial counter to the buffer
                    buff[99] = (packet_no & 0xff);
                    buff[98] = (packet_no >> 8) & 0xff;
                    buff[97] = (packet_no >> 16) & 0xff;
                    buff[96] = (packet_no >> 24) & 0xff;

                    ////
                    send_notif_eeg(buff, 100);

                    cntr += 16; ////////// 16 msec is the time when sending
                    packet_no++;
                }

            } ///// rate selection

        } /////// while time < time till rrg
        t_flag = 0;
        time_sec = 0;
        rate = 0;
        data = 0;
        ads_deint();
        printf("//////////////////read done ///////////\r\nr\n");
    } ////////main while
      ////////// code here to test eeg in serial

#endif

///////// to test tdcs Ic
#if defined test_tdc_serial

    printf("init the tdc\r\n");
    func_timer_init();

    tdcs_bus_init();
    tdcs_init();
    read_tdc_reg();
    char c;
    uint16_t ampl = 0;
    uint32_t freq = 0;

    printf("running test code for tdcs testing  \r\n");

    help();
    while (1)
    {

        /////////##################################################################
        //////////######################### to get the data from the serail terminal////////////

        c = fgetc(stdin);

        switch (c)
        {
        case 'c':
            /* code */
            printf("enter the set current \r\n");
            delay(5000);
            fscanf(stdin, "%hd", &ampl);
            printf("%d\r\n", ampl);
            set_Current(ampl);

            printf("aborted \r\n");
            ampl = 0;
            freq = 0;
            break;
        case 'b':
            printf("sine wave\r\n");
            printf("amp is 2ma \r\n");
            printf("enter frequency \r\n");
            while (freq <= 0)
            {
                delay(3000);
                fscanf(stdin, "%d", &freq);
            }

            printf(" freq %d\r\n", freq);
            while (c != 'e')
            {
                c = fgetc(stdin);
                sine_wave(freq); ////////////// sine_wave(amplitude, frequency )
            }
            ampl = 0;
            freq = 0;
            printf("aborted\r\n");
            break;

        case 'a':
            printf("abort\r\n");
            Abort();
            break;
        case 'i':
            incremt();
            break;
        case 'd':
            decrement();
            break;
        case 'r':
            printf("ramp wave\r\n");
            printf("enter amplitude \r\n");
            while (ampl <= 0)
            {
                delay(3000);
                scanf("%hd", &ampl);
            }
            printf("enter frequency \r\n");
            while (freq <= 0)
            {
                delay(3000);
                fscanf(stdin, "%d", &freq);
            }

            printf("amp is %d, freq %d\r\n", ampl, freq);
            while (c != 'e')
            {
                c = fgetc(stdin);     /////////// or you can use fscanf(stdin, "%c", &c);
                ramp_fun(ampl, freq); //////////// ramp_fync(amplitude, frequency)
            }
            printf("aborted\r\n");
            ampl = 0;
            freq = 0;
            break;
        case 's':
            printf("square wave\r\n");
            printf("enter amplitude \r\n");
            while (ampl <= 0)
            {
                delay(3000);
                scanf("%hd", &ampl);
            }
            printf("enter frequency \r\n");
            while (freq <= 0)
            {
                delay(3000);
                fscanf(stdin, "%d", &freq);
            }

            printf("amp is %d, freq %d\r\n", ampl, freq);
            while (c != 'e')
            {
                c = fgetc(stdin);       /////////// or you can use fscanf(stdin, "%c", &c);
                square_uni(ampl, freq); ////////////
            }
            printf("aborted\r\n");
            ampl = 0;
            freq = 0;
            break;
        case 'h':
            help();
            break;
        case 't':
            printf("runnung tdcs\r\n");
            while (c != 'e')
            {
                c = fgetc(stdin);
                run_tdcs();
            }
            break;
        case 'm':
            read_tdc_reg();
            break;
        default:
            break;
        }
    }
    ///// code here to test tdc serial

#elif defined test_tdc_ble
    printf("starting app\r\n");

    func_timer_init();
    ble_init();
    create_err_queue();
    create_sts_queue();

    msg_buff_create();

    xTaskCreatePinnedToCore(generaltask, FX_GEN, general_task_stack_depth, NULL, PRIORTY_4, &general_tsk_handle, app_cpu);
    // vTaskSuspend(handle1);
    printf("the free heap memory is %d\r\n", esp_get_free_heap_size());

    while (1)
    {

        delay(1000);
    }
    //////////// test the fuel gauge ic in ble mode

#endif

/////////// to test fuel gague ic
#if defined test_fuel_serial

    fuel_gauge_config();

    while (1)
    {
        ////////// run the main loop here
        printf("the vcell is %.2f V,soc %d %%, the remcap %dmAh,tte is %ds, current %dmA, temperature %d*C status %X \r\n", get_vcell(), get_soc(), get_remcap(), get_tte(), get_curent(), get_temp(), get_status());
        delay(3000);
    }
    //////// test the fuel gauge ic in serial format

#elif defined test_fuel_ble

#endif

    return;
}

// void generaltask(void *param)
// {
//     uint32_t notf_val = 0;
//     for (;;)
//     {
//         ////// wait for the notification to recieve
//         xTaskNotifyWait(0x00, 0x0f, &notf_val, portMAX_DELAY);

//         if (notf_val == RUN_TDCS)
//         {
//             printf("tdcs_command \r\n");
//             ////////// if notification recieve then check the message buffer to get the data .
//             ///// data have a particular format so the opcode, amplitude, freq, etc get extracted from the recv msg

//             funct_tdcs_data tdc_data = {0};

//             xMessageBufferReceive(gtsk_bfr_handle, &tdc_data, sizeof(tdc_data), 20);
//             if (tdcs_task_handle == NULL)
//                 xTaskCreatePinnedToCore(function_tdcs_task, FX_TDCS, tdcs_task_stack_depth, &tdc_data, PRIORTY_3, &tdcs_task_handle, app_cpu);
//             //////////// send the status that tdcs runs 
//             send_stats_code(status_tdcs_runs);
//             notf_val = NONE;
//         }
//         else if (notf_val == RUN_EEG)
//         {
//             printf("eeg command \r\n");

//             func_eeg_task eeg_data = {0};
//             xMessageBufferReceive(gtsk_bfr_handle, &eeg_data, s(eeg_data), 20);

//             if (eeg_task_handle == NULL)
//                 xTaskCreatePinnedToCore(function_eeg_task, FX_EEG, eeg_task_stack_depth, &eeg_data, PRIORTY_3, &eeg_task_handle, app_cpu);
//             //// send the status that eeg runs 
//             send_stats_code(status_eeg_runs);
//             notf_val = NONE;
//         }

//         else if (notf_val == STOP)
//         {
//             if(tdcs_task_handle != NULL)
//             {
//                 /////////// must call abort function to ramp down slowly
//                 ////xyz()

//                 vTaskDelete(tdcs_task_handle);
//                 tdcs_task_handle = NULL;
//             }
//             else if(eeg_task_handle != NULL)
//             {
//                 ads_deint();
//                 vTaskDelete(eeg_task_handle);
//                 eeg_task_handle = NULL;
//             }
//             send_stats_code(status_idle);
//             printf("stop command\r\n");
//             notf_val = NONE;
//         }
//     }
//     //////////// this function never reach here ///////////////////////////////
//     vTaskDelete(NULL);
// }

// void function_tdcs_task(void *param)
// {
//     funct_tdcs_data *tdc_data = param;
//     printf("opc %d, amp %d, fre %d,time %d\r\n", tdc_data->opcode,
//            tdc_data->amplitude, tdc_data->frequency, tdc_data->time_till_run);
//     // tdcs_init();
//     uint64_t prev_milli = millis();

//     for (;;)
//     {
//         switch (tdc_data->opcode)
//         {
//         case tac_sine_wave:

//             break;

//         case tac_ramp_fun:
//             ramp_fun(tdc_data->amplitude, tdc_data->frequency);
//             break;

//         case tac_square_uni:

//             break;

//         case tac_set_current:
//             break;

//         case tac_tdcs_prot:

//             break;

//         default:
//             break;
//         }

//         /////// some features are pending like check impedance , etc
//         // if(check_impedance() relation threhold_value )
//         // {
//         //  queuesend(error_code);  // this will send the error code to core 0 that will update the error code to phone via ble
//         // break;
//         // }

//         if ((millis() - prev_milli) > tdc_data->time_till_run)
//         {
//             ///// queuesend(status); send the status that
//             send_stats_code(status_idle);
//             break;
//         }
//     }

//     printf("tdcs_func_destroy\r\n");
//     //////////// ramp down the tdcs from here
//     // xyz()
//     ////////////////// do something stuff like changing the status and error codes

//     //////////////// making the task handler to null and void
//     tdcs_task_handle = NULL;
//     ////////////////// delete this task when reach here
//     vTaskDelete(NULL);
// }

// void function_eeg_task(void *param)
// {
//     func_eeg_task *eeg_data = param;

//     printf("rate %d,time till run%d\r\n", eeg_data->rate, eeg_data->timetill_run);

//     ///////// init the eeg hardware
//     uint64_t prev_milli = millis();

//     for (;;)
//     {

//         if ((millis() - prev_milli) > eeg_data->timetill_run)
//         {
//             send_stats_code(status_idle);
//             ///// queuesend(status); send the status that
//             break;
//         }
//     }
//     printf("eeg_func_destroy\r\n");
//     //////////// deinit the eeg hardware
//     // xyz()
//     //////////////// making the task handler to null and void
//     eeg_task_handle = NULL;
//     ///////////// delete the task when reach here
//     vTaskDelete(NULL);
// }



#endif