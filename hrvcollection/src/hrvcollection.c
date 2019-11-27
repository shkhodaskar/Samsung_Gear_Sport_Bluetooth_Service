#include <tizen.h>
#include <service_app.h>
#include "hrvcollection.h"

#include <Ecore.h>
#include <time.h>
#include <stdio.h>
#include <sensor.h>
#include<stdlib.h>
#include <device/power.h>
#include <string.h>
#include <inttypes.h>
#include <app_alarm.h>

//#define DEBUG


#define SLEEPINTERVAL 1000
#define SENSORINTERVAL 50 //20 Hz
#define arraySize  20000

const char errorFileName [256]= "/opt/usr/media/Downloads/possible_errors.csv";
const char recorderFilename[256]= "/opt/usr/media/Downloads/recorderData_%s.csv";
#ifdef DEBUG
char logFileName [256] = "/opt/usr/media/Images/hrvcollectionLog.txt";
#endif

char time_file[256];
int check_count_error_HRM = 0;
int check_count_HRM = 0;

static int controlCounter = 0;

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

#define MAXNUM 14400
#define SENSORNUM 9
#define CHECKNUM 68
sensor_type_e sensorTypeList[] = {SENSOR_HRM_LED_GREEN, SENSOR_HRM,SENSOR_ACCELEROMETER,
				SENSOR_GRAVITY, SENSOR_GYROSCOPE, SENSOR_PRESSURE , SENSOR_HUMAN_SLEEP_DETECTOR ,
				SENSOR_HUMAN_SLEEP_MONITOR , SENSOR_HUMAN_STRESS_MONITOR};
sensor_listener_h listenerList [SENSORNUM];
sensor_h sensorList [SENSORNUM];
bool writeToFileFlag = false;


collectedData allcollectedData[arraySize];
char * getCurrentTimeString(){
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return asctime (timeinfo);
}

Eina_Bool exit_service(void * data);
void endAllSensor();
void
startCollectingData();

