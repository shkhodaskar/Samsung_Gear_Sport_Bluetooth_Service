#include "app_main.h"
#include "uib_app_manager.h"

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <minizip/zip.h>
#include <wifi.h>
#include <wifi-manager.h>

#include <openssl/md5.h>
#include <http.h>
#include <bluetooth.h>
#include <dlog.h>
#include <app_control.h>
#include <glib.h>

//Initialize Bluetooth API before app starts
int bluetoothInitialize() {
	bt_error_e ret;
	ret = bt_initialize();

	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt_initialize] failed.\n");
		return 0;
	}
	return 1;
}

//Deinitialize Bluetooth API once app is closed
int bluetoothDeinitialize() {
	bt_error_e ret;
	ret = bt_deinitialize();

	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt_deinitialize] failed.\n");
		return 0;
	}
	return 1;
}

//Display Bluetooth settings so the user can toggle the Bluetooth state
//Needed so user can switch Bluetooth on
int bluetoothToggle(void) {
	int ret = 0;
	app_control_h service = NULL;
	app_control_create(&service);

	if (!service) {
		dlog_print(DLOG_INFO, LOG_TAG, "service_create failed!\n");
		return 0;
	}
	app_control_set_operation(service, APP_CONTROL_OPERATION_SETTING_BT_ENABLE);
	ret = app_control_send_launch_request(service, NULL, NULL);

	app_control_destroy(service);
	if (ret == APP_CONTROL_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "Launched bluetooth setting toggle.\n");
		return 1;
	}
	else {
		dlog_print(DLOG_INFO, LOG_TAG, "Unable to launch bluetooth toggle.\n");
		return 0;
	}
	return 0;
}

//Checks Bluetooth adapter state
//To communicate with other devices, the Bluetooth adapter should be enabled
int checkAdapterState(void) {
	bt_adapter_state_e adapter_state;
	bt_error_e ret;
	ret = bt_adapter_get_state(&adapter_state);

	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt_adapte_get_state] failed");
		return 0;
	}

	if (adapter_state == BT_ADAPTER_DISABLED) {
		dlog_print(DLOG_ERROR, LOG_TAG, "Enable Bluetooth!\n");
		return 0;
	}

	return 1;

}

//Define a Bluetooth adapter state callback function to monitor changes in Bluetooth adapter state

void adapter_device_discovery_state_changed_cb(int result, bt_adapter_device_discovery_state_e discovery_state,
                                          bt_adapter_device_discovery_info_s *discovery_info, void* user_data)
{
    if (result != BT_ERROR_NONE) {
        dlog_print(DLOG_ERROR, LOG_TAG, "[adapter_device_discovery_state_changed_cb] failed! result(%d).", result);

        return;
    }

    GList** searched_device_list = (GList**)user_data;
    switch (discovery_state) {
    case BT_ADAPTER_DEVICE_DISCOVERY_STARTED:
        dlog_print(DLOG_INFO, LOG_TAG, "BT_ADAPTER_DEVICE_DISCOVERY_STARTED");
        break;
    case BT_ADAPTER_DEVICE_DISCOVERY_FINISHED:
        dlog_print(DLOG_INFO, LOG_TAG, "BT_ADAPTER_DEVICE_DISCOVERY_FINISHED");
        break;
    case BT_ADAPTER_DEVICE_DISCOVERY_FOUND:
        dlog_print(DLOG_INFO, LOG_TAG, "BT_ADAPTER_DEVICE_DISCOVERY_FOUND");
        if (discovery_info != NULL) {
            dlog_print(DLOG_INFO, LOG_TAG, "Device Address: %s", discovery_info->remote_address);
            dlog_print(DLOG_INFO, LOG_TAG, "Device Name is: %s", discovery_info->remote_name);
            bt_adapter_device_discovery_info_s * new_device_info = malloc(sizeof(bt_adapter_device_discovery_info_s));

            //discovery_info
            if (new_device_info != NULL) {
                memcpy(new_device_info, discovery_info, sizeof(bt_adapter_device_discovery_info_s));
                new_device_info->remote_address = strdup(discovery_info->remote_address);
                new_device_info->remote_name = strdup(discovery_info->remote_name);
                *searched_device_list = g_list_append(*searched_device_list, (gpointer)new_device_info);
            }
        }
        break;
    }
}

