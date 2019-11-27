#include <tizen.h>
#include <service_app.h>
#include "sleepsensorservice.h"

#include <Ecore.h>
#include <time.h>
#include <stdio.h>
#include <sensor.h>
const char pedoFile [256] = "/opt/usr/media/Downloads/pedoData_%s.csv";
char pedometerFileName [256];
char sleepDetectorFileName [256];
const char sleepDetectorFile [256] = "/opt/usr/media/Downloads/DetectorFile_%s.csv";
char sleepMonitorFileName [256];
const char sleepMonitorFile[256] = "/opt/usr/media/Downloads/MonitorFile_%s.csv";
//char config [256] = "/opt/usr/media/Downloads/config.csv";

char time_file[256];
#ifdef DEBUG
char logFileName [256] = "/opt/usr/media/Images/sleepSensorLog.txt";
#endif

char time_file_30sec [256];

sensor_h sensorSleepDetector;
sensor_h sensorSleepMonitor;
sensor_h sensorPedometer;

sensor_listener_h SleepMonitorlistener; // sensor listener handle
sensor_listener_h SleepDetectorlistener; // sensor listener handle
sensor_listener_h pedometerListener;

static int ppgCounter = 0;
static int accCounter= 0;
static int gravityCounter = 0;
static int hrmCounter = 0;
static int gyroscpoeCounter = 0;
static int pressureCounter = 0;
static int stressCounter = 0;
typedef struct collect {
	float ppg;
	float hrm;
	float accx;
	float accy;
	float accz;
	float grax;
	float gray;
	float graz;
	float gyrx;
	float gyry;
	float gyrz;
	float pressure;
	float stress;
	unsigned long long timestamp ;
}collectedData ;

#define MAXNUM 600
#define SENSORNUM 7
//#define CHECKNUM 68
sensor_type_e sensorTypeList[] = {SENSOR_HRM_LED_GREEN, SENSOR_HRM,SENSOR_ACCELEROMETER,
				SENSOR_GRAVITY, SENSOR_GYROSCOPE, SENSOR_PRESSURE ,SENSOR_HUMAN_STRESS_MONITOR, SENSOR_HUMAN_SLEEP_DETECTOR ,
				SENSOR_HUMAN_SLEEP_MONITOR };
sensor_listener_h listenerList [SENSORNUM];
sensor_h sensorList [SENSORNUM];
bool writeToFileFlag = false;
#define SENSORINTERVAL 50 //20 Hz
#define arraySize  1300

char errorFileName [256]= "/opt/usr/media/Downloads/possible_errors.csv";

collectedData allcollectedData[arraySize];
void endAllSensor();
void startCollectingData();
void startSensor();
void endSleepSensors();


void sensorWriteToFile(){
#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "Writing to file ");
#endif
	//Write the PPG values in the file
	char filename[256];
	sprintf(filename, "/opt/usr/media/Downloads/30sec_%s.csv",time_file_30sec);
	FILE * fp = fopen(filename,"a");
	fprintf(fp , "timestamp\tppg\thrm\taccx\taccy\taccz\tgrax\tgray\tgraz\tgyrx\tgyry\tgrz\tpressure\tstress\n");
	for(int i = 0; i < ppgCounter; i++ ) {
		fprintf (fp, "%llu\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\t%.2f\n",
				allcollectedData[i].timestamp,allcollectedData[i].ppg,allcollectedData[i].hrm,
				allcollectedData[i].accx,allcollectedData[i].accy, allcollectedData[i].accz,
				allcollectedData[i].grax, allcollectedData[i].gray, allcollectedData[i].graz,
				allcollectedData[i].gyrx, allcollectedData[i].gyry, allcollectedData[i].gyrz,
				allcollectedData[i].pressure, allcollectedData[i].stress);
	}
	fclose (fp);
	writeToFileFlag = false;
}

