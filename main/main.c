#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_zigbee_core.h"
#include "ha/esp_zigbee_ha_standard.h"

#define SENSOR_ENDPOINT                 1
#define ZIGBEE_CHANNEL_MASK             (1 << 11) 

static const char *TAG = "NO_ZIGBEE_GRUPO2";
static bool task_criada = false;

typedef struct {
    float temperatura;
    float umidade;
} DadosClima;

static DadosClima ler_sensor(void) {
    DadosClima dados = {25.4, 60.5};
    return dados;
}

static void zigbee_enviar_dados_task(void *pvParameters) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(15000)); 

        if (esp_zb_lock_acquire(portMAX_DELAY)) {
            DadosClima clima = ler_sensor();
            int16_t temp_zb = (int16_t)(clima.temperatura * 100);
            uint16_t umid_zb = (uint16_t)(clima.umidade * 100);

            esp_zb_zcl_set_attribute_val(SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                                         ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID,
                                         &temp_zb, false);
            
            esp_zb_zcl_report_attr_cmd_t report_temp = {
                .zcl_basic_cmd = { .dst_addr_u = { .addr_short = 0x0000 }, .dst_endpoint = SENSOR_ENDPOINT, .src_endpoint = SENSOR_ENDPOINT },
                .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                .clusterID = ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
                .attributeID = ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID
            };
            esp_zb_zcl_report_attr_cmd_req(&report_temp);

            esp_zb_zcl_set_attribute_val(SENSOR_ENDPOINT, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
                                         ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID,
                                         &umid_zb, false);

            esp_zb_zcl_report_attr_cmd_t report_umid = {
                .zcl_basic_cmd = { .dst_addr_u = { .addr_short = 0x0000 }, .dst_endpoint = SENSOR_ENDPOINT, .src_endpoint = SENSOR_ENDPOINT },
                .address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT,
                .clusterID = ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
                .attributeID = ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID
            };
            esp_zb_zcl_report_attr_cmd_req(&report_umid);

            esp_zb_lock_release();
            ESP_LOGI(TAG, "📡 Dados enviados -> Temp: %.1fC | Umid: %.1f%%", clima.temperatura, clima.umidade);
        }
    }
}

static void tentar_reconectar_task(void *pvParameters) {
    vTaskDelay(pdMS_TO_TICKS(3000));
    esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
    vTaskDelete(NULL);
}

void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal) {
    uint32_t *p_sg_p = signal->p_app_signal;
    esp_err_t err_status = signal->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;

    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        esp_zb_bdb_start_top_level_commissioning(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK && !task_criada) {
            ESP_LOGI(TAG, "✅ Conectado! Iniciando tarefa...");
            // Prioridade reduzida para 2 e stack aumentada para 8192
            xTaskCreate(zigbee_enviar_dados_task, "zigbee_task", 8192, NULL, 2, NULL);
            task_criada = true;
        } else if (err_status != ESP_OK) {
            xTaskCreate(tentar_reconectar_task, "reconectar", 2048, NULL, 5, NULL);
        }
        break;
    default:
        break;
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_zb_cfg_t zb_cfg = { .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED, .install_code_policy = false };
    esp_zb_init(&zb_cfg);

    esp_zb_attribute_list_t *temp_attr_list = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT);
    esp_zb_temperature_meas_cluster_add_attr(temp_attr_list, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &(int16_t){0});

    esp_zb_attribute_list_t *umid_attr_list = esp_zb_zcl_attr_list_create(ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT);
    esp_zb_humidity_meas_cluster_add_attr(umid_attr_list, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &(uint16_t){0});

    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    esp_zb_cluster_list_add_temperature_meas_cluster(cluster_list, temp_attr_list, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    esp_zb_cluster_list_add_humidity_meas_cluster(cluster_list, umid_attr_list, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);

    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_ep_list_add_ep(ep_list, cluster_list, (esp_zb_endpoint_config_t){SENSOR_ENDPOINT, ESP_ZB_AF_HA_PROFILE_ID, ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID, 0});

    esp_zb_device_register(ep_list);
    esp_zb_set_primary_network_channel_set(ZIGBEE_CHANNEL_MASK);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
}