//Begin the discovery process
int startDiscovery(void) {
	bt_error_e ret;
	ret = bt_adapter_start_device_discovery();

	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt_adapter_start_device_discovery] failed");
		return 0;
	}
	return 1;
}

//end the discovery process
int endDiscovery(void) {
	bt_error_e ret;
	ret = bt_adapter_stop_device_discovery();

	if (ret != BT_ERROR_NONE) {
		dlog_print(DLOG_ERROR, LOG_TAG, "[bt_adapter_stop_device_discovery] failed");
		return 0;
	}
	return 1;
}

int bondWithDevice() {
	GList *devices_list = NULL;
	bt_error_e ret;
	ret = bt_adapter_set_device_discovery_state_changed_cb(adapter_device_discovery_state_changed_cb,
	                                                       (void*)&devices_list);

	if (ret != BT_ERROR_NONE)
	    dlog_print(DLOG_ERROR, LOG_TAG, "[bt_adapter_set_device_discovery_state_changed_cb] failed.");

	//Loop through devices_list and select the correct device you want to pair with:
	Glist* curr = devices_list;
	while (curr != NULL) {

	}
	bt_adapter_device_discovery_info_s* bt_device = NULL;

	ret = bt_device_create_bond(bt_device->remote_address);
	if (ret != BT_ERROR_NONE) {
	    dlog_print(DLOG_ERROR, LOG_TAG, "[bt_device_create_bond] failed.");
	    return 0;

	}
	else {
	    dlog_print(DLOG_INFO, LOG_TAG, "[bt_device_create_bond] succeeded. device_bond_created_cb callback will be called.");
	}
}
























typedef struct EraseDataStruct {
	char ** names;
	int fileNum;
} EraseData;
char dataPath[256] = "/opt/usr/media/Downloads/";
char zipPath[256] = "/opt/usr/media/Images/mdx.zip";
char serverUrl[256] = "https://unite.healthscitech.org/api/sensing/samsung_raw/upload_zip";
char deviceId[256] = "unite_usahil";
char devicePass[256] = "sahilsahilpassword";
char curlOutput[400];
char mdString[256];
bool show = false;

void initConfig(){

}

char *my_md5(const char* md5_str) {
	unsigned char digest[16];
	char* string = malloc(1 + strlen(md5_str));

	if (string)
		strcpy(string, md5_str);

	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, string, strlen(string));
	MD5_Final(digest, &ctx);

	for (int i = 0; i < 16; i++)
		sprintf(&mdString[i * 2], "%02x", (unsigned int) digest[i]);

	return mdString;
}

wifi_connection_state_e checkWifiConnectionState() {
	wifi_connection_state_e state = WIFI_CONNECTION_STATE_FAILURE;
	wifi_error_e error = wifi_initialize();
	dlog_print(DLOG_DEBUG, LOG_TAG, "checking wifi error %d", error);
	if (error == WIFI_ERROR_NONE) {
		if (wifi_get_connection_state(&state) == WIFI_ERROR_NONE) {
			return state;
		} else {
			//LOGE("Cannot get WiFi connection state!");
			return state;

		}
	} else {
		//LOGE("WiFi initialization error!");
		return state;
	}
	dlog_print(DLOG_DEBUG, LOG_TAG, "checking wifi states %d", state);
	return state;
}

// This should be called when you don't need WiFi services anymore (e.g. on app shutdown).
bool wifiDeinitialize() {
	if (wifi_deinitialize() == WIFI_ERROR_NONE) {
		return true;
	}
	return false;
}

