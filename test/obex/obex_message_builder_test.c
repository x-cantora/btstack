#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"

#include "btstack_util.h"
#include "btstack_debug.h"
#include "classic/obex.h"

static uint8_t obex_message_builder_packet_init(uint8_t * buffer, uint16_t buffer_len, uint8_t opcode){
    if (buffer_len < 3) return ERROR_CODE_MEMORY_CAPACITY_EXCEEDED;
    buffer[0] = opcode;
    big_endian_store_16(buffer, 1, 3);
    return ERROR_CODE_SUCCESS;
}

static uint8_t obex_message_builder_packet_append(uint8_t * buffer, uint16_t buffer_len, const uint8_t * data, uint16_t len){
    uint16_t pos = big_endian_read_16(buffer, 1);
    if (buffer_len < pos + len) return ERROR_CODE_MEMORY_CAPACITY_EXCEEDED;
    memcpy(&buffer[pos], data, len);
    pos += len;
    big_endian_store_16(buffer, 1, pos);
     return ERROR_CODE_SUCCESS;
}

static uint8_t obex_message_builder_header_add_variable(uint8_t * buffer, uint16_t buffer_len, uint8_t header_type, const uint8_t * header_data, uint16_t header_data_length){
    uint8_t header[3];
    header[0] = header_type;
    big_endian_store_16(header, 1, sizeof(header) + header_data_length);
    
    uint8_t status = obex_message_builder_packet_append(buffer, buffer_len, &header[0], sizeof(header));
    if (status != ERROR_CODE_SUCCESS) return status;

    return obex_message_builder_packet_append(buffer, buffer_len, header_data, header_data_length);        
}

static uint8_t obex_message_builder_header_add_byte(uint8_t * buffer, uint16_t buffer_len, uint8_t header_type, uint8_t value){
    uint8_t header[2];
    header[0] = header_type;
    header[1] = value;
    return obex_message_builder_packet_append(buffer, buffer_len, &header[0], sizeof(header));
}

static uint8_t obex_message_builder_header_add_word(uint8_t * buffer, uint16_t buffer_len, uint8_t header_type, uint32_t value){
    uint8_t header[5];
    header[0] = header_type;
    big_endian_store_32(header, 1, value);
    return obex_message_builder_packet_append(buffer, buffer_len, &header[0], sizeof(header));
}

static uint8_t obex_message_builder_header_add_connection_id(uint8_t * buffer, uint16_t buffer_len, uint32_t obex_connection_id){
    // add connection_id header if set, must be first header if used
    if (obex_connection_id == OBEX_CONNECTION_ID_INVALID) return ERROR_CODE_PARAMETER_OUT_OF_MANDATORY_RANGE;
    return obex_message_builder_header_add_word(buffer, buffer_len, OBEX_HEADER_CONNECTION_ID, obex_connection_id);
}

uint8_t obex_message_builder_request_create_connect(uint8_t * buffer, uint16_t buffer_len, uint8_t obex_version_number, uint8_t flags, uint16_t maximum_obex_packet_length){
    uint8_t status = obex_message_builder_packet_init(buffer, buffer_len, OBEX_OPCODE_CONNECT);
    if (status != ERROR_CODE_SUCCESS) return status;

    uint8_t fields[4];
    fields[0] = obex_version_number;
    fields[1] = flags;
    big_endian_store_16(fields, 2, maximum_obex_packet_length);
    return obex_message_builder_packet_append(buffer, buffer_len, &fields[0], sizeof(fields));
}

uint8_t obex_message_builder_request_create_get(uint8_t * buffer, uint16_t buffer_len, uint32_t obex_connection_id){
    uint8_t status = obex_message_builder_packet_init(buffer, buffer_len, OBEX_OPCODE_GET | OBEX_OPCODE_FINAL_BIT_MASK);
    if (status != ERROR_CODE_SUCCESS) return status;
    return obex_message_builder_header_add_connection_id(buffer, buffer_len, obex_connection_id);
}

uint8_t obex_message_builder_request_create_put(uint8_t * buffer, uint16_t buffer_len, uint32_t obex_connection_id){
    uint8_t status = obex_message_builder_packet_init(buffer, buffer_len, OBEX_OPCODE_PUT | OBEX_OPCODE_FINAL_BIT_MASK);
    if (status != ERROR_CODE_SUCCESS) return status;

    return obex_message_builder_header_add_connection_id(buffer, buffer_len, obex_connection_id);
}