void sensorWriteToFile(){
	//Write the PPG values in the file
	char filename[256];
	sprintf(filename, "/opt/usr/media/Downloads/data_%s.csv",time_file);
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
}
unsigned long long getCurrentTimeStamp (){
	struct timespec spec;
	clock_gettime(CLOCK_REALTIME, &spec);
	unsigned long long ts = spec.tv_sec * 1000LL + spec.tv_nsec / 1000000LL;
	return ts;
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
void setPedometer(sensor_event_s *event){}
void hrm_check_callback(sensor_h sensor, sensor_event_s *event, void * data){
	check_count_HRM++;
	if (event->values[0] < 0){
		check_count_error_HRM++;
	}
	if (check_count_HRM == CHECKNUM){
		startCollectingData();

	}
}

void setPPG (sensor_event_s *event) {
	allcollectedData[ppgCounter].ppg = event->values[0];
	ppgCounter++;
	if (ppgCounter == 7200){
	 int error;
		error = device_power_release_lock(POWER_LOCK_CPU);
		error = device_power_request_lock(POWER_LOCK_CPU, 0);
   }
	else if (ppgCounter> MAXNUM){
		endAllSensor(NULL);

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
	case SENSOR_HUMAN_SLEEP_MONITOR:
		//setSleepMonitor(event);
		break;
	case SENSOR_HUMAN_SLEEP_DETECTOR:
		//setSleepDetector(event);
		break;
	case SENSOR_HUMAN_STRESS_MONITOR:
		setStress(event);
		break;
//			case SENSOR_HUMAN_PEDOMETER:
//				setPedometer(event);
//				break;
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
#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "ending sensor one by one");
#endif
	for (int i = 0; i < SENSORNUM ; i ++){
		endSensor(i);
	}
	exit_service(NULL);
}


void startSensor (sensor_event_cb callback , int i ){
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
	if (sensorType == SENSOR_HUMAN_SLEEP_MONITOR || sensorType == SENSOR_HUMAN_SLEEP_DETECTOR){
			sensor_listener_set_event_cb(listenerList [i], SLEEPINTERVAL, callback, NULL);
	}else{
			sensor_listener_set_event_cb(listenerList [i], SENSORINTERVAL, callback , NULL); //20Hz
	}
		sensor_listener_set_attribute_int(listenerList [i], SENSOR_ATTRIBUTE_PAUSE_POLICY, SENSOR_PAUSE_NONE);
		sensor_listener_set_option(listenerList [i], SENSOR_OPTION_ALWAYS_ON);
		sensor_listener_start(listenerList [i]);
}

void check_HRM(){
	bool supported_HRM = false;
	sensor_is_supported(SENSOR_HRM, &supported_HRM);
	dlog_print(DLOG_INFO, LOG_TAG, "inside hrm checking function");
	if (!supported_HRM) {
		//Record an Error if the sensor is not supported, else continue.
		FILE * fp = fopen(errorFileName,"a");
		fprintf (fp, "HRM is not available: %s  \n", getCurrentTimeString());
		fclose (fp);
	} else{
		//Set sensors and start recording
		int error ;
		sensor_h sensor;
		error = sensor_get_default_sensor(SENSOR_HRM, &sensor);
		sensor_listener_h listener;
		error  = sensor_create_listener(sensor, &listener);
		error = sensor_listener_set_event_cb(listener, 50, hrm_check_callback, NULL); //20Hz
		error = sensor_listener_set_attribute_int(listener, SENSOR_ATTRIBUTE_PAUSE_POLICY, SENSOR_PAUSE_NONE);
		error = sensor_listener_set_option(listener, SENSOR_OPTION_ALWAYS_ON);
		error = sensor_listener_start(listener);

	}
}

void setTimeForFileName(){ // set time in time_file variable
//The current time for the filename
	time_t raw_time;
	struct tm* time_info;
	time(&raw_time);
	time_info = localtime(&raw_time);
	sprintf(time_file,"%d%s%d%s%d%s%d%s%d",(time_info->tm_year+1900), time_info->tm_mon+1<10? "0" : "", time_info->tm_mon+1, time_info->tm_mday<10? "0" : "", time_info->tm_mday, time_info->tm_hour<10? "0" : "", time_info->tm_hour, time_info->tm_min<10? "0" : "", time_info->tm_min);

}
void
startCollectingData(){
	//Check if at least half of the HRM values are not negative, else exit.

#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "check_count_error_HRM %d " , check_count_error_HRM );
	dlog_print(DLOG_INFO, LOG_TAG, "check_count_HRM %d " , check_count_HRM );
#endif

	float testValue = (check_count_error_HRM + 0.0 ) /check_count_HRM;

	if ( testValue < 0.5){

		writeToFileFlag = true;
		for (int i  = 0 ; i < SENSORNUM ; i++){
			startSensor(all_sensor_callback, i);
		}

	}
	else {
		 exit_service(NULL);
	}
#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "Start Collection ");
#endif
}


void appTerminate (){
	//Terminate the app
#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "terminate!");
#endif
	//Write everything in the file
	if (writeToFileFlag == true){
			#ifdef DEBUG
				dlog_print(DLOG_INFO, LOG_TAG, "writing to file!");
			#endif
		sensorWriteToFile();
	}

	device_power_release_lock(POWER_LOCK_CPU);
    return;
}

Eina_Bool exit_service(void * data){
#ifdef DEBUG
	dlog_print(DLOG_INFO, LOG_TAG, "exit+service!");
#endif
	appTerminate();
	service_app_exit();
	return true;
}

bool appCreate(){
	setTimeForFileName();//set time in time_file variable

	int error;
	error = device_power_request_lock(POWER_LOCK_CPU, 0);
	dlog_print(DLOG_INFO, LOG_TAG, "Initialized!");
	check_HRM(); //Check if the HRM is available (if the wristband is on the hand).
	return true;
}

bool service_app_create(void *data)
{
	#ifdef DEBUG
		FILE * fp = fopen(logFileName,"a");
	    fprintf(fp , "%s \t app create function \n", getCurrentTimeString());
	    fclose(fp);
	#endif
	return appCreate();
}

void service_app_terminate(void *data)
{
	#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app terminate function \n", getCurrentTimeString());
	fclose(fp);
	dlog_print(DLOG_INFO, LOG_TAG, "service app terminated function");
	#endif

}

void service_app_control(app_control_h app_control, void *data)
{
	controlCounter ++;
	#ifdef DEBUG
		FILE * fp = fopen(logFileName,"a");
		fprintf(fp , "%s \t app control function counter = %d \n", getCurrentTimeString(), controlCounter);
		fclose(fp);
		dlog_print(DLOG_INFO, LOG_TAG, "app control function");
	#endif
	if( controlCounter >1){
		exit_service(NULL);
	}
    return;
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
