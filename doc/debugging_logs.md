# DEBUG


### Debugging logs format 

 Type  | Description | Syntax |
----------|-------------|--------|
**Cyeint (Debug logs)**|All the Debug related logs are presented in Cyient Color.| `  ESP_LOGD(LOG_tag, " message with format specifiers like printf %d,%c",variables) `|
**Green (Information logs )**|All the information are presented in the green color |   `  ESP_LOGI(LOG_tag, " message with format specifiers like printf %d,%c",variables) `|
**Yellow (Warning logs)**.|All the Warnings are presented in yellow color |  `  ESP_LOGW(LOG_tag, " message with format specifiers like printf %d,%c",variables) `
**Red (Error Logs)**| All the error messages are present in Red color | `ESP_LOGE(LOG_tag, " message with format specifiers like printf %d,%c",variables) `
**white (normal message)**|All the temporary message that are only present in development process are in white color. because the message size is less than the above logs. so if you want to log a lot amount of data. directly use printf | ` printf(" message with format specifiers%d,%c",variables) `


