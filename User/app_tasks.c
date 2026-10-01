#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "Control.h"
#include "Display.h"
#include "Level.h"
#include "Pressure.h"
#include "Serial.h"
#include "TempHeat.h"

static StaticQueue_t g_sensorQueueStorage;
static uint8_t g_sensorQueueBuffer[sizeof(ControlSensorData_t)] __attribute__((aligned(8)));
static QueueHandle_t g_sensorQueue;
static StaticQueue_t g_displayQueueStorage;
static uint8_t g_displayQueueBuffer[sizeof(SystemConfig_t)] __attribute__((aligned(8)));
static QueueHandle_t g_displayQueue;
static StaticQueue_t g_statusQueueStorage;
static uint8_t g_statusQueueBuffer[sizeof(ControlStatusSnapshot_t)] __attribute__((aligned(8)));
static QueueHandle_t g_statusQueue;
static StaticTask_t g_controlTaskTcb;
static StackType_t g_controlTaskStack[384];
static StaticTask_t g_sensorTaskTcb;
static StackType_t g_sensorTaskStack[384];
static StaticTask_t g_uiTaskTcb;
static StackType_t g_uiTaskStack[192];
static StaticTask_t g_serialTaskTcb;
static StackType_t g_serialTaskStack[256];
static TaskHandle_t g_controlTaskHandle;
static TaskHandle_t g_sensorTaskHandle;
static TaskHandle_t g_uiTaskHandle;
static TaskHandle_t g_serialTaskHandle;

static void Serial_SendSystemStatus(const ControlStatusSnapshot_t *statusData) {
	Serial_SendString((uint8_t*)"System Status:\r\n");
	Serial_SendString((uint8_t*)"Current Temp: ");
	Serial_SendNumber(statusData->config.currentTemp, 2);
	Serial_SendString((uint8_t*)"C\r\nState: ");

	switch (statusData->config.systemState) {
		case SYSTEM_OFF:
			Serial_SendString((uint8_t*)"OFF\r\n");
			break;
		case SYSTEM_STANDBY:
			Serial_SendString((uint8_t*)"STANDBY\r\n");
			break;
		case SYSTEM_HEATING:
			Serial_SendString((uint8_t*)"HEATING\r\n");
			break;
		case SYSTEM_KEEPING:
			Serial_SendString((uint8_t*)"KEEPING\r\n");
			break;
		case SYSTEM_ERROR:
			Serial_SendString((uint8_t*)"ERROR\r\n");
			break;
		default:
			Serial_SendString((uint8_t*)"UNKNOWN\r\n");
			break;
	}

	Serial_SendString((uint8_t*)"Fault: ");
	switch (statusData->fault) {
		case SYSTEM_FAULT_TEMPERATURE:
			Serial_SendString((uint8_t*)"Temperature\r\n");
			break;
		case SYSTEM_FAULT_PRESSURE:
			Serial_SendString((uint8_t*)"Pressure Sensor\r\n");
			break;
		case SYSTEM_FAULT_LIQUID_LEVEL:
			Serial_SendString((uint8_t*)"Liquid Level\r\n");
			break;
		case SYSTEM_FAULT_NONE:
		default:
			Serial_SendString((uint8_t*)"None\r\n");
			break;
	}

	Serial_SendString((uint8_t*)"Liquid Level: ");
	Serial_SendString(statusData->config.liquidLevel ? (uint8_t*)"Present\r\n" : (uint8_t*)"Absent\r\n");
	Serial_SendString((uint8_t*)"Pressure: ");
	Serial_SendString(statusData->config.pressureDetected ? (uint8_t*)"Detected\r\n" : (uint8_t*)"Not Detected\r\n");
	Serial_SendString((uint8_t*)"Stack Free (words): C=");
	Serial_SendNumber(uxTaskGetStackHighWaterMark(g_controlTaskHandle), 3);
	Serial_SendString((uint8_t*)" S=");
	Serial_SendNumber(uxTaskGetStackHighWaterMark(g_sensorTaskHandle), 3);
	Serial_SendString((uint8_t*)" U=");
	Serial_SendNumber(uxTaskGetStackHighWaterMark(g_uiTaskHandle), 3);
	Serial_SendString((uint8_t*)" R=");
	Serial_SendNumber(uxTaskGetStackHighWaterMark(g_serialTaskHandle), 3);
	Serial_SendString((uint8_t*)"\r\n");
	Serial_SendString((uint8_t*)"--------------------\r\n");
}

static void ControlTask(void *argument) {
	TickType_t lastWakeTime = xTaskGetTickCount();
	ControlSensorData_t sensorData;
	ControlStatusSnapshot_t statusData;

	(void)argument;
	for (;;) {
		if (xQueueReceive(g_sensorQueue, &sensorData, pdMS_TO_TICKS(100U)) != pdPASS) {
			sensorData.temperatureValid = 0;
			sensorData.pressureValid = 0;
			sensorData.pressureDetected = 0;
			sensorData.liquidLevel = 0;
			sensorData.ready = 1;
		}
		if (sensorData.ready) {
			Control_UpdateFromSensors(&sensorData);
			Control_GetStatusSnapshot(&statusData);
			xQueueOverwrite(g_displayQueue, &statusData.config);
			xQueueOverwrite(g_statusQueue, &statusData);
		}
		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100U));
	}
}