unsigned long long getCurrentTimeStamp (){
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	unsigned long long ts = spec.tv_sec * 1000LL + spec.tv_nsec / 1000000LL;
	return ts;
}
char * getCurrentTimeString(){
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return asctime (timeinfo);
}
void setTimeStamp(){
	allcollectedData[ppgCounter].timestamp = getCurrentTimeStamp();
}
void setGravity(sensor_event_s *event){
	allcollectedData[gravityCounter].grax = event->values[0];
	allcollectedData[gravityCounter].gray = event->values[1];
	allcollectedData[gravityCounter].graz= event->values[2];
	gravityCounter++;
}
void setGyroscope (sensor_event_s *event){
	allcollectedData[gyroscpoeCounter].gyrx = event->values[0];
	allcollectedData[gyroscpoeCounter].gyry = event->values[1];
	allcollectedData[gyroscpoeCounter].gyrz = event->values[2];
	gyroscpoeCounter++;
}
void setAccelerometter(sensor_event_s *event){
	allcollectedData[accCounter].accx = event->values[0];
	allcollectedData[accCounter].accy = event->values[1];
	allcollectedData[accCounter].accz = event->values[2];
	accCounter++;
}
void setPressure (sensor_event_s *event){
	allcollectedData[pressureCounter].pressure = event->values[0];
	pressureCounter++;
}

void setHeartRate(sensor_event_s *event){
	allcollectedData[hrmCounter].hrm = event->values[0];
	hrmCounter++;
}
void setStress(sensor_event_s *event){
	allcollectedData[stressCounter].stress = event->values[0];
	stressCounter++;
}
//void hrm_check_callback(sensor_h sensor, sensor_event_s *event, void * data){
//	check_count_HRM++;
//	if (event->values[0] < 0){
//		check_count_error_HRM++;
//	}
//	if (check_count_HRM == CHECKNUM){
//		startCollectingData(NULL);
//	}
//}
void setPPG (sensor_event_s *event) {
	allcollectedData[ppgCounter].ppg = event->values[0];
	ppgCounter++;
	if (ppgCounter> MAXNUM){
		endAllSensor();

	}
}
void all_sensor_callback(sensor_h sensor, sensor_event_s *event, void * data){
	sensor_type_e type;
	sensor_get_type(sensor, &type);
	switch (type) {
	case SENSOR_HRM_LED_GREEN:
		//Define the current Timestamp (We define the Timestamp while the PPG value is available)
		setTimeStamp();
		setPPG(event);
		break;
	case SENSOR_HRM:
		setHeartRate(event);
		break;
	case SENSOR_ACCELEROMETER:
		setAccelerometter(event);
		break;
	case SENSOR_GRAVITY:
		setGravity(event);
		break;
	case SENSOR_GYROSCOPE:
		setGyroscope(event);
		break;
	case SENSOR_PRESSURE:
		setPressure(event);
		break;
	case SENSOR_HUMAN_STRESS_MONITOR:
		setStress(event);
		break;
	default:
		break;
	}
}

void endSensor(int index){
	sensor_listener_h listener = listenerList [index];
	sensor_listener_stop(listener);
	sensor_destroy_listener(listener);
}

void endAllSensor(){
	for (int i = 0; i < SENSORNUM ; i ++){
		endSensor(i);
	}
#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "ending All Sensors");
#endif
	if (writeToFileFlag == true){
#ifdef DEBUG
		dlog_print(DLOG_INFO, LOG_TAG, "writing to file!");
#endif
		sensorWriteToFile();
	}
}

void startSensor30Sec (sensor_event_cb callback , int i ){
	bool isSupported = false;
	sensor_type_e sensorType = sensorTypeList[i];
	sensor_is_supported(sensorType,&isSupported);
	if (!isSupported) {
		FILE * fp = fopen(errorFileName, "a");
		fprintf (fp, "%d is not available: %s \n", sensorType, getCurrentTimeString());
		fclose (fp);
		return;
	}
	sensor_get_default_sensor(sensorType, &sensorList[i]);
	sensor_create_listener(sensorList[i], &listenerList [i]);
	sensor_listener_set_event_cb(listenerList [i], SENSORINTERVAL, callback , NULL); //20Hz
	sensor_listener_set_attribute_int(listenerList [i], SENSOR_ATTRIBUTE_PAUSE_POLICY, SENSOR_PAUSE_NONE);
	sensor_listener_set_option(listenerList [i], SENSOR_OPTION_ALWAYS_ON);
	sensor_listener_start(listenerList [i]);
}

void setTimeForFileName(){ // set time in time_file variable
//The current time for the filename
	time_t raw_time;
	struct tm* time_info;
	time(&raw_time);
	time_info = localtime(&raw_time);
	sprintf(time_file_30sec,"%d%s%d%s%d%s%d%s%d",(time_info->tm_year+1900), time_info->tm_mon+1<10? "0" : "", time_info->tm_mon+1, time_info->tm_mday<10? "0" : "", time_info->tm_mday, time_info->tm_hour<10? "0" : "", time_info->tm_hour, time_info->tm_min<10? "0" : "", time_info->tm_min);

}

