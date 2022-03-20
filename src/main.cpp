#include "USER_LOGI.h"
#include "wifi_config.hpp"
#include <Arduino.h>
#include <TM1637.h>
#include <esp_timer.h>
#include <pins_arduino.h>
#include <time.h>

hw_timer_t *time1 = NULL;
uint64_t cnt_tick;
#define MAX_CNT_TICK 1000

/*定义信号量*/
SemaphoreHandle_t sem_time; //信号量 用于时钟更新

void intHandeler(void);
TaskHandle_t hd_update_led;
void taskDispalyTime(void *parm);
void initTask(void);
void initTimer(void);

void setup()
{
  Serial.begin(115200);

  TRACE_LOG(0, "init wifi");
  if (!connect_to_wifi())
  {
    TRACE_LOG(2, "error init WIFI");
    exit(-4);
  }

  TRACE_LOG(0, "init NTP");
  configTzTime("UTC-8", "time2.cloud.tencent.com", "ntp1.aliyun.com", "ntp.ntsc.ac.cn");

  initTask();

  initTimer();
}

void initTask()
{
  sem_time = xSemaphoreCreateBinary();
  if (sem_time == NULL)
    TRACE_LOG(2, "error when create Task_update_led");

  TRACE_LOG(0, "init task update_led");
  hd_update_led = (TaskHandle_t *)xTaskCreate(taskDispalyTime, "update_led", 4096, NULL, 1, NULL);
  if (NULL == hd_update_led)
  {
    TRACE_LOG(1, "error when create Task_update_led");
  }
}

void initTimer()
{
  TRACE_LOG(0, "init timer");
  time1 = timerBegin(0, 80, true);
  if (time1 == NULL)
  {
    TRACE_LOG(2, "error when init timer");
    exit(-4);
  }
  timerAttachInterrupt(time1, intHandeler, true);
  timerAlarmWrite(time1, 1000 * 100, true);
  timerAlarmEnable(time1);
}

void intHandeler()
{
  if (cnt_tick < MAX_CNT_TICK)
  {
    if (0 == cnt_tick % 5)
    {
      xSemaphoreGiveFromISR(sem_time, NULL);
    }
    if (0 == cnt_tick % 15)
    {
    }
  }
  else
  {
    cnt_tick = 0;
  }
}

// pins definitions for TM1637 and can be changed to other ports
#define CLK VSPICLK
#define DIO VSPID

void taskDispalyTime(void *parm)
{
  tm s_time;
  bool ClockPoint = false;
  int8_t TimeDisp[] = {0x00, 0x00, 0x00, 0x00};
  TM1637 tm1637(CLK, DIO);

  TRACE_LOG(0, "init led");
  tm1637.set();
  tm1637.init();
  while (1)
  {
    xSemaphoreTake(sem_time, portMAX_DELAY);
    getLocalTime(&s_time, 500);
    if (TimeDisp[3] != s_time.tm_sec % 10)
    {
      if ((59 == s_time.tm_sec) && (59 == s_time.tm_min))
      {
        TimeDisp[0] = s_time.tm_hour / 10;
        TimeDisp[1] = s_time.tm_hour % 10;
      }
      else
      {
        TimeDisp[0] = s_time.tm_min / 10;
        TimeDisp[1] = s_time.tm_min % 10;
      }
      TimeDisp[2] = s_time.tm_sec / 10;
      TimeDisp[3] = s_time.tm_sec % 10;
      tm1637.display(TimeDisp);
      if (ClockPoint)
        tm1637.point(POINT_ON);
      else
        tm1637.point(POINT_OFF);
      ClockPoint = !ClockPoint;
    }
  }
}

void loop()
{
  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS); // app_main()也被看作一个任务，所以需要设置任务切换
  }
  vTaskSuspendAll(); //不会执行到此，但如果不加上面的死循环则必须用这个指令删除任务防止内存溢出或程序跑飞
  vTaskDelete(hd_update_led);
}
