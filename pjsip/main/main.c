/* $Id$ */
/* 
 * Copyright (C) 2008-2011 Teluu Inc. (http://www.teluu.com)
 * Copyright (C) 2003-2008 Benny Prijono <benny@prijono.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 */

/**
 * simple_pjsua.c
 *
 * This is a very simple but fully featured SIP user agent, with the 
 * following capabilities:
 *  - SIP registration
 *  - Making and receiving call
 *  - Audio/media to sound device.
 *
 * Usage:
 *  - To make outgoing call, start simple_pjsua with the URL of remote
 *    destination to contact.
 *    E.g.:
 *	 simpleua sip:user@remote
 *
 *  - Incoming calls will automatically be answered with 200.
 *
 * This program will quit once it has completed a single call.
 */

#include <pjsua-lib/pjsua.h>
#include <pthread.h>
#include <tcpip_adapter.h>
#include <mdns.h>
#include "nvs_flash.h"
#include "esp_peripherals.h"
#include "periph_wifi.h"
#include "board.h"
#include "esp_log.h"
#include "console.h"

#define TAG	        "VoIPDemo"

#define WIFI_SSID   "xxx"            // wifi ssid
#define WIFI_PWD    "xxx"            // wifi password

#define SIP_DOMAIN	"xx.xx.xx.xx"    // sip server ip address
#define SIP_PASSWD	"xxx"            // sip regiser password
#define SIP_USER	"xxx"            // sip account name
#define SIP_CALLEE  "xxx"            // callee account name
#define SIP_PORT    "5070"           // sip port
#define SIP_TRANSPORT_TYPE PJSIP_TRANSPORT_TCP   // default sip transport type 

/* Display error and exit application */
static void error_exit(const char *title, pj_status_t status)
{
    pjsua_perror(TAG, title, status);
}

/*
 * Handler registration status has changed.
 */
static void on_reg_state2(pjsua_acc_id acc_id, pjsua_reg_info *reg_info){
    pjsua_acc_info acc_info;

    pjsua_acc_get_info(acc_id, &acc_info);

    if (reg_info->renew != 0) {
        if (reg_info->cbparam->code == 200) {
            printf("on_reg_state2 account:%s logined ok.\n", acc_info.acc_uri.ptr);

            /* If argument is specified, it's got to be a valid SIP URL */
            char url[50] = {0};
            pj_status_t status;

            if (SIP_TRANSPORT_TYPE == PJSIP_TRANSPORT_TCP)
                pj_ansi_sprintf(url, "sip:%s@%s:%s;transport=tcp", SIP_CALLEE,
                                SIP_DOMAIN, SIP_PORT);
            else
                pj_ansi_sprintf(url, "sip:%s@%s:%s", SIP_CALLEE, SIP_DOMAIN, SIP_PORT);
            
            status = pjsua_verify_url(url);
            if (status != PJ_SUCCESS)
            {
                error_exit("Invalid URL", status);
                return;
            }

            /* If URL is specified, make call to the URL. */
            if (url[0] != '\0') {
                pj_str_t uri = pj_str(url);
                status = pjsua_call_make_call(acc_id, &uri, 0, NULL, NULL, NULL);
                if (status != PJ_SUCCESS)
                {
                    error_exit("Error making call", status);
                    return;
                }
            }
        }
        else{
            printf("on_reg_state2 account:%s logined failed error code %d.\n",
                            acc_info.acc_uri.ptr, reg_info->cbparam->code);
        }
    }
    else{
        if (reg_info->cbparam->code == 200)
        {
            printf("on_reg_state2 account:%s logout ok.\n", acc_info.acc_uri.ptr);
        }
    }
}

/* Callback called by the library upon receiving incoming call */
static void on_incoming_call(pjsua_acc_id acc_id, pjsua_call_id call_id,
			     pjsip_rx_data *rdata)
{
    pjsua_call_info ci;

    PJ_UNUSED_ARG(acc_id);
    PJ_UNUSED_ARG(rdata);

    pjsua_call_get_info(call_id, &ci);

    PJ_LOG(3,(TAG, "Incoming call from %.*s!!",
			 (int)ci.remote_info.slen,
			 ci.remote_info.ptr));

    /* Automatically answer incoming calls with 200/OK */
    pjsua_call_answer(call_id, 200, NULL, NULL);
}

/* Callback called by the library when call's state has changed */
static void on_call_state(pjsua_call_id call_id, pjsip_event *e)
{
    pjsua_call_info ci;

    PJ_UNUSED_ARG(e);

    pjsua_call_get_info(call_id, &ci);
    PJ_LOG(3,(TAG, "Call %d state=%.*s", call_id,
			 (int)ci.state_text.slen,
			 ci.state_text.ptr));
}

/* Callback called by the library when call's media state has changed */
static void on_call_media_state(pjsua_call_id call_id)
{
    pjsua_call_info ci;

    pjsua_call_get_info(call_id, &ci);

    if (ci.media_status == PJSUA_CALL_MEDIA_ACTIVE) {
	    // When media is active, connect call to sound device.
        pjsua_conf_connect(ci.conf_slot, 0);
        pjsua_conf_connect(0, ci.conf_slot);
    }
}