void initializeCounters(){
	ppgCounter = 0;
	accCounter= 0;
	gravityCounter = 0;
	hrmCounter = 0;
	gyroscpoeCounter = 0;
	pressureCounter = 0;
	stressCounter = 0;
}
void
startCollectingData(){
	setTimeForFileName();
//	float testValue = (check_count_error_HRM + 0.0 ) /check_count_HRM;
//	if ( testValue < 0.5){
		writeToFileFlag = true;
		initializeCounters();
		for (int i  = 0 ; i < SENSORNUM ; i++){
			startSensor30Sec(all_sensor_callback, i);
		}
}


void endSleepSensors(){
	sensor_listener_stop(SleepDetectorlistener);
	sensor_destroy_listener(SleepDetectorlistener);
	sensor_listener_stop(SleepMonitorlistener);
	sensor_destroy_listener(SleepMonitorlistener);
	sensor_listener_stop(pedometerListener);
	sensor_destroy_listener(pedometerListener);
}


void setTimeForWakeupSensorFileName(){ // set time in time_file variable
//The current time for the filename
	time_t raw_time;
	struct tm* time_info;
	time(&raw_time);
	time_info = localtime(&raw_time);
	sprintf(time_file,"%d%s%d%s%d",(time_info->tm_year+1900), time_info->tm_mon+1<10? "0" : "", time_info->tm_mon+1, time_info->tm_mday<10? "0" : "", time_info->tm_mday);

}


void updateFileName(){
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t filenames updated \n", getCurrentTimeString());
	fclose(fp);
#endif
	setTimeForWakeupSensorFileName();
	sprintf(sleepDetectorFileName, sleepDetectorFile,time_file);
	sprintf(sleepMonitorFileName, sleepMonitorFile, time_file);
	sprintf(pedometerFileName,  pedoFile, time_file);
}

void
example_sensor_callback(sensor_h sensor, sensor_event_s *event, void *data)
{
	FILE * fp;
	sensor_type_e type = SENSOR_ALL;

   if ((sensor_get_type(sensor, &type) == SENSOR_ERROR_NONE) && type == SENSOR_HUMAN_SLEEP_DETECTOR) {
	   updateFileName();
		fp = fopen (sleepDetectorFileName, "a");
	   dlog_print(DLOG_DEBUG, LOG_TAG, "sleep");
	   fprintf(fp, "%llu\t%f\t%f\t%f\t%f\n" , getCurrentTimeStamp(), event->values [0] , event->values [1], event->values [2],event->values [3] );
		fclose(fp);
		if (!writeToFileFlag){
			startCollectingData();
		}
   }

   if ((sensor_get_type(sensor, &type) == SENSOR_ERROR_NONE) && type == SENSOR_HUMAN_SLEEP_MONITOR) {
	   fp = fopen (sleepMonitorFileName, "a");
	   	   dlog_print(DLOG_DEBUG, LOG_TAG, "sleep");
		   fprintf(fp, "%llu\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n" , getCurrentTimeStamp(), event->values [0] , event->values [1], event->values [2]
						, event->values [3] , event->values [4], event->values [5], event->values [6]);
	   		fclose(fp);
   }
   if ((sensor_get_type(sensor, &type) == SENSOR_ERROR_NONE) && type == SENSOR_HUMAN_PEDOMETER) {
   	   fp = fopen (pedometerFileName, "a");
   		   fprintf(fp, "%llu\t%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n" , getCurrentTimeStamp(), event->values [0] , event->values [1], event->values [2]
   						, event->values [3] , event->values [4], event->values [5], event->values [6], event->values[7]);
   	   		fclose(fp);
      }
}




