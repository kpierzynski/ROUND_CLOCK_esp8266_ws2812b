#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#include <FreeRTOS.h>
#include <task.h>

#include <esp/uart.h>
#include <espressif/esp_common.h>

#include <ssid_config.h>

#include <sntp.h>
#include <ws2812_i2s/ws2812_i2s.h>

/* Delay with RTos define */
#define delay_ms(ms) vTaskDelay((ms) / portTICK_PERIOD_MS)
/* */

/* */
#define LED_COUNT 60
#define NTP_REFRESH 16
#define TIMER_FREQ_HZ 3

#define HOUR_COLOR 32,0,0
#define MINUTE_COLOR 0,4,0
#define SECOND_COLOR 0,0,16
/* */

/* Variables */
time_t ts;
struct tm *tms;

ws2812_pixel_t pixels[LED_COUNT];
/* */

/* Functions */
ws2812_pixel_t color_set( uint8_t r, uint8_t g, uint8_t b ) {
    ws2812_pixel_t color = {{b,g,r,0}};
    return color;
}

ws2812_pixel_t color_add( ws2812_pixel_t a, ws2812_pixel_t b) {
    ws2812_pixel_t color = {{a.blue + b.blue, a.green + b.green, a.red + b.red, 0}};
    return color;
}
/* */

/* Timer for updating clock */
void clock_update( void * arg ) {
    /* Time refresh */
    ts = time( NULL );
    tms = localtime( &ts );

    uint8_t i;
    /* */

    /* Debug time printf */
    printf( "TIME: %d %d %d\n", tms->tm_hour, tms->tm_min, tms->tm_sec );
    /* */
    
    /* Clear all leds */
    memset( pixels, 0, sizeof(ws2812_pixel_t) * LED_COUNT );
    /* */

    /* Turn on diodes based on time */
    uint8_t indexs[3] = { (((tms->tm_hour*5) % LED_COUNT)+(tms->tm_min/15)) % LED_COUNT, tms->tm_min % LED_COUNT, tms->tm_sec % LED_COUNT };
    ws2812_pixel_t colors[3] = { color_set(HOUR_COLOR), color_set(MINUTE_COLOR), color_set(SECOND_COLOR) };
    
    for( i = 0; i < 3; i++ ) pixels[indexs[i]] = color_add( colors[i], pixels[indexs[i]]);
    /* */

    /* Push data to leds */
    ws2812_i2s_update(pixels, PIXEL_RGB);
    /* */
}
/* */

/* Configuration task */
void configuration( void * pvParameters ) {

    /* Wait for wifi connection*/
    while (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
	    delay_ms(50);
	}
    /* */
    
    /* Some ntp servers*/
    const char *servers[] = {"0.pool.ntp.org", "1.pool.ntp.org", "2.pool.ntp.org", "3.pool.ntp.org"};
    /* */

    /* Setup ntp*/
    LOCK_TCPIP_CORE();                                              //Lock core for sync tcpip thread
        sntp_set_update_delay(NTP_REFRESH*60000);                   //NTP server update each NTP_REFRESH minute
        const struct timezone tz = {2*60, 0};                       //Set timezone 
        sntp_initialize(&tz);                                       //Init ntp
        sntp_set_servers(servers, sizeof(servers) / sizeof(char*)); //Pass servers to ntp module
    UNLOCK_TCPIP_CORE();                                            //Unlock core
    /* */

    /* Init ws2818 bus on i2s */
    ws2812_i2s_init(LED_COUNT, PIXEL_RGB);
    /* */

    /* Setup timer with 1Hz frequency */
    timer_set_interrupts(FRC1, false);
    timer_set_run(FRC1, false);

    _xt_isr_attach(INUM_TIMER_FRC1, clock_update, NULL);

    timer_set_frequency(FRC1, TIMER_FREQ_HZ);
    timer_set_interrupts(FRC1, true);

    timer_set_run(FRC1, true);
    /* */

    /* Delete configuration task */
    printf("Config task done.\n");
    vTaskDelete(NULL);
    /* */
}
/* */

/* void main( void ) */
void user_init(void)
{
    /* Configure uart */
    uart_set_baud(0, 115200);
    /* */

    /* Configure ssid and password for wifi sta connection */
    struct sdk_station_config config = {
        .ssid = WIFI_SSID,
        .password = WIFI_PASS,
    };
    /* */

    /* Setup wifi */
    sdk_wifi_set_opmode(STATION_MODE);
    sdk_wifi_station_set_config(&config);
    /* */

    /* Create task for configuration */
    xTaskCreate(&configuration, "configuration", 2048, NULL, 1 | portPRIVILEGE_BIT, NULL);
    /* */
}
/* */