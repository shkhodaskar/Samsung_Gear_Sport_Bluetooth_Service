#include <tizen.h>
#include <service_app.h>
#include "myppgservice.h"
#include <time.h>
#include <app.h>
#include <app_alarm.h>
#include <stdio.h>

#ifdef Debug
char logFileName [256] = "/opt/usr/media/Images/myppgServiceLog.txt";
#endif

char * getCurrentTimeString(){
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	return asctime (timeinfo);
}


bool recorderAlarm(){
	int ret;
	int DELAY = 1; //seconds
	int REMIND = 60* 60* 12; //seconds
	int alarm_id;
	app_control_h app_control = NULL;

		ret = app_control_create(&app_control);
	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
	ret = app_control_set_app_id(app_control, "org.example.recorderservice");
	ret = alarm_schedule_after_delay(app_control, DELAY, REMIND, &alarm_id);
	dlog_print(DLOG_INFO, LOG_TAG, "Alarm is set!"); //Alarm is set!
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t alarm is set for org.example.recorderservice \n", getCurrentTimeString());
	fclose(fp);
#endif
	return true;
}

bool test_sleep(){
	int ret;
	int DELAY = 12; //seconds
	int REMIND = 60* 60* 4; //seconds
	int alarm_id;
	app_control_h app_control = NULL;

		ret = app_control_create(&app_control);
	ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
	ret = app_control_set_app_id(app_control, "org.example.sleepsensorservice");
	ret = alarm_schedule_after_delay(app_control, DELAY, REMIND, &alarm_id);
	dlog_print(DLOG_INFO, LOG_TAG, "Alarm is set!"); //Alarm is set!
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t alarm is set for sleepSensorService \n", getCurrentTimeString());
	fclose(fp);
#endif

	return true;
}

bool
set_alarm()
{
	//Set the alarm
    int ret;
    int DELAY = 10; //seconds
    int REMIND = 60 * 60*2; //seconds
    int alarm_id;
    app_control_h app_control = NULL;

        ret = app_control_create(&app_control);
    ret = app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
    ret = app_control_set_app_id(app_control, "org.example.hrvcollection");
    ret = alarm_schedule_after_delay(app_control, DELAY, REMIND, &alarm_id);
    dlog_print(DLOG_INFO, LOG_TAG, "Alarm is set!"); //Alarm is set!
#ifdef DEBUG
    FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s \t alarm is set for myppg \n", getCurrentTimeString());
	fclose(fp);
#endif
    return true;
 }

void my_service()
{
	set_alarm();
	//service_app_exit(); //Uncomment to terminate the service.
}

bool service_app_create(void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s appCreate Function\n", getCurrentTimeString());
	fclose(fp);
#endif
//	my_service();
//	recorderAlarm();
//	test_sleep();

    return true;
}

void service_app_terminate(void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s  terminate  Function\n", getCurrentTimeString());
	fclose(fp);
#endif

    return;
}

void service_app_control(app_control_h app_control, void *data)
{
#ifdef DEBUG
	FILE * fp = fopen(logFileName,"a");
	fprintf(fp , "%s  control Function\n", getCurrentTimeString());
	fclose(fp);
#endif
	alarm_cancel_all();

	my_service();
	recorderAlarm();
	test_sleep();

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
