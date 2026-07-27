/**
 * ESP32-P4 GoPro-style camera - firmware
 * ---------------------------------------
 * Behavior:
 *   - The board spends almost all of its time in deep sleep to save battery.
 *   - Pressing BOOT wakes it up.
 *     - Single click  -> start recording. The board stays awake and keeps
 *                        recording until BOOT is clicked again, then it
 *                        closes the file and goes back to sleep.
 *     - Triple click (within 800 ms of the wake press) -> take one photo,
 *                        then go back to sleep.
 *   - Recording is H.264 (hardware encoded), photos are JPEG. Both are
 *     written straight to the SD card.
 *
 * Hardware APIs used here (esp_video, esp_h264) are real Espressif
 * components for the ESP32-P4's camera/video pipeline, not Arduino APIs -
 * see the README "Firmware & Software" section for how they're pulled in.
 *
 * ============================================================================
 * THINGS TO VERIFY ON YOUR BENCH BEFORE YOU TRUST THIS BLIND
 * (I don't have this exact board + a compiler on hand to build-test against,
 * so I'm flagging every spot where the correct value depends on your exact
 * board revision / component versions rather than pretending certainty)
 * ============================================================================
 *  1. BOOT_BUTTON_PIN (GPIO_NUM_35) - confirm against your Waveshare
 *     ESP32-P4 module's schematic. Kept as-is from the original file since
 *     that's a hardware fact only you can confirm.
 *  2. Deep-sleep wake API - ESP32-P4 is a RISC-V part without the classic
 *     ESP32/S2/S3 RTC_IO peripheral, so it wakes via the newer chip-agnostic
 *     esp_deep_sleep_enable_gpio_wakeup() rather than
 *     esp_sleep_enable_ext0_wakeup(). If your installed ESP-IDF version
 *     disagrees, swap this call for whatever esp_sleep.h exposes for
 *     esp32p4 - it's a one-line change, isolated in configure_wakeup_and_sleep().
 *  3. esp_video_init_config_t - the two fields below (sensor_name, interface)
 *     match the original file. Real-world sensor bring-up usually also needs
 *     I2C port/pins and XCLK config; check esp_video_init.h inside
 *     managed_components/espressif__esp_video after your first `idf.py
 *     reconfigure` and extend this struct if your version requires more.
 *  4. Photo capture assumes the sensor/ISP path can hand back
 *     V4L2_PIX_FMT_JPEG directly at PHOTO_WIDTH x PHOTO_HEIGHT. If that
 *     ioctl/read fails on your setup, drop to a lower resolution first -
 *     this is a known rough edge in some esp_video + sensor combinations.
 *  5. Recording feeds V4L2_PIX_FMT_YUYV frames straight into esp_h264
 *     (which documents YUYV/IYUV support). Field names on
 *     esp_h264_enc_cfg_t/esp_h264_raw_frame_t/esp_h264_enc_frame_t match
 *     Espressif's published usage example; double check them against your
 *     pulled esp_h264 version's esp_h264_enc.h.
 * ============================================================================
 */

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include "linux/videodev2.h"   // V4L2 types/ioctls - esp_video is V4L2-compatible
#include "esp_video_init.h"    // board/sensor bring-up -> registers /dev/videoX
#include "esp_h264_enc.h"      // hardware H.264 encoder

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

#define BOOT_BUTTON_PIN     GPIO_NUM_35   // TODO: confirm against board schematic
#define TAG                 "CUSTOM_GOPRO"

#define CAMERA_DEV_PATH     "/dev/video0"
#define SD_MOUNT_POINT      "/sdcard"
#define PHOTO_PATH          SD_MOUNT_POINT "/GOPRO_IMG.jpg"
#define VIDEO_PATH          SD_MOUNT_POINT "/GOPRO_VID.h264"

#define PHOTO_WIDTH         2592
#define PHOTO_HEIGHT        1944   // OV5647 max resolution

#define VIDEO_WIDTH         1280
#define VIDEO_HEIGHT        720
#define VIDEO_FPS           30

#define MULTI_CLICK_WINDOW_US   800000   // 800 ms window for triple-click

typedef enum { ACTION_NONE, ACTION_RECORD, ACTION_PHOTO } gopro_action_t;

static sdmmc_card_t *s_sd_card = NULL;

// ---------------------------------------------------------------------------
// Button helpers
// ---------------------------------------------------------------------------

static inline bool button_pressed(void)
{
    return gpio_get_level(BOOT_BUTTON_PIN) == 0; // active-low button to GND
}