uint8_t obex_message_builder_request_create_set_path(uint8_t * buffer, uint16_t buffer_len, uint8_t flags, uint32_t obex_connection_id){
    uint8_t status = obex_message_builder_packet_init(buffer, buffer_len, OBEX_OPCODE_SETPATH);
    if (status != ERROR_CODE_SUCCESS) return status;

    uint8_t fields[2];
    fields[0] = flags;
    fields[1] = 0;  // reserved
    status = obex_message_builder_packet_append(buffer, buffer_len, &fields[0], sizeof(fields));
    if (status != ERROR_CODE_SUCCESS) return status;
    return obex_message_builder_header_add_connection_id(buffer, buffer_len, obex_connection_id);
}

uint8_t obex_message_builder_request_create_abort(uint8_t * buffer, uint16_t buffer_len, uint32_t obex_connection_id){
    uint8_t status = obex_message_builder_packet_init(buffer, buffer_len, OBEX_OPCODE_ABORT);
    if (status != ERROR_CODE_SUCCESS) return status;
    return obex_message_builder_header_add_connection_id(buffer, buffer_len, obex_connection_id);
}

uint8_t obex_message_builder_request_create_disconnect(uint8_t * buffer, uint16_t buffer_len, uint32_t obex_connection_id){
    uint8_t status = obex_message_builder_packet_init(buffer, buffer_len, OBEX_OPCODE_DISCONNECT);
    if (status != ERROR_CODE_SUCCESS) return status;
    return obex_message_builder_header_add_connection_id(buffer, buffer_len, obex_connection_id);
}

uint8_t obex_message_builder_header_add_srm_enable(uint8_t * buffer, uint16_t buffer_len){
    return obex_message_builder_header_add_byte(buffer, buffer_len, OBEX_HEADER_SINGLE_RESPONSE_MODE, OBEX_SRM_ENABLE);
}

uint8_t obex_message_builder_add_header_target(uint8_t * buffer, uint16_t buffer_len, const uint8_t * target, uint16_t length){
    return obex_message_builder_header_add_variable(buffer, buffer_len, OBEX_HEADER_TARGET, target, length);
}

uint8_t obex_message_builder_add_header_application_parameters(uint8_t * buffer, uint16_t buffer_len, const uint8_t * data, uint16_t length){
    return obex_message_builder_header_add_variable(buffer, buffer_len, OBEX_HEADER_APPLICATION_PARAMETERS, data, length);
}

uint8_t obex_message_builder_add_header_challenge_response(uint8_t * buffer, uint16_t buffer_len, const uint8_t * data, uint16_t length){
    return obex_message_builder_header_add_variable(buffer, buffer_len, OBEX_HEADER_AUTHENTICATION_RESPONSE, data, length);
}

uint8_t obex_message_builder_add_body_static(uint8_t * buffer, uint16_t buffer_len, const uint8_t * data, uint32_t length){
    return obex_message_builder_header_add_variable(buffer, buffer_len, OBEX_HEADER_END_OF_BODY, data, length);
}

uint8_t obex_message_builder_header_add_name(uint8_t * buffer, uint16_t buffer_len, const char * name){
    int len = strlen(name); 
    if (len) {
        // empty string does not have trailing \0
        len++;
    }
    if (buffer_len <  (1 + 2 + len*2) ) return ERROR_CODE_MEMORY_CAPACITY_EXCEEDED;

    uint16_t pos = big_endian_read_16(buffer, 1);
    buffer[pos++] = OBEX_HEADER_NAME;
    big_endian_store_16(buffer, pos, 1 + 2 + len*2);
    pos += 2;
    int i;
    // @note name[len] == 0 
    for (i = 0 ; i < len ; i++){
        buffer[pos++] = 0;
        buffer[pos++] = *name++;
    }
    big_endian_store_16(buffer, 1, pos);
    return ERROR_CODE_SUCCESS;
}

uint8_t obex_message_builder_header_add_type(uint8_t * buffer, uint16_t buffer_len, const char * type){
    uint8_t header[3];
    header[0] = OBEX_HEADER_TYPE;
    int len_incl_zero = strlen(type) + 1;
    big_endian_store_16(header, 1, 1 + 2 + len_incl_zero);

    uint8_t status = obex_message_builder_packet_append(buffer, buffer_len, &header[0], sizeof(header));
    if (status != ERROR_CODE_SUCCESS) return status;
    return obex_message_builder_packet_append(buffer, buffer_len, (const uint8_t*)type, len_incl_zero);
}