void startSensor(){

	if (sensor_get_default_sensor(SENSOR_HUMAN_SLEEP_DETECTOR, &(sensorSleepDetector)) == SENSOR_ERROR_NONE)
	{
		if (sensor_create_listener(sensorSleepDetector, &SleepDetectorlistener) == SENSOR_ERROR_NONE
			&& sensor_listener_set_event_cb(SleepDetectorlistener, 1000, example_sensor_callback, NULL) == SENSOR_ERROR_NONE
			&& sensor_listener_set_option(SleepDetectorlistener, SENSOR_OPTION_ALWAYS_ON) == SENSOR_ERROR_NONE)
		{
			sensor_listener_set_attribute_int(SleepDetectorlistener, SENSOR_ATTRIBUTE_PAUSE_POLICY, SENSOR_PAUSE_NONE);
			if (sensor_listener_start(SleepDetectorlistener) == SENSOR_ERROR_NONE)
			{
#ifdef DEBUG
				dlog_print(DLOG_INFO, LOG_TAG, "sleep detector listener started.");
#endif
			}
		}
	}
	if (sensor_get_default_sensor(SENSOR_HUMAN_SLEEP_MONITOR, &(sensorSleepMonitor)) == SENSOR_ERROR_NONE)
	{
		if (sensor_create_listener(sensorSleepMonitor, &SleepMonitorlistener) == SENSOR_ERROR_NONE
			&& sensor_listener_set_event_cb(SleepMonitorlistener, 1000, example_sensor_callback, NULL) == SENSOR_ERROR_NONE
			&& sensor_listener_set_option(SleepMonitorlistener, SENSOR_OPTION_ALWAYS_ON) == SENSOR_ERROR_NONE)
		{
			sensor_listener_set_attribute_int(SleepMonitorlistener, SENSOR_ATTRIBUTE_PAUSE_POLICY, SENSOR_PAUSE_NONE);
			if (sensor_listener_start(SleepMonitorlistener) == SENSOR_ERROR_NONE)
			{
#ifdef DEBUG
				dlog_print(DLOG_INFO, LOG_TAG, "sleep monitor listener started.");
#endif
			}
		}
	}
	if (sensor_get_default_sensor(SENSOR_HUMAN_PEDOMETER, &(sensorPedometer)) == SENSOR_ERROR_NONE)
		{
			if (sensor_create_listener(sensorPedometer, &pedometerListener) == SENSOR_ERROR_NONE
				&& sensor_listener_set_event_cb(pedometerListener, 1000, example_sensor_callback, NULL) == SENSOR_ERROR_NONE
				&& sensor_listener_set_option(pedometerListener, SENSOR_OPTION_ALWAYS_ON) == SENSOR_ERROR_NONE)
			{
				sensor_listener_set_attribute_int(pedometerListener, SENSOR_ATTRIBUTE_PAUSE_POLICY, SENSOR_PAUSE_NONE);
				if (sensor_listener_start(pedometerListener) == SENSOR_ERROR_NONE)
				{
#ifdef DEBUG
					dlog_print(DLOG_INFO, LOG_TAG, "pedometer listener started.");
#endif
				}
			}
		}
}

bool service_app_create(void *data)
{
	dlog_print(DLOG_INFO, LOG_TAG, "Sleep sensor service started!");
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app create function \n", getCurrentTimeString());
	fclose(fp);
#endif

	startCollectingData();
	updateFileName();
    startSensor();
    return true;
}


void service_app_terminate(void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app terminate function\n", getCurrentTimeString());
	fclose(fp);
#endif

endSleepSensors();
    return;
}


void service_app_control(app_control_h app_control, void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app control function \n", getCurrentTimeString());
	fclose(fp);
#endif
	 endSleepSensors();
	 startSensor();

}

static void
service_app_lang_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LANGUAGE_CHANGED*/
	return;
}

static void
service_app_region_changed(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_REGION_FORMAT_CHANGED*/
}

static void
service_app_low_battery(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_BATTERY*/
}

static void
service_app_low_memory(app_event_info_h event_info, void *user_data)
{
	/*APP_EVENT_LOW_MEMORY*/
}

int main(int argc, char* argv[])
{
    char ad[50] = {0,};
	service_app_lifecycle_callback_s event_callback;
	app_event_handler_h handlers[5] = {NULL, };

	event_callback.create = service_app_create;
	event_callback.terminate = service_app_terminate;
	event_callback.app_control = service_app_control;

	service_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY], APP_EVENT_LOW_BATTERY, service_app_low_battery, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY], APP_EVENT_LOW_MEMORY, service_app_low_memory, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED], APP_EVENT_LANGUAGE_CHANGED, service_app_lang_changed, &ad);
	service_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED], APP_EVENT_REGION_FORMAT_CHANGED, service_app_region_changed, &ad);

	return service_app_main(argc, argv, &event_callback, ad);
}