/* sip main thread */
void * sip_main(void *param)
{
    pjsua_acc_id acc_id;
    pj_status_t status;
    
     printf("sip_main 1\n");
    /* Create pjsua first! */
    status = pjsua_create();
    if (status != PJ_SUCCESS)
    {
        error_exit("Error in pjsua_create()", status);
        return NULL;
    }

    /* Init pjsua */
    {
        pjsua_config cfg;
        pjsua_logging_config log_cfg;

        pjsua_config_default(&cfg);
        cfg.cb.on_reg_state2 = &on_reg_state2;    //register 
        cfg.cb.on_incoming_call = &on_incoming_call;
        cfg.cb.on_call_media_state = &on_call_media_state;
        cfg.cb.on_call_state = &on_call_state;

        pjsua_logging_config_default(&log_cfg);
        log_cfg.console_level = 6;

        status = pjsua_init(&cfg, &log_cfg, NULL);
        if (status != PJ_SUCCESS)
        {
            error_exit("Error in pjsua_init()", status);
            return NULL;
        }
        
    }

    /* Add UDP/tcp transport. */
    {
        pjsua_transport_config cfg;
            pj_sockaddr addr = {0};

        pj_gethostip(pj_AF_INET(), &addr);

        printf("sip_main local ip:%s\n",  pj_addr_string(&addr));
        pjsua_transport_config_default(&cfg);

        cfg.port = (unsigned int)atoi(SIP_PORT);
        status = pjsua_transport_create(SIP_TRANSPORT_TYPE, &cfg, NULL);
        if (status != PJ_SUCCESS){
            error_exit("Error creating transport", status);
            return NULL;
        }
    }

    /* Initialization is done, now start pjsua */
    status = pjsua_start();
    if (status != PJ_SUCCESS)
    {
        error_exit("Error starting pjsua", status);
        return NULL;
    }

    /* Register to SIP server by creating SIP account. */
    {
        pjsua_acc_config cfg;

        pjsua_acc_config_default(&cfg);
        cfg.id = pj_str("sip:" SIP_USER "@" SIP_DOMAIN);
        if (SIP_TRANSPORT_TYPE == PJSIP_TRANSPORT_TCP)
            cfg.reg_uri = pj_str("sip:" SIP_DOMAIN ":" SIP_PORT ";transport=tcp");
        else
            cfg.reg_uri = pj_str("sip:" SIP_DOMAIN ":" SIP_PORT);

        cfg.cred_count = 1;
        cfg.cred_info[0].realm = pj_str(SIP_DOMAIN);
        cfg.cred_info[0].scheme = pj_str("digest");
        cfg.cred_info[0].username = pj_str(SIP_USER);
        cfg.cred_info[0].data_type = PJSIP_CRED_DATA_PLAIN_PASSWD;
        cfg.cred_info[0].data = pj_str(SIP_PASSWD);

        status = pjsua_acc_add(&cfg, PJ_TRUE, &acc_id);
        if (status != PJ_SUCCESS)
        {
            error_exit("Error adding account", status);
            return NULL;
        }
    }

    for (;;)
    {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    return NULL;
}

static int sip_call_cmd(int argc, char **argv)
{
    printf("sip_call_cmd argv1=%s", argv[1]);
    pjsua_call_hangup_all();
    
    return 0;
}

/*
 * app_main()
 */
void app_main()
{
    pthread_attr_t thread_attr;
    pthread_t sip_main_thread;
    int rc;

	/* tcp/ip init */
	esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

	tcpip_adapter_init();

    ESP_LOGI(TAG, "[1.0] Initialize peripherals management");
    esp_periph_config_t periph_cfg = DEFAULT_ESP_PERIPH_SET_CONFIG();
    esp_periph_set_handle_t set = esp_periph_set_init(&periph_cfg);

    ESP_LOGI(TAG, "[1.1] Initialize and start peripherals");
    audio_board_key_init(set);

    ESP_LOGI(TAG, "[1.2] Start and wait for Wi-Fi network");
    periph_wifi_cfg_t wifi_cfg = {
        .ssid = WIFI_SSID,
        .password = WIFI_PWD,
    };

    esp_periph_handle_t wifi_handle = periph_wifi_init(&wifi_cfg);
    esp_periph_start(set, wifi_handle);
    periph_wifi_wait_for_connected(wifi_handle, portMAX_DELAY);

    ESP_LOGI(TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_BOTH, AUDIO_HAL_CTRL_START);

    console_init();

    const esp_console_cmd_t cmd_sip_call = {
        .command = "call",
        .help = "Get the current size of free heap memory",
        .hint = NULL,
        .func = &sip_call_cmd,
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd_sip_call));

    printf("app_main 0\n");
    /* Init thread attributes */
    pthread_attr_init(&thread_attr);
    thread_attr.stacksize = 50 * 1024;
    rc = pthread_create_static(&sip_main_thread, &thread_attr, sip_main, &sip_main_thread,
         "sip_main", 10, 1);

    if (rc != 0) {
        printf("voip call create thread failed\n");
        abort();
    }
}