static const uint8_t service_uuid[] = {0xbb, 0x58, 0x2b, 0x40, 0x42, 0xc, 0x11, 0xdb, 0xb0, 0xde, 0x8, 0x0, 0x20, 0xc, 0x9a, 0x66};
static const uint8_t application_parameters[] = {0x29, 4, 0, 0, 0xFF, 0xFF};
static const char    path_element[] = {'t','e','s','t'};

TEST_GROUP(OBEX_MESSAGE_BUILDER){
    uint8_t  actual_message[300];
    uint16_t actual_message_len;
    uint8_t  obex_version_number;
    uint8_t  flags;
    uint16_t maximum_obex_packet_length;
    uint8_t  status;
    uint32_t connection_id;
    // uint32_t map_supported_features = 0x1F;

    void setup(void){
        actual_message_len = sizeof(actual_message);
        memset(actual_message, 0, actual_message_len);
        flags = 1 << 1;
        obex_version_number = OBEX_VERSION;
        maximum_obex_packet_length = 0xFFFF;
        status = ERROR_CODE_MEMORY_CAPACITY_EXCEEDED;
        connection_id = 10;
    }

    void CHECK_EQUAL_ARRAY(const uint8_t * expected, uint8_t * actual, int size){
        for (int i=0; i<size; i++){
            // printf("%03u: %02x - %02x\n", i, expected[i], actual[i]);
            BYTES_EQUAL(expected[i], actual[i]);
        }
    }

    void validate(uint8_t * expected_message, uint16_t expected_len, uint8_t expected_status){
        CHECK_EQUAL(status, expected_status);
        uint16_t actual_len = big_endian_read_16(actual_message, 1);
        CHECK_EQUAL(actual_len, expected_len);
        CHECK_EQUAL_ARRAY(actual_message, expected_message, expected_len);
    }

    void validate_invalid_parameter(uint8_t * expected_message, uint16_t expected_len){
        validate(expected_message, expected_len, ERROR_CODE_PARAMETER_OUT_OF_MANDATORY_RANGE);
    }

    void validate_success(uint8_t * expected_message, uint16_t expected_len){
        validate(expected_message, expected_len, ERROR_CODE_SUCCESS);
    }
};


