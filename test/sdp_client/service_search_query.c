
// *****************************************************************************
//
// test rfcomm query tests
//
// *****************************************************************************

#include "btstack_config.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btstack_event.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "classic/sdp_parser.h"
#include "hci.h"
#include "hci_cmd.h"
#include "hci_dump.h"
#include "l2cap.h"

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"


static uint8_t  sdp_test_record_list[] = { 
0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x01, 
0x00, 0x00, 0x00, 0x02, 
0x00, 0x00, 0x00, 0x03, 
0x00, 0x00, 0x00, 0x04, 
0x00, 0x00, 0x00, 0x05, 
0x00, 0x00, 0x00, 0x06, 
0x00, 0x00, 0x00, 0x07, 
0x00, 0x00, 0x00, 0x08, 
0x00, 0x00, 0x00, 0x09, 
0x00, 0x00, 0x00, 0x0A
};


static void handle_sdp_parser_event(sdp_query_event_t * event){
    const uint8_t * ve;
    const uint8_t * ce;

    static uint32_t record_handle = sdp_test_record_list[0];

    switch (event->type){
        case SDP_QUERY_SERVICE_RECORD_HANDLE:
            ve = (const uint8_t*) event;
            
            CHECK_EQUAL(sdp_query_service_record_handle_event_get_record_handle(ve), record_handle);
            record_handle++;

            
            break;
        case SDP_QUERY_COMPLETE:
            ce = (const uint8_t *) event;
            printf("General query done with status %d.\n", sdp_query_complete_event_get_status(ce));
            break;
    }
}


TEST_GROUP(SDPClient){
    void setup(void){
        sdp_parser_init_service_search();
        sdp_parser_register_callback(handle_sdp_parser_event);
    }
};


TEST(SDPClient, QueryData){
    uint16_t test_size = sizeof(sdp_test_record_list)/4;
    sdp_parser_handle_service_search(sdp_test_record_list, test_size, test_size);
}


int main (int argc, const char * argv[]){
    return CommandLineTestRunner::RunAllTests(argc, argv);
}
