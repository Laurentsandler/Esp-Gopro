#include <stdio.h>
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/gpio.h"
#include "esp_video.h"
#include "esp_h264_enc.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define BOOT_BUTTON_PIN GPIO_NUM_35
#define TAG "CUSTOM_GOPRO"

typedef enum { ACTION_NONE, ACTION_RECORD, ACTION_PHOTO } gopro_action_t;

gopro_action_t detect_action() {
    if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT0) {
        return ACTION_NONE;
    }

    int click_count = 1; 
    int64_t start_time = esp_timer_get_time();
    
    // 800ms window to detect a triple-click
    while (esp_timer_get_time() - start_time < 800000) { 
        if (gpio_get_level(BOOT_BUTTON_PIN) == 0) {
            click_count++;
            while(gpio_get_level(BOOT_BUTTON_PIN) == 0) vTaskDelay(pdMS_TO_TICKS(20));
            vTaskDelay(pdMS_TO_TICKS(50)); // Debounce
        }
    }

    if (click_count >= 3) return ACTION_PHOTO;
    return ACTION_RECORD;
}

void init_gopro_harware()
{
    //Init sd card
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, NULL, NULL);

    // Init video system
    esp_video_init_config_t cam_config = {
        .sensor_name = "OV5647",
        .interface = ESP_VIDEO_IF_MIPI_CSI,
    };
    esp_video_init(&cam_config);
}

void execute_action(gopro_action_t action){
    if (action == ACTION_PHOTO) {
        ESP_LOGI(TAG, "Taking 5MP Photo...");
        //captre photo
        esp_video_sw_capture_frame("/sdcard/GOPRO_IMG.jpg", 2592, 1944, VIDEO_FMT_JPEG);
}
else




}