static void SensorTask(void *argument) {
	TickType_t lastWakeTime = xTaskGetTickCount();
	uint16_t slowSampleElapsed = 100U;
	ControlSensorData_t sensorData = {0};
	PressureReadStatus_t pressureStatus;
	TempHeatSampleStatus_t temperatureStatus;
	uint8_t temperatureReady = 0;
	uint8_t pressureReady = 0;

	(void)argument;
	for (;;) {
		temperatureStatus = TempHeat_TryGetCurrentTemp(&sensorData.currentTemp);
		if (temperatureStatus == TEMPHEAT_SAMPLE_READY) {
			sensorData.temperatureValid = 1;
			temperatureReady = 1;
		} else if (temperatureStatus == TEMPHEAT_SAMPLE_ERROR) {
			sensorData.temperatureValid = 0;
			temperatureReady = 1;
		}

		if (slowSampleElapsed >= 100U) {
			sensorData.liquidLevel = Level_Detect();
			slowSampleElapsed = 0;
		}

		pressureStatus = Pressure_TryDetect(&sensorData.pressureDetected);
		if (pressureStatus == PRESSURE_READ_READY) {
			sensorData.pressureValid = 1;
			pressureReady = 1;
		} else if (pressureStatus == PRESSURE_READ_TIMEOUT) {
			sensorData.pressureValid = 0;
			pressureReady = 1;
		}

		sensorData.ready = (uint8_t)(temperatureReady && pressureReady);
		xQueueOverwrite(g_sensorQueue, &sensorData);
		slowSampleElapsed += 10U;
		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10U));
	}
}

static void UiTask(void *argument) {
	TickType_t lastWakeTime = xTaskGetTickCount();
	SystemConfig_t displayData;

	(void)argument;
	for (;;) {
		if (xQueueReceive(g_displayQueue, &displayData, 0) == pdPASS) {
			Display_Update(displayData.targetTemp,
				displayData.currentTemp,
				displayData.systemState);
		}
		Display_CheckButtons();
		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10U));
	}
}

static void SerialTask(void *argument) {
	TickType_t lastWakeTime = xTaskGetTickCount();
	TickType_t lastStatusTime = lastWakeTime;
	ControlStatusSnapshot_t statusData = {0};

	(void)argument;
	Control_GetStatusSnapshot(&statusData);
	for (;;) {
		xQueueReceive(g_statusQueue, &statusData, 0);

		if ((xTaskGetTickCount() - lastStatusTime) >= pdMS_TO_TICKS(1000U)) {
			lastStatusTime += pdMS_TO_TICKS(1000U);
			Serial_SendSystemStatus(&statusData);
		}

		if (Serial_GetRxFlag()) {
			uint8_t rxData = Serial_ReceiveByte();
			switch (rxData) {
				case 's':
					Serial_SendSystemStatus(&statusData);
					break;
				case 'r':
					Control_RequestReset();
					Serial_SendString((uint8_t*)"Reset requested\r\n");
					break;
				case 'h':
					Serial_SendString((uint8_t*)"Help:\r\n");
					Serial_SendString((uint8_t*)"s - Send system status\r\n");
					Serial_SendString((uint8_t*)"r - Reset system fault\r\n");
					Serial_SendString((uint8_t*)"h - Show help\r\n");
					break;
				default:
					Serial_SendString((uint8_t*)"Unknown command. Type 'h' for help.\r\n");
					break;
			}
		}

		vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(10U));
	}
}

void AppTasks_Create(void) {
	g_sensorQueue = xQueueCreateStatic(1U,
		sizeof(ControlSensorData_t),
		g_sensorQueueBuffer,
		&g_sensorQueueStorage);
	configASSERT(g_sensorQueue != 0);
	g_displayQueue = xQueueCreateStatic(1U,
		sizeof(SystemConfig_t),
		g_displayQueueBuffer,
		&g_displayQueueStorage);
	configASSERT(g_displayQueue != 0);
	g_statusQueue = xQueueCreateStatic(1U,
		sizeof(ControlStatusSnapshot_t),
		g_statusQueueBuffer,
		&g_statusQueueStorage);
	configASSERT(g_statusQueue != 0);

	g_sensorTaskHandle = xTaskCreateStatic(SensorTask, "Sensor", 384U, 0, 2U, g_sensorTaskStack, &g_sensorTaskTcb);
	configASSERT(g_sensorTaskHandle != 0);
	g_controlTaskHandle = xTaskCreateStatic(ControlTask, "Control", 384U, 0, 3U, g_controlTaskStack, &g_controlTaskTcb);
	configASSERT(g_controlTaskHandle != 0);
	g_uiTaskHandle = xTaskCreateStatic(UiTask, "Ui", 192U, 0, 2U, g_uiTaskStack, &g_uiTaskTcb);
	configASSERT(g_uiTaskHandle != 0);
	g_serialTaskHandle = xTaskCreateStatic(SerialTask, "Serial", 256U, 0, 1U, g_serialTaskStack, &g_serialTaskTcb);
	configASSERT(g_serialTaskHandle != 0);
}