static void wait_for_release(void)
{
    while (button_pressed()) {
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // debounce
}

static void init_button_gpio(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << BOOT_BUTTON_PIN,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

/**
 * Figures out what the user wants based on how many times BOOT was clicked
 * within MULTI_CLICK_WINDOW_US of waking the board up.
 *   1 click  -> ACTION_RECORD
 *   3+ clicks -> ACTION_PHOTO
 * Returns ACTION_NONE if we didn't wake up because of the button at all
 * (e.g. first power-on, brown-out reset).
 */
static gopro_action_t detect_action(void)
{
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    if (cause != ESP_SLEEP_WAKEUP_GPIO && cause != ESP_SLEEP_WAKEUP_EXT0) {
        return ACTION_NONE;
    }

    // The press that woke us is still being held (that's *why* we woke up).
    // Wait it out first so the loop below doesn't double-count it.
    wait_for_release();

    int click_count = 1; // the wake press itself counts as click #1
    int64_t start_time = esp_timer_get_time();

    while (esp_timer_get_time() - start_time < MULTI_CLICK_WINDOW_US) {
        if (button_pressed()) {
            click_count++;
            wait_for_release();
        }
    }

    ESP_LOGI(TAG, "Detected %d click(s)", click_count);
    return (click_count >= 3) ? ACTION_PHOTO : ACTION_RECORD;
}

// ---------------------------------------------------------------------------
// SD card
// ---------------------------------------------------------------------------

static esp_err_t init_sd_card(void)
{
    ESP_LOGI(TAG, "Mounting SD card at %s", SD_MOUNT_POINT);

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files              = 5,
        .allocation_unit_size   = 16 * 1024,
    };

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot_config,
                                             &mount_config, &s_sd_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD card mount failed (%s)", esp_err_to_name(ret));
    }
    return ret;
}

static void deinit_sd_card(void)
{
    if (s_sd_card) {
        esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_sd_card);
        s_sd_card = NULL;
    }
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

static esp_err_t init_camera(void)
{
    esp_video_init_config_t cam_config = {};
    cam_config.sensor_name = "OV5647";
    cam_config.interface   = ESP_VIDEO_IF_MIPI_CSI;

    esp_err_t ret = esp_video_init(&cam_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Camera init failed (%s)", esp_err_to_name(ret));
    }
    return ret;
}

/**
 * Captures a single JPEG frame from the camera and writes it to out_path.
 */
static esp_err_t capture_photo(const char *out_path)
{
    int fd = open(CAMERA_DEV_PATH, O_RDWR);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open %s", CAMERA_DEV_PATH);
        return ESP_FAIL;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = PHOTO_WIDTH;
    fmt.fmt.pix.height      = PHOTO_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) != 0) {
        ESP_LOGE(TAG, "VIDIOC_S_FMT failed for photo capture");
        close(fd);
        return ESP_FAIL;
    }

    size_t buf_size = fmt.fmt.pix.sizeimage
                           ? fmt.fmt.pix.sizeimage
                           : (size_t)(PHOTO_WIDTH * PHOTO_HEIGHT / 2);

    uint8_t *frame = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!frame) {
        close(fd);
        return ESP_ERR_NO_MEM;
    }

    // On a READWRITE-capable V4L2 device the first read() implicitly starts
    // streaming - no explicit VIDIOC_STREAMON needed for single-shot capture.
    ssize_t n = read(fd, frame, buf_size);

    esp_err_t ret = ESP_OK;
    if (n <= 0) {
        ESP_LOGE(TAG, "Camera read() failed while taking photo");
        ret = ESP_FAIL;
    } else {
        FILE *f = fopen(out_path, "wb");
        if (!f) {
            ESP_LOGE(TAG, "Could not open %s for writing", out_path);
            ret = ESP_FAIL;
        } else {
            fwrite(frame, 1, n, f);
            fclose(f);
            ESP_LOGI(TAG, "Photo saved: %s (%d bytes)", out_path, (int)n);
        }
    }

    heap_caps_free(frame);
    close(fd);
    return ret;
}

/**
 * Records raw frames from the camera, encodes them to H.264 in hardware,
 * and streams the result to out_path. Recording stops as soon as BOOT is
 * clicked again.
 */
