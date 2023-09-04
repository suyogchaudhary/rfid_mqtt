#include <inttypes.h>
#include "rc522.h"
#include <stdio.h>
#include "nvs_flash.h"
#include "wifi_connect.h"
#include "mqtt_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_mac.h"

//#define BROKER "mqtt://mqtt.thinger.io:1883"
#define BROKER "mqtt://mqtt.eclipseprojects.io:1883"
//#define BROKER "mqtt://52.87.177.173:1883"
//#define BROKER  "mqtt://mqtt.broker.io:1883"
#define USERNAME "suyog123"
#define DEVICE_CRED "Suyog@123"
#define DEVICE_ID "RFID"
#define TOPIC "CDAC/ACTS/DESD"
//#define TOPIC "v2/suyog123/buckets/RFID/CDAC/ACTS/DESD"
//#define SSID "VOLDEMORT"
#define SSID "acts"
//#define PASSWORD ""
#define PASSWORD "12345678"

static uint8_t external_storage_mac_addr[6] = { 0xf4, 0x96, 0x34, 0x9d, 0xe5, 0x95 };
static const char *TAG_ST = "store";
static char *TAG = "MQTT";
static const char* TAG_RF = "rc522-demo";
static const char *BASE_PATH = "/storage";


static rc522_handle_t scanner;
static esp_mqtt_client_handle_t client;

static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
int mqtt_send(const char *topic, const char *payload);

static void write_file(const char *path, char *content);
static void read_file(const char *path);

static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    gpio_set_level(2, 1);
    char message[50];
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG_RF, "Tag scanned (sn: %" PRIu64 ")", tag->serial_number);
                sprintf(message, "{\"STUDENT SN.\":%llu}", tag->serial_number);
                if(status == DISCONNECTED_FROM_WIFI){
                    write_file("/storage/attendance.txt", message);
                    ESP_LOGI(TAG_ST, "Writen to Txt %s",BASE_PATH);
                    read_file("/storage/attendance.txt");
                }
                else{
                mqtt_send(TOPIC, message);
            }
            break;
        }
    }
    gpio_set_level(2,0);
}

void app_main()
{
    //esp_err_t esp_base_mac_addr_set(const uint8_t *mac)
    esp_iface_mac_addr_set(  external_storage_mac_addr ,ESP_MAC_WIFI_STA);
    gpio_set_direction(2, GPIO_MODE_OUTPUT);
    wl_handle_t wl_handle;
    esp_err_t res;
    esp_vfs_fat_mount_config_t esp_vfs_mount_config = {
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .max_files = 1,
        .format_if_mount_failed = true,
    };
    ESP_ERROR_CHECK(res = esp_vfs_fat_spiflash_mount_rw_wl(BASE_PATH, "storage", &esp_vfs_mount_config, &wl_handle));


    rc522_config_t config = {
        .spi.host = VSPI_HOST,
        .spi.miso_gpio = 25,
        .spi.mosi_gpio = 23,
        .spi.sck_gpio = 19,
        .spi.sda_gpio = 22,
    };
    ESP_ERROR_CHECK(nvs_flash_init());
    wifi_connect_init();
    ESP_ERROR_CHECK(wifi_connect_sta(SSID, PASSWORD, 10000));

    esp_mqtt_client_config_t esp_mqtt_client_config = {
        .broker.address.uri = BROKER,
        .credentials.username = "USER_NAME",
        .credentials.authentication.password = "DEVICE_CRED",
        .credentials.client_id = DEVICE_ID
        //.credentials.authentication.password = DEVICE_CRED
        };
    client = esp_mqtt_client_init(&esp_mqtt_client_config);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL));
    ESP_ERROR_CHECK(esp_mqtt_client_start(client));

    rc522_create(&config, &scanner);
    rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
    rc522_start(scanner);
    ESP_LOGI(TAG_RF, "RFID STARTED SCANNING!!!");

}

void write_file(const char *path, char *content)
{
    FILE *file;
    if (NULL ==(file = fopen(path,"w"))){
    ESP_LOGW(TAG_ST, " WRITE NULL fptr");
    return;
}
    fputs( content, file);
    ESP_LOGI(TAG_ST, "WRITE %s fptr",content);

    fclose(file);

}
void read_file(const char *path){
    char buff[40] = "abcd";
    FILE *file;
    if (NULL ==(file = fopen(path,"r"))){
    ESP_LOGW(TAG_ST, "READ NULL fptr");
    return;
    }
    fgets( buff,40 , file);
    fclose(file);
    ESP_LOGI(TAG_ST, "READ %s",buff);

}



static void mqtt_event_handler(void *event_handler_arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED");
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("topic: %.*s\n", event->topic_len, event->topic);
        printf("message: %.*s\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "ERROR %s", strerror(event->error_handle->esp_transport_sock_errno));
        break;
    default:
        break;
    }
}

int mqtt_send(const char *topic, const char *payload)
{
    return esp_mqtt_client_publish(client, topic, payload, strlen(payload), 1, 0);
}
