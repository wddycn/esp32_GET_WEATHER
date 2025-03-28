# ESP32学习笔记



## ESP32简介

<img src="C:\Users\ycndr\Pictures\Screenshots\屏幕截图 2025-01-18 020637.png" style="zoom:50%;" />

- 按键按下：下载模式
- 按键放下：运行模式

<img src="C:\Users\ycndr\Pictures\Screenshots\屏幕截图 2025-01-18 021843.png" style="zoom:50%;" />

- BOOT是GPIO0

<img src="./ESP32学习笔记.assets/5b43e54fb0a95b61d6553593942859c-1743072779876-1.png" alt="5b43e54fb0a95b61d6553593942859c" style="zoom:50%;" />

<img src="./ESP32学习笔记.assets/image-20250328121538017.png" alt="image-20250328121538017" style="zoom:50%;" />

IDF 编程指南：https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/index.html

## 天气获取

<img src="./ESP32学习笔记.assets/image-20250328121647239.png" alt="image-20250328121647239" style="zoom:50%;" />

#### 首先在（[天气查询服务 | 百度地图API SDK](https://lbsyun.baidu.com/faq/api?title=webapi/weather/base)） 当中获得

```c
https://api.map.baidu.com/weather/v1/?district_id=222405&data_type=all&ak=你的ak
```

- 222405：是行政区编码；**浦东**：`310115`  **奉贤区**：`310120`  **徐汇区**：`310104`  

- 你的ak：是注册百度开发者账号后点击创建获得

​	Windows系统获得ip地址的方法是输入命令nslookup myip.opendns.com resolver1.opendns.com<br>	如果上一个方法获得的ip不行的话也可以直接输入0.0.0.0/0<br>

- 最后得到奉贤的api服务地址为：

```c
https://api.map.baidu.com/weather/v1/?district_id=310120&data_type=all&ak=MtSR3VBaTAEqxdTKvkKi7iFvqojZig4i
```

- 在浏览器当中输入该网址如果能查询到如图所示，那就是正确的

<img src="./ESP32学习笔记.assets/image-20250327180309119.png" alt="image-20250327180309119" style="zoom:50%;" />

## 创建HTTP项目

- 创建新project：复制`"D:\esp32\esp-idf\v5.3.2\esp-idf\examples\get-started\sample_project"`空白项目到想要保存的文件路径，在这个里面进行新项目的编写<br>而且每次都要生成configuration(配置文件)
- 另外，遇到头文件INCLUDE 下划红线 问题，在安装了“Espressif IDF”插件的前提下，`Shift+Ctrl+P` --> `ESP-IDF:Add vscode configuration folder` 可以解决。小技巧

- 打开`D:\esp32\esp32code\esp-idf-master\examples\protocols\esp_http_client\main\esp_http_client_example.c`<br>引用示例代码的全部头文件

```py
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
```

- 还需要用到TAG用来管理和输出日志

```c
static const char *TAG = "HTTP_CLIENT";
```

- 然后初始化nvs（放在``void app_main() {}``当中）

```c
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
```

- 上面的ESP_LOGI()基于printf进行了封装，用于输出日志，根据最后一个字母区分日志等级

<img src="./ESP32学习笔记.assets/image-20250327191347218.png" alt="image-20250327191347218" style="zoom: 25%;" />

- 接下来需要打开使用`D:\esp32\esp32code\esp-idf-master\examples\common_components\protocol_examples_common`里面的组件<br>在这之前我们需要在我们的`CMakeLists.txt`里面添加一行配置(放在``cmake_minimum_required(VERSION 3.16)``下面)

```c
set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/examples/common_components/protocol_examples_common)
```

- 其次还需要引用头文件（不过之前咱们已经引用过了）

```c
#include "protocol_examples_common.h"
```

## 配置WIFI名称和密码

- 在根目录下创建一个`sdkconfig.defaults`的文件

  打开`D:\esp32\esp32code\esp-idf-master\examples\common_components\protocol_examples_common\wifi_connect.c`<br>找到以下两个参数   *`CONFIG_EXAMPLE_WIFI_SSID     CONFIG_EXAMPLE_WIFI_PASSWORD`*<br>在`sdkconfig.defaults`文件当中预配置SSID和密码（注意其中不要使用空格）

```c
CONFIG_EXAMPLE_WIFI_SSID="ycn"//自己的WIFI名称
CONFIG_EXAMPLE_WIFI_PASSWORD="ycnycnycn"//自己的WIFI密码
```

## 创建HTTP任务

- 在`D:\esp32\esp32code\esp-idf-master\examples\protocols\esp_http_client\main\esp_http_client_example.c`当中找到以下API调用函数（放在``void app_main() {}``当中）

```c
xTaskCreate(&http_test_task, "http_test_task", 8192, NULL, 5, NULL);
```

*第一个参数：指针指向任务的实际执行函数；第二个参数是任务的名称；第三个参数是任务的堆栈深度，深度越大，可执行的任务和变量越多；第四个参数是传递任务函数的参数指针，可通过此参数访问其他任务或者全局数据；第五个参数是任务的优先级；第六个参数是这个task的句柄，如果你需要在其他task中掌握这个task的生死，就需要用到这个句柄*

- 创建HTTP task回调函数（放在``void app_main() {}``之外）<br>添加gpio的头文件

```c
#include"driver/gpio.h"

#define GPIO_INPUT_PIN GPIO_NUM_9//定义gpio引脚，放在static const char *TAG = "HTTP_CLIENT";之后
```

```c
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
```

## 执行HTTP GET

```c
#define MAX_HTTP_OUTPUT_BUFFER 2048//定义缓冲区的大小，缓冲区用于储存HTTP响应内容

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
    
    esp_http_client_cleanup(client);//释放之前创建的HTTP客户端资源
}
```

- _http_event_handler

```c
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
```

## 截止目前为止的完整代码

```c
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
#include"driver/gpio.h"

static const char *TAG = "HTTP_CLIENT";
#define GPIO_INPUT_PIN GPIO_NUM_9//定义gpio引脚，放在static const char *TAG = "HTTP_CLIENT";之后
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
```

## 解析.json文件

- 因为串口日志当中打印出来的天气信息无法直接使用，所以我们需要对他进行解析
- 要在头文件中引用cjson.h

```c
#include "cjson.h"
```

- 创建一些全局变量，用于储存解析后的天气信息

```c
static int temp = 0;//温度
static char *name;//地区
static char *text;//天气
static char *wind_class;//风力
static int rh;//湿度
```

- 在执行完HTTP_GET之后解析天气信息（替换掉原来的`static void http_rest_with_url(void)`）

```c
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

```

- 但是他这个还是打印不出来，是因为在进行 HTTPS 连接时，TLS 配置没有设置服务器验证选项，导致 TLS 握手失败，进而造成连接失败，并在后续使用时发生了空指针访问，导致 Guru Meditation 错误（LoadProhibited）<br>解决办法是在menuconfig菜单里找到Component config` -> `ESP-TLS（勾选以下两点）

<img src="./ESP32学习笔记.assets/1743171732075.png" alt="1743171732075" style="zoom: 33%;" />





