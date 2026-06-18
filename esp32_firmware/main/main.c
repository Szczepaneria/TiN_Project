#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "driver/mcpwm_prelude.h"

#ifdef DEBUG
#include "esp_log.h"
#endif

#define SERVER_PORT 1234

#define MOTOR_OUT_PWM 18
#define SERVO_OUT_PWM 19

typedef struct
{
    uint32_t motor_duty;
    uint32_t servo_duty;
} drive_settings_t;

volatile drive_settings_t shared_data = {0,0};
static portMUX_TYPE data_mux = portMUX_INITIALIZER_UNLOCKED;

#ifdef DEBUG
static const char *COMM_TAG = "Comm Task";
static const char *MOTOR_TAG = "MotorCtrl Task";
#endif


// Access Point init
void wifi_init_ap(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "car_control",
            .ssid_len = strlen("car_control"),
            .channel = 1,
            .password = "qwerty1234",
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    if (strlen((char*)wifi_config.ap.password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}



// communication task
void comm_receive_task(void *) {

#ifdef DEBUG
    ESP_LOGI(COMM_TAG, "Comm task core: %d", xPortGetCoreID());
#endif

    uint8_t rx_buffer[128];
    struct sockaddr_in car_addr;

    car_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    car_addr.sin_family = AF_INET;
    car_addr.sin_port = htons(SERVER_PORT);

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sock < 0) {
        // error during socket init - add led toggle or smth

#ifdef DEBUG
        ESP_LOGE(COMM_TAG, "Socket creation failed");
#endif
        close(sock);
        esp_restart();
    }

    if(bind(sock, (struct sockaddr *)&car_addr, sizeof(car_addr)) < 0) {
#ifdef DEBUG
        ESP_LOGE(COMM_TAG, "Socket creation failed");
#endif
        close(sock);
        esp_restart();
    }

    // 1s timeout for communication loss - avoiding losing control over car
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


    while(1) {
        struct sockaddr_in source_addr;
        socklen_t socklen = sizeof(source_addr);
        int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);

        if(len < 0) {

            portENTER_CRITICAL(&data_mux);
            shared_data.motor_duty = 1500;
            shared_data.servo_duty = 1500;
            portEXIT_CRITICAL(&data_mux);

#ifdef DEBUG
            ESP_LOGI(COMM_TAG, "Timeout while receiving control data");
#endif
            continue;
        }
        
        if (len != 8) {
#ifdef DEBUG
            ESP_LOGW(COMM_TAG, "Incorrect control packet size: %d B", len);
#endif
            continue;
        }
        uint32_t rx_motor, rx_servo;

        memcpy(&rx_motor, rx_buffer, 4);
        memcpy(&rx_servo, rx_buffer + 4, 4);

        if ((rx_servo < 1000) || (rx_servo > 2000)) {
#ifdef DEBUG
            ESP_LOGW(COMM_TAG, "Incorrect servo control value: %d", rx_servo);
#endif
            continue;
        }

        portENTER_CRITICAL(&data_mux);
        shared_data.motor_duty = rx_motor;
        shared_data.servo_duty = rx_servo;
        portEXIT_CRITICAL(&data_mux);

#ifdef DEBUG
        ESP_LOGI(COMM_TAG, "%ld", shared_data.servo_duty);
#endif
    }
}   

// motor control task
void motor_control_task(void *) {

#ifdef DEBUG
    ESP_LOGI(MOTOR_TAG, "Motor control task core: %d", xPortGetCoreID());
#endif

    // timer config
    mcpwm_timer_handle_t timer = NULL;

    mcpwm_timer_config_t timer_conf;
    timer_conf.group_id = 0;
    timer_conf.clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT;
    timer_conf.resolution_hz = 1000000;
    timer_conf.period_ticks = 20000;
    timer_conf.count_mode = MCPWM_TIMER_COUNT_MODE_UP;

    mcpwm_new_timer(&timer_conf, &timer);

    // operator config
    mcpwm_oper_handle_t oper = NULL;
    mcpwm_operator_config_t oper_conf;
    oper_conf.group_id = 0;

    mcpwm_new_operator(&oper_conf, &oper);
    mcpwm_operator_connect_timer(oper, timer);

    mcpwm_cmpr_handle_t cmpr_motor = NULL;
    mcpwm_cmpr_handle_t cmpr_servo = NULL;
    mcpwm_comparator_config_t cmpr_conf;
    cmpr_conf.flags.update_cmp_on_tez = true;

    mcpwm_new_comparator(oper, &cmpr_conf, &cmpr_motor);
    mcpwm_new_comparator(oper, &cmpr_conf, &cmpr_servo);

    // output GPIO config
    mcpwm_gen_handle_t motor_out = NULL, servo_out = NULL;
    mcpwm_generator_config_t motor_out_conf;
    motor_out_conf.gen_gpio_num = MOTOR_OUT_PWM;

    mcpwm_generator_config_t servo_out_conf;
    servo_out_conf.gen_gpio_num = SERVO_OUT_PWM;

    mcpwm_new_generator(oper, &motor_out_conf, &motor_out);
    mcpwm_new_generator(oper, &servo_out_conf, &servo_out);

    // setting timer and compare actions
    mcpwm_generator_set_action_on_timer_event(motor_out, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(motor_out, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, cmpr_motor, MCPWM_GEN_ACTION_LOW));
    
    mcpwm_generator_set_action_on_timer_event(servo_out, MCPWM_GEN_TIMER_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, MCPWM_TIMER_EVENT_EMPTY, MCPWM_GEN_ACTION_HIGH));
    mcpwm_generator_set_action_on_compare_event(servo_out, MCPWM_GEN_COMPARE_EVENT_ACTION(MCPWM_TIMER_DIRECTION_UP, cmpr_servo, MCPWM_GEN_ACTION_LOW));
    
    // enable timer
    mcpwm_timer_enable(timer);
    mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP);


    drive_settings_t pwm_settings;

    TickType_t xLastWakeTime = xTaskGetTickCount();

    while(1) {
        portENTER_CRITICAL(&data_mux);
        pwm_settings = shared_data;
        portEXIT_CRITICAL(&data_mux);

#ifndef HIL
        mcpwm_comparator_set_compare_value(cmpr_motor, pwm_settings.motor_duty);
        mcpwm_comparator_set_compare_value(cmpr_servo, pwm_settings.servo_duty);
        
        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
#else        
        printf("HIL:%lu:%lu\n", pwm_settings.motor_duty, pwm_settings.servo_duty);

        xTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
#endif
    }
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_ap();

    xTaskCreatePinnedToCore(comm_receive_task, "comm_task", 4096, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(motor_control_task, "motor_ctrl_task", 4096, NULL, 5, NULL, 1);
}
