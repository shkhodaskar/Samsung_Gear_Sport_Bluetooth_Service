#include <tizen.h>
#include <service_app.h>
#include <sensor.h>
#include<stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include "recorderservice.h"

#ifdef DEBUG
char logFileName [256] = "/opt/usr/media/Images/recorderService.txt";
#endif
const char recorderFilename[256]= "/opt/usr/media/Downloads/recorderData_%s.csv";
char time_file[256];


int recording_sensors[4]={SENSOR_HUMAN_PEDOMETER,SENSOR_PRESSURE,SENSOR_HUMAN_SLEEP_MONITOR,SENSOR_HRM};
#define recording_sensor_len 3  // to remove hrm

char * getCurrentTimeString(){
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return asctime (timeinfo);
}

void setTimeForFileName(){ // set time in time_file variable
	time_t raw_time;
	struct tm* time_info;
	time(&raw_time);
	time_info = localtime(&raw_time);
	sprintf(time_file,"%d%s%d%s%d",(time_info->tm_year+1900), time_info->tm_mon+1<10? "0" : "", time_info->tm_mon+1, time_info->tm_mday<10? "0" : "", time_info->tm_mday);

}

bool pedo_sensor_recorder_callback(sensor_type_e type,
		sensor_recorder_data_h data,int remains, sensor_error_e error,
		void *user_data) {
	int step, walkStep, runStep;
	double distance, cal;
	time_t start;
	time_t end;
	char filename [256];
	sprintf(filename, recorderFilename, time_file);
	FILE * fp = fopen(filename,"a");



	if (error != SENSOR_ERROR_NONE) {
		#ifdef DEBUG
		dlog_print(DLOG_DEBUG, LOG_TAG, "Pedo getting data ERR = %d", error);
		fprintf(fp, "Pedo getting data ERR = %d", error);
		#endif
		fclose(fp);
		return true;
	}

	sensor_recorder_data_get_time(data, &start, &end);
	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_STEPS, &step);
	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_WALK_STEPS,
			&walkStep);
	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_RUN_STEPS,
			&runStep);

	sensor_recorder_data_get_double(data, SENSOR_RECORDER_DATA_DISTANCE,
			&distance);
	sensor_recorder_data_get_double(data, SENSOR_RECORDER_DATA_CALORIE, &cal);

	 fprintf (fp , "START_TIME=%ld\tSTOP_TIME=%ld\tSTEP= %d\tDISTANCE= %.2f\tWALKSTEPS=%d\t"
		                "RunSteps=%d\tcalorie=%.2f\tremains %d\n",start,end, step,distance, walkStep, runStep,cal , remains);
	 fclose(fp);
	 return true;
}

bool pressure_sensor_recorder_callback(sensor_type_e type, sensor_recorder_data_h data, int remains,
		sensor_error_e error, void *user_data) {
	double pressure, avgPressure, minPressure, maxPressure;
	time_t start;
	time_t end;
	char filename [256];
	sprintf(filename, recorderFilename, time_file);
	FILE * fp = fopen(filename,"a");

	if (error != SENSOR_ERROR_NONE) {
		#ifdef DEBUG
		dlog_print(DLOG_DEBUG, LOG_TAG, "Pressure getting data ERR = %d", error);
		fprintf (fp , "Pressure getting data ERR = %d\n", error);
		#endif
		fclose(fp);
		return true;
	}

	sensor_recorder_data_get_time(data, &start, &end);
	sensor_recorder_data_get_double(data, SENSOR_RECORDER_DATA_PRESSURE,
			&pressure);
	sensor_recorder_data_get_double(data, SENSOR_RECORDER_DATA_AVERAGE_PRESSURE,
			&avgPressure);
	sensor_recorder_data_get_double(data, SENSOR_RECORDER_DATA_MIN_PRESSURE,
			&minPressure);
	sensor_recorder_data_get_double(data, SENSOR_RECORDER_DATA_MAX_PRESSURE,
			&maxPressure);

	fprintf(fp , "START_TIME=%ld\tSTOP_TIME=%ld\tPressure= %.2f\tavgPressure=%.2f\tminPressure= %.2f\tmaxPressure=%.2f\tremains %d \n",
			start,end,pressure, avgPressure, minPressure, maxPressure , remains);
	 fclose(fp);
	 return true;

}