TEST(OBEX_MESSAGE_BUILDER, CreateConnect){
    uint8_t  expected_message[] = {OBEX_OPCODE_CONNECT, 0, 0, obex_version_number, flags, 0, 0};
    expected_message[2] = sizeof(expected_message);
    big_endian_store_16(expected_message, 5, maximum_obex_packet_length);
    
    status = obex_message_builder_request_create_connect(actual_message, actual_message_len, obex_version_number, flags, maximum_obex_packet_length);
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateGet){
    uint8_t  expected_message[] = {OBEX_OPCODE_GET | OBEX_OPCODE_FINAL_BIT_MASK, 0, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0};
    expected_message[2] = sizeof(expected_message);
    big_endian_store_32(expected_message, 4, connection_id);
    
    status = obex_message_builder_request_create_get(actual_message, actual_message_len, connection_id);
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateGetInvalidConnectionID){
    uint8_t  expected_message[] = {OBEX_OPCODE_GET | OBEX_OPCODE_FINAL_BIT_MASK, 0, 0};
    expected_message[2] = sizeof(expected_message);

    status = obex_message_builder_request_create_get(actual_message, actual_message_len, OBEX_CONNECTION_ID_INVALID);
    validate_invalid_parameter(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreatePut){
    uint8_t  expected_message[] = {OBEX_OPCODE_PUT | OBEX_OPCODE_FINAL_BIT_MASK, 0, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0};
    expected_message[2] = sizeof(expected_message);
    big_endian_store_32(expected_message, 4, connection_id);
    
    status = obex_message_builder_request_create_put(actual_message, actual_message_len, connection_id);
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateSetPath){
    uint8_t  expected_message[] = {OBEX_OPCODE_SETPATH, 0, 0, flags, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0};
    expected_message[2] = sizeof(expected_message);
    big_endian_store_32(expected_message, 6, connection_id);
    
    status = obex_message_builder_request_create_set_path(actual_message, actual_message_len, flags, connection_id);
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateAbort){
    uint8_t  expected_message[] = {OBEX_OPCODE_ABORT, 0, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0};
    expected_message[2] = sizeof(expected_message);
    big_endian_store_32(expected_message, 4, connection_id);
    
    status = obex_message_builder_request_create_abort(actual_message, actual_message_len, connection_id);
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateDisconnect){
    uint8_t  expected_message[] = {OBEX_OPCODE_DISCONNECT, 0, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0};
    expected_message[2] = sizeof(expected_message);
    big_endian_store_32(expected_message, 4, connection_id);
    
    status = obex_message_builder_request_create_disconnect(actual_message, actual_message_len, connection_id);
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateGetAddHeader){
    uint8_t  expected_message[] = {OBEX_OPCODE_GET | OBEX_OPCODE_FINAL_BIT_MASK, 0, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0, OBEX_HEADER_SINGLE_RESPONSE_MODE, OBEX_SRM_ENABLE};
    expected_message[2] = 8; // only get request
    big_endian_store_32(expected_message, 4, connection_id);

    status = obex_message_builder_request_create_get(actual_message, actual_message_len, connection_id);
    validate_success(expected_message, expected_message[2]);
    
    status = obex_message_builder_header_add_srm_enable(actual_message, actual_message_len);
    expected_message[2] += 2;
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateConnectWithHeaderTarget){
    uint8_t  expected_message[] = {OBEX_OPCODE_CONNECT, 0, 0, obex_version_number, flags, 0, 0, 
        // service UUID
        OBEX_HEADER_TARGET, 0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    };
    int header_len = 3 + sizeof(service_uuid);
    expected_message[2] = 7;
    big_endian_store_16(expected_message, 5, maximum_obex_packet_length);
    big_endian_store_16(expected_message, 8, header_len);
    memcpy(expected_message + 10, service_uuid, sizeof(service_uuid));

    status = obex_message_builder_request_create_connect(actual_message, actual_message_len, obex_version_number, flags, maximum_obex_packet_length);
    validate_success(expected_message, expected_message[2]);

    status = obex_message_builder_add_header_target(actual_message, actual_message_len, service_uuid, sizeof(service_uuid));
    expected_message[2] += header_len;
    validate_success(expected_message, expected_message[2]);
}

TEST(OBEX_MESSAGE_BUILDER, CreateConnectWithHeaderApplicationParameters){
    uint8_t  expected_message[] = {OBEX_OPCODE_CONNECT, 0, 0, obex_version_number, flags, 0, 0, 
        // service UUID
        OBEX_HEADER_APPLICATION_PARAMETERS, 0,0, 0,0,0,0,0,0
        // OBEX_HEADER_AUTHENTICATION_RESPONSE, 0,0
        // OBEX_HEADER_END_OF_BODY, 0,0
    };
    int header_len = 3 + sizeof(application_parameters);
    expected_message[2] = 7;
    big_endian_store_16(expected_message, 5, maximum_obex_packet_length);
    big_endian_store_16(expected_message, 8, header_len);
    memcpy(expected_message + 10, application_parameters, sizeof(application_parameters));

    status = obex_message_builder_request_create_connect(actual_message, actual_message_len, obex_version_number, flags, maximum_obex_packet_length);
    validate_success(expected_message, expected_message[2]);

    status = obex_message_builder_add_header_application_parameters(actual_message, actual_message_len, &application_parameters[0], sizeof(application_parameters));
    expected_message[2] += header_len;
    validate_success(expected_message, expected_message[2]);
}


TEST(OBEX_MESSAGE_BUILDER, CreateSetPathWithName){
    uint8_t  expected_message[] = {OBEX_OPCODE_SETPATH, 0, 0, flags, 0, OBEX_HEADER_CONNECTION_ID, 0, 0, 0, 0, OBEX_HEADER_NAME, 0x00, 0x0D, 0x00, 0x74, 0x00, 0x65, 0x00, 0x73, 0x00, 0x74, 0x00, 0x00};
    expected_message[2] = 10;
    big_endian_store_32(expected_message, 6, connection_id);
    
    status = obex_message_builder_request_create_set_path(actual_message, actual_message_len, flags, connection_id);
    validate_success(expected_message, expected_message[2]);

    expected_message[2] += 13;
    status = obex_message_builder_header_add_name(actual_message, actual_message_len, (const char *) path_element); 
    validate_success(expected_message, expected_message[2]);
}

int main (int argc, const char * argv[]){
    return CommandLineTestRunner::RunAllTests(argc, argv);
}