int fileZipper(char *localPath, char *zipFilePath, EraseData * sendFiles) {
	char localFilepath[256];
	struct dirent **namelist;
	int ret = 0;
	FILE *infile;
	char *buffer;
	long numbytes;
	int notfound = 0;
	int memoryerr = 0;
	bool error = false;

	int n;
	int i = 0;

	zip_fileinfo zfi;
	zipFile zf = zipOpen(zipFilePath, APPEND_STATUS_CREATE);

	n = scandir(localPath, &namelist, 0, alphasort);
	sendFiles->fileNum = 0;
	sendFiles->names = calloc(n, sizeof(char*));
	if (sendFiles->names == NULL) {
		dlog_print(DLOG_DEBUG, LOG_TAG,
				"could not get memory for send files names");
		//To DO: free memmory before return
		return -1;
	}
	if (n < 0) {
		//perror("scandir");
		return -1;
	} else {
		while (i < n ) {  // change it from -1
			//	dlog_print(DLOG_DEBUG, LOG_TAG, "files : %s", namelist[i]->d_name);
			if (namelist[i]->d_name[0] == '.') {
				//dlog_print(DLOG_DEBUG, LOG_TAG, "ignoring file : %s", namelist[i]->d_name);
				//free(namelist[i]);
				//i++;

			} else {
				sprintf(localFilepath, "%s/%s", localPath, namelist[i]->d_name);
				ret = zipOpenNewFileInZip(zf, localFilepath, &zfi,
				NULL, 0,
				NULL, 0, "my comment for this interior file",
				Z_DEFLATED,
				Z_DEFAULT_COMPRESSION);
				if (ret < 0) {
					dlog_print(DLOG_DEBUG, LOG_TAG,
							"error in opening zip files");
					error = true;
				}

				/* open an existing file for reading */
				infile = fopen(localFilepath, "r");

				/* quit if the file does not exist */
				if (infile == NULL) {
					dlog_print(DLOG_DEBUG, LOG_TAG, "error in opening files");
					notfound += 1;
					error = true;
				}

				/* Get the number of bytes */
				fseek(infile, 0L, SEEK_END);
				numbytes = ftell(infile);

				/* reset the file position indicator to
				 the beginning of the file */
				fseek(infile, 0L, SEEK_SET);
				//dlog_print(DLOG_DEBUG, LOG_TAG, "file is %d bytes", numbytes);

				/* grab sufficient memory for the
				 buffer to hold the text */
				if (numbytes > 0) {
					buffer = (char*) calloc(numbytes, sizeof(char));

					/* memory error */
					if (buffer == NULL) {
						dlog_print(DLOG_DEBUG, LOG_TAG, "buffer is null");
						memoryerr += 1;
						error = true;

					}

					/* copy all the text into the buffer */
					fread(buffer, sizeof(char), numbytes, infile);

					zipWriteInFileInZip(zf, buffer, numbytes);
					free(buffer);
				}
				fclose(infile);

				/* free the memory we used for the buffer */
				zipCloseFileInZip(zf);

				if (i<n-1 && namelist[i]->d_name[0]== namelist[i+1]->d_name[0]){
					sendFiles->names[sendFiles->fileNum] = (char*) calloc(256,
										sizeof(char));

					strcpy(sendFiles->names[sendFiles->fileNum],
							namelist[i]->d_name);
					//	dlog_print(DLOG_DEBUG, LOG_TAG, "files : %s", namelist[i]->d_name);
					dlog_print(DLOG_DEBUG, LOG_TAG, "send files : %s",
							sendFiles->names[sendFiles->fileNum]);
					sendFiles->fileNum++;
				}
			}
			free(namelist[i]);
			i++;
		}
		free(namelist);
	}
	zipClose(zf, "my comment for exterior file");
	if (error) {
		return -1;
	}
	return 0;

}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream) {
	//  dlog_print(DLOG_DEBUG, LOG_TAG,"in write callback function response = [%s]\n", ptr);
	size_t written;
	written = fwrite(ptr, size, nmemb, stream);
	return written;
}

