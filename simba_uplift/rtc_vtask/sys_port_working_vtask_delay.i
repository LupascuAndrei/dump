
static int sys_port_module_init(void)
{
    /* Setup interrupt handler. */
    printf("sleeping some time...\n");
    esp_vTaskDelay(250 / portTICK_PERIOD_MS);
    printf("Done1 ...\n");
    ESP_INTR_DISABLE(ESP32_CPU_INTR_SYS_TICK_NUM);
    esp_xt_set_interrupt_handler(ESP32_CPU_INTR_SYS_TICK_NUM,
                                 (xt_handler)sys_port_tick,
                                 NULL);
    intr_matrix_set(xPortGetCoreID(),
                    ESP32_INTR_SOURCE_TG0_T1_LEVEL,
                    ESP32_CPU_INTR_SYS_TICK_NUM);

    /* Configure and start the system tick timer. */
    ESP32_TIMG1->TIMER[0].ALARMLO = (40000000 / CONFIG_SYSTEM_TICK_FREQUENCY);
    ESP32_TIMG1->TIMER[0].ALARMHI = 0;
    ESP32_TIMG1->TIMER[0].CONFIG = (ESP32_TIMG_TIMER_CONFIG_ALARM_EN
                                    | ESP32_TIMG_TIMER_CONFIG_LEVEL_INT_EN
                                    | ESP32_TIMG_TIMER_CONFIG_DIVIDER(1)
                                    | ESP32_TIMG_TIMER_CONFIG_AUTORELOAD
                                    | ESP32_TIMG_TIMER_CONFIG_INCREASE
                                    | ESP32_TIMG_TIMER_CONFIG_EN);
    ESP32_TIMG1->INT.ENA = 1;
    ESP_INTR_ENABLE(ESP32_CPU_INTR_SYS_TICK_NUM);

    printf("sleeping some time...\n");
    esp_vTaskDelay(250 / portTICK_PERIOD_MS);
    printf("Done2 ...\n");
    return (0);
}
