#include "nvs_lib.h"


void save_log_info(uint64_t eeg_time , uint64_t tdcstime, int error)
{
    // eeg_time = eeg_time * smp_dur;

    printf("save eeg time\r\n");
    save_eeg_time(eeg_time);
    printf("save tdcs time\r\n");
    save_tdcs_time(tdcstime);
    printf("save error code\r\n");
    set_err_code(error);

} 


void save_tdcs_time(uint64_t time)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_set_u64(my_handle, tdcs_time, time);
    if (err != ESP_OK)
        error_hand();

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
        error_hand();

    // Close
    nvs_close(my_handle);
}

void save_eeg_time(uint64_t time)
{
     nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_set_u64(my_handle,EEG_time, time);
    if (err != ESP_OK)
        error_hand();

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
        error_hand();

    // Close
    nvs_close(my_handle);
}

void set_err_code(int error)
{
    nvs_handle_t my_handle;
    esp_err_t err;
    
    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_set_i32(my_handle,error_code ,error);
    if (err != ESP_OK)
        error_hand();

    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
        error_hand();

    // Close
    nvs_close(my_handle);
    
}

/////////////////// save tdcs time 
void save_fuel_gauge_nvs(uint16_t arr[])
{
    
     nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_set_u16(my_handle,RC_COMP0, arr[0]);
    if (err != ESP_OK)
        error_hand();
        
    err = nvs_set_u16(my_handle,TEMP_CO, arr[1]);
    if (err != ESP_OK)
        error_hand();

    err = nvs_set_u16(my_handle,FULL_CAP_REP, arr[2]);
    if (err != ESP_OK)
        error_hand();
        
    err = nvs_set_u16(my_handle,Saved_cycle, arr[3]);
    if (err != ESP_OK)
        error_hand();
        
    err = nvs_set_u16(my_handle,FUL_CAP_NOM, arr[4]);
    if (err != ESP_OK)
        error_hand();
    // Commit written value.
    // After setting any values, nvs_commit() must be called to ensure changes are written
    // to flash storage. Implementations may write to storage at other times,
    // but this is not guaranteed.
    err = nvs_commit(my_handle);
    if (err != ESP_OK)
        error_hand();

    // Close
    nvs_close(my_handle);

}

//////////////////////////////////////////////
////////////////////////////

void get_fuel_gauge_nvs(uint16_t arr[])
{

    
     nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_get_u16(my_handle,RC_COMP0, arr);
    if (err != ESP_OK)
        error_hand();
        
    err = nvs_get_u16(my_handle,TEMP_CO, arr+1);
    if (err != ESP_OK)
        error_hand();

    err = nvs_get_u16(my_handle,FULL_CAP_REP, arr+2);
    if (err != ESP_OK)
        error_hand();
        
    err = nvs_get_u16(my_handle,Saved_cycle, arr+3);
    if (err != ESP_OK)
        error_hand();
        
    err = nvs_get_u16(my_handle,FUL_CAP_NOM, arr+4);
    if (err != ESP_OK)
        error_hand();

    // Close
    nvs_close(my_handle);
}

uint64_t get_eeg_time(void)
{
    uint64_t time;
     nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_get_u64(my_handle, EEG_time, &time);
    if (err != ESP_OK)
        error_hand();

       // Close
    nvs_close(my_handle);
    return time;
}

uint64_t get_tdcs_time(void)
{

    uint64_t time = 0;
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_get_u64(my_handle, tdcs_time, &time);
    if (err != ESP_OK)
        error_hand();

    

    // Close
    nvs_close(my_handle);

    return time;
}


int get_err_code(void)
{

    int error = 0;
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open the entry point

    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        error_hand();

    err = nvs_get_i32(my_handle, error_code, &error);
    if (err != ESP_OK)
        error_hand();

   

    // Close
    nvs_close(my_handle);

    return error;

}

void nvs_initialize(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    ESP_ERROR_CHECK(err);
}