int httpUpload(char *localFile, char * httpUrl) {
	CURL *curl;
	CURLcode res = -1;
	char output[400];
	int size;
	bool error = false;

	char request[256];
	FILE * fp;

	struct curl_httppost *formpost = NULL;
	struct curl_httppost *lastptr = NULL;

	struct curl_slist *headerlist = NULL;
	static const char buf[] = "Expect:";

	fp = fopen("/opt/usr/media/Images/result.txt", "wb");

	dlog_print(DLOG_DEBUG, LOG_TAG, "in HTTPUPLOAD ");

	curl_global_init(CURL_GLOBAL_ALL);

	/* Fill in the file upload field */
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "userfile",
			CURLFORM_FILE, localFile, CURLFORM_END);

	/* Fill in the filename field */
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "username",
			CURLFORM_COPYCONTENTS, deviceId, CURLFORM_END);

	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "password",
			CURLFORM_COPYCONTENTS, my_md5(devicePass), CURLFORM_END);

	/* Fill in the submit field too, even if this is rarely needed */
	curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, "submit",
			CURLFORM_COPYCONTENTS, "Send File", CURLFORM_END);
	dlog_print(DLOG_DEBUG, LOG_TAG, " curl form add ");

	curl = curl_easy_init();
	/* initialize custom header list (stating that Expect: 100-continue is not
	 wanted */
	headerlist = curl_slist_append(headerlist, buf);
	if (curl) {
		dlog_print(DLOG_DEBUG, LOG_TAG, " curl not Null ");

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);

		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);
		/* what URL that receives this POST */
		curl_easy_setopt(curl, CURLOPT_URL, serverUrl);

		curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

		/* setting a callback function to return the data */
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);

		/* passing the pointer to the response as the callback parameter */
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		/* Check for errors */
		if (res != CURLE_OK)
			dlog_print(DLOG_DEBUG, LOG_TAG, " curl result %s ",
					curl_easy_strerror(res));

		/* always cleanup */
		curl_easy_cleanup(curl);

		/* then cleanup the formpost chain */
		curl_formfree(formpost);
		/* free slist */
		curl_slist_free_all(headerlist);
	}
	dlog_print(DLOG_DEBUG, LOG_TAG, " curl result %d ", res);

	fclose(fp);
	fp = fopen("/opt/usr/media/Images/result.txt", "rb");
	if (fp == NULL) {
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	rewind(fp);
	fread(output, 1, size, fp);
	output[size] = 0;
	fclose(fp);
	strcpy(curlOutput, output);
	dlog_print(DLOG_DEBUG, LOG_TAG, "file content %d", size);
	dlog_print(DLOG_DEBUG, LOG_TAG, "file content %s", output);
	if (error) {
		return -1;
	}
	return 0;
}

int eraseAll(char * path, EraseData * sendedFiles) {

	int i = 0;
	char filepath[256];
	while (i < sendedFiles->fileNum) {
		//	dlog_print(DLOG_DEBUG, LOG_TAG, "remove %d file : %s", i , sendedFiles->names[i]);
		sprintf(filepath, "%s/%s", path, sendedFiles->names[i]);
		remove(filepath);

		free(sendedFiles->names[i]);
		i++;
	}
	free(sendedFiles->names);

	return 0;
}

static void thread_run_cb(void *data, Ecore_Thread *thread) {
	hideBTN(data, 0);
	hideBTN(data, 1);
	show = false;

	EraseData sendedFiles;
	int ret = 0;
	ecore_thread_feedback(thread, "Compressing");
	ret = fileZipper(dataPath, zipPath, &sendedFiles);
	if (ret < 0) {
		//dlog_print(DLOG_DEBUG, LOG_TAG, "error while zipping files");
		show = true;
		ecore_thread_feedback(thread, "zip Error");
		return;
	}

	ecore_thread_feedback(thread, "Uploading");
	ret = httpUpload(zipPath, "");
	if (ret < 0) {
		//dlog_print(DLOG_DEBUG, LOG_TAG, "error while Uploading files");
		show = true;
		ecore_thread_feedback(thread, "Upload Error");
		return;
	}

	dlog_print(DLOG_DEBUG, LOG_TAG, "curloutput %s", curlOutput);
	dlog_print(DLOG_DEBUG, LOG_TAG, "strcmp output %d",
			strcmp(curlOutput, "evDONE"));
	//	if (curlOutput[0]!= 'e')
	if (strcmp(curlOutput, "evDONE") != 0) {

		//	dlog_print(DLOG_DEBUG, LOG_TAG, "error while checking uploaded files");
		show = true;
		ecore_thread_feedback(thread, "Error");

		return;
	}

	ecore_thread_feedback(thread, "Erasing");
	dlog_print(DLOG_DEBUG, LOG_TAG, "files Number to Erase : %d ",
			sendedFiles.fileNum);
	ret = eraseAll(dataPath, &sendedFiles);
	if (ret < 0) {
		//	dlog_print(DLOG_DEBUG, LOG_TAG, "error while erasing files");
		show = true;
		ecore_thread_feedback(thread, "erase Error");
		return;
	}
	show = true;
	ecore_thread_feedback(thread, "Done");
}

