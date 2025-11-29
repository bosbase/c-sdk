#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bosbase_client bosbase_client;
typedef struct bosbase_subscription bosbase_subscription;
typedef struct bosbase_batch bosbase_batch;

typedef struct bosbase_error {
    int status;
    int is_abort;
    char* url;
    char* message;
    char* response;
} bosbase_error;

typedef struct bosbase_file_attachment {
    const char* field;
    const char* filename;
    const char* content_type;
    const uint8_t* data;
    size_t data_len;
} bosbase_file_attachment;

typedef void (*bosbase_record_event_cb)(const char* json_event, void* user_data);
typedef void (*bosbase_pubsub_cb)(const char* topic, const char* message_id, const char* created, const char* data_json, void* user_data);

void bosbase_free_string(char* str);
void bosbase_free_error(bosbase_error* err);

bosbase_client* bosbase_client_new(const char* base_url, const char* lang);
void bosbase_client_free(bosbase_client* client);

const char* bosbase_auth_token(bosbase_client* client);
char* bosbase_auth_record(bosbase_client* client);
int bosbase_auth_save(bosbase_client* client, const char* token, const char* record_json, bosbase_error** out_error);
void bosbase_auth_clear(bosbase_client* client);

int bosbase_filter(bosbase_client* client, const char* expr, const char* params_json, char** out_string, bosbase_error** out_error);

int bosbase_send(
    bosbase_client* client,
    const char* path,
    const char* method,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    int timeout_ms,
    const bosbase_file_attachment* files,
    size_t files_len,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_get_list(
    bosbase_client* client,
    const char* collection,
    int page,
    int per_page,
    const char* filter,
    const char* sort,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_get_one(
    bosbase_client* client,
    const char* collection,
    const char* record_id,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_get_full_list(
    bosbase_client* client,
    const char* collection,
    int batch_size,
    const char* filter,
    const char* sort,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_create(
    bosbase_client* client,
    const char* collection,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_update(
    bosbase_client* client,
    const char* collection,
    const char* record_id,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_delete(
    bosbase_client* client,
    const char* collection,
    const char* record_id,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_collection_auth_with_password(
    bosbase_client* client,
    const char* collection,
    const char* identity,
    const char* password,
    const char* body_json,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_auth_refresh(
    bosbase_client* client,
    const char* collection,
    const char* body_json,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_collection_request_password_reset(
    bosbase_client* client,
    const char* collection,
    const char* email,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_collection_confirm_password_reset(
    bosbase_client* client,
    const char* collection,
    const char* token,
    const char* password,
    const char* password_confirm,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_collection_request_verification(
    bosbase_client* client,
    const char* collection,
    const char* email,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_collection_confirm_verification(
    bosbase_client* client,
    const char* collection,
    const char* token,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

bosbase_subscription* bosbase_collection_subscribe(
    bosbase_client* client,
    const char* collection,
    const char* topic,
    bosbase_record_event_cb callback,
    void* user_data,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

void bosbase_subscription_cancel(bosbase_subscription* sub);

int bosbase_files_get_url(
    bosbase_client* client,
    const char* record_json,
    const char* filename,
    const char* thumb,
    const char* token,
    int download,
    const char* query_json,
    char** out_url,
    bosbase_error** out_error);

int bosbase_files_get_token(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_health_check(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_graphql_query(
    bosbase_client* client,
    const char* query,
    const char* variables_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_sql_execute(
    bosbase_client* client,
    const char* query,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_create_collection(
    bosbase_client* client,
    const char* name,
    const char* config_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_update_collection(
    bosbase_client* client,
    const char* name,
    const char* config_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_list_collections(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_delete_collection(
    bosbase_client* client,
    const char* name,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_vector_insert(
    bosbase_client* client,
    const char* collection,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_batch_insert(
    bosbase_client* client,
    const char* collection,
    const char* options_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_get(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_update(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_delete(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_vector_list(
    bosbase_client* client,
    const char* collection,
    int page,
    int per_page,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_vector_search(
    bosbase_client* client,
    const char* collection,
    const char* options_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_list(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_create(
    bosbase_client* client,
    const char* name,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_update(
    bosbase_client* client,
    const char* name,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_delete(
    bosbase_client* client,
    const char* name,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_cache_set_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    const char* value_json,
    int ttl_seconds,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_get_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_renew_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    int ttl_seconds,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cache_delete_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_backup_get_full_list(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_backup_create(
    bosbase_client* client,
    const char* basename,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_backup_upload(
    bosbase_client* client,
    const bosbase_file_attachment* file,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_backup_delete(
    bosbase_client* client,
    const char* key,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_backup_restore(
    bosbase_client* client,
    const char* key,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_backup_get_download_url(
    bosbase_client* client,
    const char* token,
    const char* key,
    char** out_url,
    bosbase_error** out_error);

int bosbase_cron_get_full_list(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_cron_run(
    bosbase_client* client,
    const char* job_id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_logs_get_list(
    bosbase_client* client,
    int page,
    int per_page,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_logs_get_one(
    bosbase_client* client,
    const char* id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_logs_get_stats(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_settings_get_all(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_settings_update(
    bosbase_client* client,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_settings_test_s3(
    bosbase_client* client,
    const char* filesystem,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_settings_test_email(
    bosbase_client* client,
    const char* collection,
    const char* to_email,
    const char* template_name,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_settings_generate_apple_client_secret(
    bosbase_client* client,
    const char* client_id,
    const char* team_id,
    const char* key_id,
    const char* private_key,
    int duration,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_langchaingo_completions(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_langchaingo_rag(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_langchaingo_query_documents(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_langchaingo_sql(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_llm_list_collections(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_llm_create_collection(
    bosbase_client* client,
    const char* name,
    const char* metadata_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_llm_delete_collection(
    bosbase_client* client,
    const char* name,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_llm_insert(
    bosbase_client* client,
    const char* collection,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_llm_get(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_llm_update(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_llm_delete(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error);

int bosbase_llm_list(
    bosbase_client* client,
    const char* collection,
    int page,
    int per_page,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_llm_query(
    bosbase_client* client,
    const char* collection,
    const char* options_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

bosbase_batch* bosbase_batch_new(bosbase_client* client);
void bosbase_batch_free(bosbase_batch* batch);

int bosbase_batch_collection_create(
    bosbase_batch* batch,
    const char* collection,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    bosbase_error** out_error);

int bosbase_batch_collection_upsert(
    bosbase_batch* batch,
    const char* collection,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    bosbase_error** out_error);

int bosbase_batch_collection_update(
    bosbase_batch* batch,
    const char* collection,
    const char* record_id,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    bosbase_error** out_error);

int bosbase_batch_collection_delete(
    bosbase_batch* batch,
    const char* collection,
    const char* record_id,
    const char* body_json,
    const char* query_json,
    bosbase_error** out_error);

int bosbase_batch_send(
    bosbase_batch* batch,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error);

int bosbase_pubsub_publish(
    bosbase_client* client,
    const char* topic,
    const char* data_json,
    char** out_json,
    bosbase_error** out_error);

bosbase_subscription* bosbase_pubsub_subscribe(
    bosbase_client* client,
    const char* topic,
    bosbase_pubsub_cb callback,
    void* user_data,
    bosbase_error** out_error);

void bosbase_pubsub_disconnect(bosbase_client* client);

#ifdef __cplusplus
}
#endif