bool sleep_sensor_recorder_callback(sensor_type_e type,
		sensor_recorder_data_h data, int remains, sensor_error_e error,
		void *user_data) {
	int sleepState;
	time_t start;
	time_t end;
	char filename [256];
		sprintf(filename, recorderFilename, time_file);
		FILE * fp = fopen(filename,"a");

	if (error != SENSOR_ERROR_NONE) {
		fprintf(fp , "Sleep getting data ERR = %d\n", error);
		 fclose(fp);
		return true;
	}

	sensor_recorder_data_get_time(data, &start, &end);
	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_SLEEP_STATE,
			&sleepState);

	fprintf(fp , "START_TIME=%ld\tSTOP_TIME=%ld\tSLEEP_STATE= %d\tremains %d\n", start,end,sleepState, remains);
	 fclose(fp);
	 return true;

}

bool hrm_sensor_recorder_callback(sensor_type_e type,
		sensor_recorder_data_h data, int remains, sensor_error_e error,
		void *user_data) {
	int hr;

	time_t start;
	time_t end;
	char filename [256];
	sprintf(filename, recorderFilename, time_file);
	FILE * fp = fopen(filename,"a");

	if (error != SENSOR_ERROR_NONE) {
		#ifdef DEBUG
		dlog_print(DLOG_DEBUG, LOG_TAG, "HRM getting data ERR = %d\n", error);
		#endif
		fclose(fp);
		return true;
	}

	sensor_recorder_data_get_time(data, &start, &end);
	sensor_recorder_data_get_int(data, SENSOR_RECORDER_DATA_HEART_RATE, &hr);

	fprintf(fp , "START_TIME=%ld\tSTOP_TIME=%ld\tHEART_RATE= %d\tremains %d\n", start,end,hr, remains);
	 fclose(fp);
	 return true;
}

void query_builder(){
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t INSIDE query builder function\n", getCurrentTimeString());
	fclose(fp);
	dlog_print(DLOG_DEBUG, LOG_TAG, "INSIDE query builder function");
#endif

	sensor_recorder_query_h query;
	sensor_recorder_create_query(&query);
	/* Start time by second */
	sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_START_TIME,
	                               (time_t)(time(NULL) - (12 * 60 * 60)));
	/* End now */
	sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_END_TIME, time(NULL));
	/* Aggregate every 10 minutes (not possible less than 10 min)*/
	sensor_recorder_query_set_int(query, SENSOR_RECORDER_QUERY_TIME_INTERVAL, 10);
	/* Start the aggregation at 7 AM */
	//sensor_recorder_query_set_time(query, SENSOR_RECORDER_QUERY_ANCHOR_TIME, (time_t)(7 * 3600));

//	sensor_recorder_read(recording_sensors[3], query, hrm_sensor_recorder_callback, NULL);
	sensor_recorder_read(recording_sensors[0], query, pedo_sensor_recorder_callback, NULL);
	sensor_recorder_read(recording_sensors[1], query, pressure_sensor_recorder_callback, NULL);
	sensor_recorder_read(recording_sensors[2], query, sleep_sensor_recorder_callback, NULL);

}
void start_recorder() {
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t INSIDE start recorder\n", getCurrentTimeString());
	dlog_print(DLOG_DEBUG, LOG_TAG, "INSIDE start recorder");
#endif

	bool supported;
	sensor_recorder_option_h option;
	sensor_recorder_create_option(&option);
	/* Recorder Period in Hours */
	sensor_recorder_option_set_int(option, SENSOR_RECORDER_OPTION_RETENTION_PERIOD, 12 *1*1);

	for (int i = 0; i <  recording_sensor_len; i++) {
		supported = false;
		sensor_recorder_is_supported(recording_sensors[i], &supported);
		if (!supported) {
			#ifdef DEBUG
			dlog_print(DLOG_DEBUG, LOG_TAG,
					"recorder is not supported for sensor No = %d",
					recording_sensors[i]);
			fprintf(fp , "%s \t recorder is not supported for sensor No = %d\n", getCurrentTimeString(),recording_sensors[i]);
			#endif
		} else {
			sensor_recorder_start(recording_sensors[i], option);
		}
	}
#ifdef DEBUG
	fclose(fp);
#endif
}


void stop_recorder(){
	for (int i = 0; i <  recording_sensor_len; i++) {
		sensor_recorder_stop(recording_sensors[i]);
	}

}

bool service_app_create(void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app create function \n", getCurrentTimeString());
	fclose(fp);
#endif

	setTimeForFileName();
	start_recorder();
	return true;
}

void service_app_terminate(void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app terminate function\n", getCurrentTimeString());
	fclose(fp);
#endif

	query_builder();
	stop_recorder();
    return;
}

void service_app_control(app_control_h app_control, void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t app control function \n", getCurrentTimeString());
	fclose(fp);
#endif
	setTimeForFileName();
	query_builder();
	stop_recorder();
	start_recorder();
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