Eina_Bool my_timer_cb(void *data) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "All Done ");

	appTERMINATE(data);
	return ECORE_CALLBACK_CANCEL;
}

static void thread_feedback_cb(void * data, Ecore_Thread *thread, void *msg) {
	// This function is in critical section

	if (show) {
		showBTN(data, 0);
		showBTN(data, 1);
	}
	char * txt = msg;
	showMSG(data, txt);
	if (txt[0] == 'D') {
		hideBTN(data, 0);
		ecore_timer_add(10, my_timer_cb, data);

	}
}

static void thread_end_cb(void *data, Ecore_Thread *thread) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "thread end!");
}

static void thread_cancel_cb(void *data, Ecore_Thread *thread) {
	dlog_print(DLOG_DEBUG, LOG_TAG, "thread cancel!");
}

void sendBTN(void *data) {
	if (checkWifiConnectionState() != WIFI_CONNECTION_STATE_CONNECTED) {
		showMSG(data, "WIFI ERROR");
	} else {
		Ecore_Thread *thread;
		dlog_print(DLOG_DEBUG, LOG_TAG, "adding new thread");
		thread = ecore_thread_feedback_run(thread_run_cb, thread_feedback_cb,
				thread_end_cb, thread_cancel_cb, data,
				EINA_FALSE);

	}
	wifiDeinitialize();
}

void wifiCheck(void *data) {
	if (checkWifiConnectionState() != WIFI_CONNECTION_STATE_CONNECTED) {
		hideBTN(data, 0);
		showMSG(data, "WIFI ERROR");
	} else {
		showBTN(data, 1);
		showMSG(data, "Press SEND");
	}
	wifiDeinitialize();

}

static void __wifi_manager_activated_cb(wifi_error_e result, void *user_data) {
	if (result == WIFI_ERROR_NONE) {
		dlog_print(DLOG_INFO, LOG_TAG, "Success to activate Wi-Fi device!");
		showBTN(user_data, 0);
		showMSG(user_data, "Press SEND");
	} else {
		showMSG(user_data, "Turn on WIFI");
	}
}

wifi_manager_h turnOnWifi(void * data) {
	int error_code;
	wifi_manager_h wifi;
	error_code = wifi_manager_initialize(&wifi);
	if (error_code != WIFI_ERROR_NONE)
		return NULL;
	error_code = wifi_manager_activate_with_wifi_picker_tested(wifi,
			__wifi_manager_activated_cb, data);
	return wifi;

}

/* app event callbacks */
static bool _on_create_cb(void *user_data);
static void _on_terminate_cb(void *user_data);
static void _on_app_control_cb(app_control_h app_control, void *user_data);
static void _on_resume_cb(void *user_data);
static void _on_pause_cb(void *user_data);
static void _on_low_memory_cb(app_event_info_h event_info, void *user_data);
static void _on_low_battery_cb(app_event_info_h event_info, void *user_data);
static void _on_device_orientation_cb(app_event_info_h event_info,
		void *user_data);
static void _on_language_changed_cb(app_event_info_h event_info,
		void *user_data);