static esp_err_t record_video(const char *out_path)
{
    int fd = open(CAMERA_DEV_PATH, O_RDWR);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open %s", CAMERA_DEV_PATH);
        return ESP_FAIL;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = VIDEO_WIDTH;
    fmt.fmt.pix.height      = VIDEO_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; // esp_h264 accepts YUYV/IYUV raw input

    if (ioctl(fd, VIDIOC_S_FMT, &fmt) != 0) {
        ESP_LOGE(TAG, "VIDIOC_S_FMT failed for video recording");
        close(fd);
        return ESP_FAIL;
    }

    size_t frame_size = fmt.fmt.pix.sizeimage
                             ? fmt.fmt.pix.sizeimage
                             : (size_t)(VIDEO_WIDTH * VIDEO_HEIGHT * 2); // YUYV = 2 B/px

    esp_h264_enc_cfg_t enc_cfg = DEFAULT_H264_ENCODER_CONFIG();
    enc_cfg.width  = VIDEO_WIDTH;
    enc_cfg.height = VIDEO_HEIGHT;
    enc_cfg.fps    = VIDEO_FPS;

    esp_h264_enc_handle_t encoder = NULL;
    if (esp_h264_enc_open(&enc_cfg, &encoder) != ESP_H264_ERR_OK) {
        ESP_LOGE(TAG, "Failed to open H.264 encoder");
        close(fd);
        return ESP_FAIL;
    }

    FILE *out = fopen(out_path, "wb");
    if (!out) {
        ESP_LOGE(TAG, "Could not open %s for writing", out_path);
        esp_h264_enc_close(encoder);
        esp_h264_enc_del(encoder);
        close(fd);
        return ESP_FAIL;
    }

    uint8_t *raw_buf = (uint8_t *)heap_caps_aligned_alloc(16, frame_size,
                                                           MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    if (!raw_buf) {
        fclose(out);
        esp_h264_enc_close(encoder);
        esp_h264_enc_del(encoder);
        close(fd);
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "Recording started: %s", out_path);
    int frame_count = 0;

    while (1) {
        ssize_t n = read(fd, raw_buf, frame_size);
        if (n <= 0) {
            ESP_LOGW(TAG, "Camera read() returned %d, stopping recording", (int)n);
            break;
        }

        esp_h264_raw_frame_t in_frame;
        memset(&in_frame, 0, sizeof(in_frame));
        in_frame.raw_data.buffer = raw_buf;
        in_frame.raw_data.len    = n;
        in_frame.pts             = frame_count * (1000 / VIDEO_FPS);

        esp_h264_enc_frame_t out_frame;
        memset(&out_frame, 0, sizeof(out_frame));

        if (esp_h264_enc_process(encoder, &in_frame, &out_frame) == ESP_H264_ERR_OK) {
            for (size_t layer = 0; layer < out_frame.layer_num; layer++) {
                fwrite(out_frame.layer_data[layer].buffer, 1,
                       out_frame.layer_data[layer].len, out);
            }
        }

        frame_count++;

        // A fresh press here means "stop recording".
        if (button_pressed()) {
            wait_for_release();
            ESP_LOGI(TAG, "Stop button detected, ending recording");
            break;
        }
    }

    heap_caps_free(raw_buf);
    fclose(out);
    esp_h264_enc_close(encoder);
    esp_h264_enc_del(encoder);
    close(fd);

    ESP_LOGI(TAG, "Recording saved: %s (%d frames)", out_path, frame_count);
    return ESP_OK;
}

// ---------------------------------------------------------------------------
// Action dispatch + sleep
// ---------------------------------------------------------------------------

static void execute_action(gopro_action_t action)
{
    switch (action) {
        case ACTION_PHOTO:
            ESP_LOGI(TAG, "Taking photo...");
            capture_photo(PHOTO_PATH);
            break;

        case ACTION_RECORD:
            ESP_LOGI(TAG, "Toggling video recording...");
            record_video(VIDEO_PATH);
            break;

        case ACTION_NONE:
        default:
            ESP_LOGI(TAG, "No button action detected, nothing to do");
            break;
    }
}

static void configure_wakeup_and_sleep(void)
{
    // See note (2) at the top of this file: esp32p4 wakes from deep sleep
    // via the newer chip-agnostic GPIO wakeup API rather than ext0/ext1.
    esp_deep_sleep_enable_gpio_wakeup(1ULL << BOOT_BUTTON_PIN, ESP_GPIO_WAKEUP_GPIO_LOW);

    ESP_LOGI(TAG, "Going to sleep - press the button to wake up");
    esp_deep_sleep_start();
}

extern "C" void app_main(void)
{
    init_button_gpio();

    ESP_LOGI(TAG, "Boot, wakeup cause = %d", esp_sleep_get_wakeup_cause());

    gopro_action_t action = detect_action();

    if (action != ACTION_NONE) {
        if (init_sd_card() == ESP_OK) {
            if (init_camera() == ESP_OK) {
                execute_action(action);
            }
            deinit_sd_card();
        }
    }

    configure_wakeup_and_sleep();
}
