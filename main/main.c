#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <stdlib.h>
#include <ctype.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "protocol_examples_utils.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_http_client.h"
#include "driver/gpio.h"
#include "cjson.h"

static int temp = 0;//温度
static char *name;//地区
static char *text;//天气
static char *wind_class;//风力
static int rh;//湿度

static const char *TAG = "HTTP_CLIENT";
#define GPIO_INPUT_PIN GPIO_NUM_0//定义gpio引脚，放在static const char *TAG = "HTTP_CLIENT";之后
#define MAX_HTTP_OUTPUT_BUFFER 2048//定义缓冲区的大小，缓冲区用于储存HTTP响应内容

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
    static char *output_buffer;  // Buffer to store response of http request from event handler
    static int output_len;       // Stores number of bytes read
    switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            // Clean the buffer in case of a new request
            if (output_len == 0 && evt->user_data) {
                // we are just starting to copy the output data into the use
                memset(evt->user_data, 0, MAX_HTTP_OUTPUT_BUFFER);
            }
            /*
             *  Check for chunked encoding is added as the URL for chunked encoding used in this example returns binary data.
             *  However, event handler can also be used in case chunked encoding is used.
             */
            if (!esp_http_client_is_chunked_response(evt->client)) {
                // If user_data buffer is configured, copy the response into the buffer
                int copy_len = 0;
                if (evt->user_data) {
                    // The last byte in evt->user_data is kept for the NULL character in case of out-of-bound access.
                    copy_len = MIN(evt->data_len, (MAX_HTTP_OUTPUT_BUFFER - output_len));
                    if (copy_len) {
                        memcpy(evt->user_data + output_len, evt->data, copy_len);
                    }
                } else {
                    int content_len = esp_http_client_get_content_length(evt->client);
                    if (output_buffer == NULL) {
                        // We initialize output_buffer with 0 because it is used by strlen() and similar functions therefore should be null terminated.
                        output_buffer = (char *) calloc(content_len + 1, sizeof(char));
                        output_len = 0;
                        if (output_buffer == NULL) {
                            ESP_LOGE(TAG, "Failed to allocate memory for output buffer");
                            return ESP_FAIL;
                        }
                    }
                    copy_len = MIN(evt->data_len, (content_len - output_len));
                    if (copy_len) {
                        memcpy(output_buffer + output_len, evt->data, copy_len);
                    }
                }
                output_len += copy_len;
            }

            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            if (output_buffer != NULL) {
                // Response is accumulated in output_buffer. Uncomment the below line to print the accumulated response
                // ESP_LOG_BUFFER_HEX(TAG, output_buffer, output_len);
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "HTTP_EVENT_DISCONNECTED");
            int mbedtls_err = 0;
            esp_err_t err = esp_tls_get_and_clear_last_error((esp_tls_error_handle_t)evt->data, &mbedtls_err, NULL);
            if (err != 0) {
                ESP_LOGI(TAG, "Last esp error code: 0x%x", err);
                ESP_LOGI(TAG, "Last mbedtls failure: 0x%x", mbedtls_err);
            }
            if (output_buffer != NULL) {
                free(output_buffer);
                output_buffer = NULL;
            }
            output_len = 0;
            break;
        case HTTP_EVENT_REDIRECT:
            ESP_LOGD(TAG, "HTTP_EVENT_REDIRECT");
            esp_http_client_set_header(evt->client, "From", "user@example.com");
            esp_http_client_set_header(evt->client, "Accept", "text/html");
            esp_http_client_set_redirection(evt->client);
            break;
    }
    return ESP_OK;
}

static void http_rest_with_url(void)
{
    char local_response_buffer[MAX_HTTP_OUTPUT_BUFFER + 1] = {0};

    esp_http_client_config_t config = {
        .url = "https://api.map.baidu.com/weather/v1/?district_id=310120&data_type=all&ak=MtSR3VBaTAEqxdTKvkKi7iFvqojZig4i",
        .event_handler = _http_event_handler,		//事件处理器，用于定义在HTTP请求期间处理不同事件的请求函数
        .user_data = local_response_buffer,        	//将local_response_buffer的地址传给HTTP客户端，在执行HTTP请求后，服务器的响应数据会被储存在local_response_buffer中，在请求完成后，你可以使用local_response_buffer来访问HTTP响应的内容
        .disable_auto_redirect = true,
    };
    esp_http_client_handle_t client = esp_http_client_init(&config);//用以上参数来初始化一个HTTP的句柄client

    esp_err_t err = esp_http_client_perform(client);//执行HTTPGET，得到的结果会储存在err变量当中
    
    //将json响应解析为一个cjson结构体对象
    cJSON *root = cJSON_Parse(local_response_buffer);
    
    //然后逐层获取json对象和字段
    cJSON *result = cJSON_GetObjectItem(root,"result");
    cJSON *location = cJSON_GetObjectItem(result,"location");
    cJSON *now = cJSON_GetObjectItem(result,"now");
    
    //将数据提取到刚才创建的变量中
    name = cJSON_GetObjectItem(location,"name")->valuestring;
    text = cJSON_GetObjectItem(now,"text")->valuestring;
    temp = cJSON_GetObjectItem(now,"temp")->valueint;
    rh = cJSON_GetObjectItem(now,"rh")->valueint;
    wind_class = cJSON_GetObjectItem(now,"wind_class")->valuestring;
    
    //把获取到的天气内容打印出来
    ESP_LOGI(TAG, "地区 %s", name);
    ESP_LOGI(TAG, "天气 %s", text);
    ESP_LOGI(TAG, "温度 %d", temp);
    ESP_LOGI(TAG, "湿度 %d", rh);
    ESP_LOGI(TAG, "风力 %s", wind_class);
             
    //最后删除cjson结构体对象
    cJSON_Delete(root);
    
    esp_http_client_cleanup(client);//释放之前创建的HTTP客户端资源
}

static void http_test_task(void *pvParameters)
{
    while(1){
        if(gpio_get_level(GPIO_INPUT_PIN) == 0){
            http_rest_with_url();//获取天气数据
            vTaskDelay(pdMS_TO_TICKS(100));//延迟100ms
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    vTaskDelete(NULL);//删除这个任务释放资源
}

void app_main(void)
{
    xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);

    /*--------------------------------------------------
nvs:非易失性存储，开发板断电后保留数据
    通常保存一些配置数据，比如WIFI名称和密码
    如果初始化失败，就返回一个错误码到这个变量中
    如果nvs不可用，就擦除nvs的数据
    然后重新初始化
    ESP_ERROR_CHECK()会检查返回的结果，如果返回值不正确就会终止程序

    然后以下三行代码就可以通过API进行WIFI连接
    ESP_LOGI()基于printf进行了封装，用于输出日志
----------------------------------------------------*/
esp_err_t ret = nvs_flash_init();
if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
  ESP_ERROR_CHECK(nvs_flash_erase());
  ret = nvs_flash_init();
}
ESP_ERROR_CHECK(ret);

ESP_ERROR_CHECK(esp_netif_init());
ESP_ERROR_CHECK(esp_event_loop_create_default());
ESP_ERROR_CHECK(example_connect());
ESP_LOGI(TAG, "Connected to AP, begin http example");

}