static void _on_region_format_changed_cb(app_event_info_h event_info,
		void *user_data);

void nf_hw_back_cb(void* param, Evas_Object * evas_obj, void* event_info) {
	//TODO : user define code
	evas_obj = uib_views_get_instance()->get_window_obj()->app_naviframe;
	elm_naviframe_item_pop(evas_obj);
}

void win_del_request_cb(void *data, Evas_Object *obj, void *event_info) {
	ui_app_exit();
}

Eina_Bool nf_root_it_pop_cb(void* elm_win, Elm_Object_Item *it) {
	elm_win_lower(elm_win);
	return EINA_FALSE;
}

app_data *uib_app_create() {
	return calloc(1, sizeof(app_data));
}

void uib_app_destroy(app_data *user_data) {
	uib_app_manager_get_instance()->free_all_view_context();
	free(user_data);
}

int uib_app_run(app_data *user_data, int argc, char **argv) {
	ui_app_lifecycle_callback_s cbs = { .create = _on_create_cb, .terminate =
			_on_terminate_cb, .pause = _on_pause_cb, .resume = _on_resume_cb,
			.app_control = _on_app_control_cb, };

	app_event_handler_h handlers[5] = { NULL, };

	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_BATTERY],
			APP_EVENT_LOW_BATTERY, _on_low_battery_cb, user_data);
	ui_app_add_event_handler(&handlers[APP_EVENT_LOW_MEMORY],
			APP_EVENT_LOW_MEMORY, _on_low_memory_cb, user_data);
	ui_app_add_event_handler(&handlers[APP_EVENT_DEVICE_ORIENTATION_CHANGED],
			APP_EVENT_DEVICE_ORIENTATION_CHANGED, _on_device_orientation_cb,
			user_data);
	ui_app_add_event_handler(&handlers[APP_EVENT_LANGUAGE_CHANGED],
			APP_EVENT_LANGUAGE_CHANGED, _on_language_changed_cb, user_data);
	ui_app_add_event_handler(&handlers[APP_EVENT_REGION_FORMAT_CHANGED],
			APP_EVENT_REGION_FORMAT_CHANGED, _on_region_format_changed_cb,
			user_data);

	return ui_app_main(argc, argv, &cbs, user_data);
}

void app_get_resource(const char *res_file_in, char *res_path_out,
		int res_path_max) {
	char *res_path = app_get_resource_path();
	if (res_path) {
		snprintf(res_path_out, res_path_max, "%s%s", res_path, res_file_in);
		free(res_path);
	}
}

static bool _on_create_cb(void *user_data) {
	/*
	 * This area will be auto-generated when you add or delete user_view
	 * Please do not hand edit this area. The code may be lost.
	 */
	uib_app_manager_st* app_manager = uib_app_manager_get_instance();

	app_manager->initialize();
	/*
	 * End of area
	 */
	return true;
}

static void _on_terminate_cb(void *user_data) {
	uib_views_get_instance()->destroy_window_obj();
}

static void _on_resume_cb(void *user_data) {
	/* Take necessary actions when application becomes visible. */
}

static void _on_pause_cb(void *user_data) {
	/* Take necessary actions when application becomes invisible. */
}

static void _on_app_control_cb(app_control_h app_control, void *user_data) {
	/* Handle the launch request. */
}

static void _on_low_memory_cb(app_event_info_h event_info, void *user_data) {
	/* Take necessary actions when the system runs low on memory. */
}

static void _on_low_battery_cb(app_event_info_h event_info, void *user_data) {
	/* Take necessary actions when the battery is low. */
}

static void _on_device_orientation_cb(app_event_info_h event_info,
		void *user_data) {
	/* deprecated APIs */
}

static void _on_language_changed_cb(app_event_info_h event_info,
		void *user_data) {
	/* Take necessary actions is called when language setting changes. */
	uib_views_get_instance()->uib_views_current_view_redraw();
}

static void _on_region_format_changed_cb(app_event_info_h event_info,
		void *user_data) {
	/* Take necessary actions when region format setting changes. */
}

