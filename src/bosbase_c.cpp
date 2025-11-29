#include "bosbase_c.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

#include "bosbase/bosbase.h"
#include "bosbase/error.h"
#include "bosbase/utils.h"

namespace {

char* dup_string(const std::string& s) {
    auto* data = static_cast<char*>(std::malloc(s.size() + 1));
    if (!data) {
        return nullptr;
    }
    std::memcpy(data, s.c_str(), s.size() + 1);
    return data;
}

char* dup_json(const nlohmann::json& j) {
    return dup_string(j.dump());
}

bosbase_error* make_error(const std::string& message, int status = -1) {
    auto* err = static_cast<bosbase_error*>(std::malloc(sizeof(bosbase_error)));
    if (!err) return nullptr;
    err->status = status;
    err->is_abort = 0;
    err->url = nullptr;
    err->message = dup_string(message);
    err->response = nullptr;
    return err;
}

bosbase_error* make_error(const bosbase::ClientResponseError& ex) {
    auto* err = static_cast<bosbase_error*>(std::malloc(sizeof(bosbase_error)));
    if (!err) return nullptr;
    err->status = static_cast<int>(ex.status());
    err->is_abort = ex.isAbort() ? 1 : 0;
    err->url = dup_string(ex.url());
    err->message = dup_string(ex.response().dump());
    err->response = dup_string(ex.response().dump());
    return err;
}

nlohmann::json parse_json_or(const char* text, const nlohmann::json& fallback = nlohmann::json::object()) {
    if (!text || !*text) return fallback;
    return nlohmann::json::parse(text);
}

std::map<std::string, nlohmann::json> parse_query(const char* text) {
    std::map<std::string, nlohmann::json> result;
    if (!text || !*text) return result;
    auto parsed = nlohmann::json::parse(text);
    if (!parsed.is_object()) {
        throw std::runtime_error("query_json must be an object");
    }
    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        result[it.key()] = it.value();
    }
    return result;
}

std::map<std::string, std::string> parse_headers(const char* text) {
    std::map<std::string, std::string> result;
    if (!text || !*text) return result;
    auto parsed = nlohmann::json::parse(text);
    if (!parsed.is_object()) {
        throw std::runtime_error("headers_json must be an object");
    }
    for (auto it = parsed.begin(); it != parsed.end(); ++it) {
        if (it.value().is_string()) {
            result[it.key()] = it.value().get<std::string>();
        } else {
            result[it.key()] = it.value().dump();
        }
    }
    return result;
}

std::vector<bosbase::FileAttachment> convert_files(const bosbase_file_attachment* files, size_t len) {
    std::vector<bosbase::FileAttachment> out;
    if (!files || len == 0) return out;
    out.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        bosbase::FileAttachment att;
        if (files[i].field) att.field = files[i].field;
        if (files[i].filename) att.filename = files[i].filename;
        if (files[i].content_type) att.contentType = files[i].content_type;
        if (files[i].data && files[i].data_len > 0) {
            att.data.assign(files[i].data, files[i].data + files[i].data_len);
        }
        out.push_back(std::move(att));
    }
    return out;
}

template <typename Fn>
int wrap_json(Fn&& fn, char** out_json, bosbase_error** out_error) {
    try {
        auto result = fn();
        if (out_json) {
            *out_json = dup_json(result);
        }
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

template <typename Fn>
int wrap_void(Fn&& fn, bosbase_error** out_error) {
    try {
        fn();
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

struct SubscriptionHolder {
    std::function<void()> cancel;
};

} // namespace

struct bosbase_client {
    std::unique_ptr<bosbase::BosBase> impl;
};

struct bosbase_subscription {
    std::shared_ptr<SubscriptionHolder> holder;
};

struct bosbase_batch {
    bosbase_client* owner;
    std::unique_ptr<bosbase::BatchService> impl;
};

void bosbase_free_string(char* str) {
    std::free(str);
}

void bosbase_free_error(bosbase_error* err) {
    if (!err) return;
    bosbase_free_string(err->url);
    bosbase_free_string(err->message);
    bosbase_free_string(err->response);
    std::free(err);
}

bosbase_client* bosbase_client_new(const char* base_url, const char* lang) {
    std::string base = base_url ? base_url : "/";
    std::string language = lang ? lang : "en-US";
    auto client = std::make_unique<bosbase::BosBase>(base, nullptr, language);
    auto* wrapper = new bosbase_client();
    wrapper->impl = std::move(client);
    return wrapper;
}

void bosbase_client_free(bosbase_client* client) {
    delete client;
}

bosbase_batch* bosbase_batch_new(bosbase_client* client) {
    if (!client || !client->impl) return nullptr;
    auto batch = client->impl->createBatch();
    if (!batch) return nullptr;
    auto* wrapper = new bosbase_batch();
    wrapper->owner = client;
    wrapper->impl = std::move(batch);
    return wrapper;
}

void bosbase_batch_free(bosbase_batch* batch) {
    delete batch;
}

const char* bosbase_auth_token(bosbase_client* client) {
    if (!client || !client->impl) return "";
    return client->impl->authStore()->token().c_str();
}

char* bosbase_auth_record(bosbase_client* client) {
    if (!client || !client->impl) return nullptr;
    return dup_json(client->impl->authStore()->record());
}

int bosbase_auth_save(bosbase_client* client, const char* token, const char* record_json, bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    return wrap_void([&]() {
        auto data = parse_json_or(record_json, nlohmann::json::object());
        client->impl->authStore()->save(token ? token : "", data);
    }, out_error);
}

void bosbase_auth_clear(bosbase_client* client) {
    if (!client || !client->impl) return;
    client->impl->authStore()->clear();
}

int bosbase_filter(bosbase_client* client, const char* expr, const char* params_json, char** out_string, bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto params = parse_query(params_json);
        auto result = client->impl->filter(expr ? expr : "", params);
        if (out_string) {
            *out_string = dup_string(result);
        }
        return 0;
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    bosbase::SendOptions opts;
    opts.method = method && *method ? method : "GET";
    try {
        opts.body = parse_json_or(body_json, nlohmann::json::object());
        opts.query = parse_query(query_json);
        opts.headers = parse_headers(headers_json);
        if (timeout_ms > 0) {
            opts.timeoutMs = timeout_ms;
        }
        opts.files = convert_files(files, files_len);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }

    return wrap_json([&]() {
        return client->impl->send(path ? path : "/", std::move(opts));
    }, out_json, out_error);
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<std::string> f = filter && *filter ? std::optional<std::string>(filter) : std::nullopt;
        std::optional<std::string> s = sort && *sort ? std::optional<std::string>(sort) : std::nullopt;
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.getList(page, per_page, false, query, headers, f, s, e, fld);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_get_one(
    bosbase_client* client,
    const char* collection,
    const char* record_id,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.getOne(record_id ? record_id : "", query, headers, e, fld);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<std::string> f = filter && *filter ? std::optional<std::string>(filter) : std::nullopt;
        std::optional<std::string> s = sort && *sort ? std::optional<std::string>(sort) : std::nullopt;
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.getFullList(batch_size, query, headers, f, s, e, fld);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        auto attachments = convert_files(files, files_len);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.create(body, query, attachments, headers, e, fld);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        auto attachments = convert_files(files, files_len);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.update(record_id ? record_id : "", body, query, attachments, headers, e, fld);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_delete(
    bosbase_client* client,
    const char* collection,
    const char* record_id,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            svc.remove(record_id ? record_id : "", body, query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.authWithPassword(identity ? identity : "", password ? password : "", e, fld, body, query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_auth_refresh(
    bosbase_client* client,
    const char* collection,
    const char* body_json,
    const char* expand,
    const char* fields,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        return wrap_json([&]() {
            return svc.authRefresh(body, query, headers, e, fld);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_request_password_reset(
    bosbase_client* client,
    const char* collection,
    const char* email,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            svc.requestPasswordReset(email ? email : "", body, query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_confirm_password_reset(
    bosbase_client* client,
    const char* collection,
    const char* token,
    const char* password,
    const char* password_confirm,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            svc.confirmPasswordReset(token ? token : "", password ? password : "", password_confirm ? password_confirm : "", body, query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_request_verification(
    bosbase_client* client,
    const char* collection,
    const char* email,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            svc.requestVerification(email ? email : "", body, query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_collection_confirm_verification(
    bosbase_client* client,
    const char* collection,
    const char* token,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            svc.confirmVerification(token ? token : "", body, query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

bosbase_subscription* bosbase_collection_subscribe(
    bosbase_client* client,
    const char* collection,
    const char* topic,
    bosbase_record_event_cb callback,
    void* user_data,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl || !callback) return nullptr;
    try {
        auto& svc = client->impl->collection(collection ? collection : "");
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);

        auto holder = std::make_shared<SubscriptionHolder>();
        auto cb = [callback, user_data](const nlohmann::json& evt) {
            auto* json = dup_json(evt);
            callback(json, user_data);
            bosbase_free_string(json);
        };
        holder->cancel = svc.subscribe(topic ? topic : "*", cb, query, headers);
        auto* sub = new bosbase_subscription();
        sub->holder = holder;
        return sub;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return nullptr;
}

void bosbase_subscription_cancel(bosbase_subscription* sub) {
    if (!sub || !sub->holder) return;
    if (sub->holder->cancel) {
        sub->holder->cancel();
    }
    delete sub;
}

int bosbase_files_get_url(
    bosbase_client* client,
    const char* record_json,
    const char* filename,
    const char* thumb,
    const char* token,
    int download,
    const char* query_json,
    char** out_url,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto record = parse_json_or(record_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        std::optional<std::string> tmb = thumb && *thumb ? std::optional<std::string>(thumb) : std::nullopt;
        std::optional<std::string> tkn = token && *token ? std::optional<std::string>(token) : std::nullopt;
        std::optional<bool> dl;
        if (download == 0 || download == 1) {
            dl = download == 1;
        }
        auto url = client->impl->files->getURL(record, filename ? filename : "", tmb, tkn, dl, query);
        if (out_url) *out_url = dup_string(url);
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

int bosbase_files_get_token(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->files->getToken(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_health_check(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->health->check(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_graphql_query(
    bosbase_client* client,
    const char* query,
    const char* variables_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto vars = parse_json_or(variables_json, nlohmann::json::object());
        auto q = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->graphql->sendQuery(query ? query : "", vars, q, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_sql_execute(
    bosbase_client* client,
    const char* query,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query_params = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            auto result = client->impl->sql->execute(query ? query : "", query_params, headers);
            nlohmann::json payload;
            payload["columns"] = result.columns;
            payload["rows"] = result.rows;
            if (result.rowsAffected) payload["rowsAffected"] = *result.rowsAffected;
            return payload;
        }, out_json, out_error);
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

int bosbase_vector_create_collection(
    bosbase_client* client,
    const char* name,
    const char* config_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(config_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/collections/" + bosbase::encodePathSegment(name ? name : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_update_collection(
    bosbase_client* client,
    const char* name,
    const char* config_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(config_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "PATCH";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/collections/" + bosbase::encodePathSegment(name ? name : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_list_collections(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            bosbase::SendOptions opts;
            opts.query = query;
            opts.headers = headers;
            return client->impl->send("/api/vectors/collections", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_delete_collection(
    bosbase_client* client,
    const char* name,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            bosbase::SendOptions opts;
            opts.method = "DELETE";
            opts.query = query;
            opts.headers = headers;
            client->impl->send("/api/vectors/collections/" + bosbase::encodePathSegment(name ? name : ""), std::move(opts));
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_insert(
    bosbase_client* client,
    const char* collection,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(document_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_batch_insert(
    bosbase_client* client,
    const char* collection,
    const char* options_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(options_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : "") + "/documents/batch", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_get(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : "") + "/" + bosbase::encodePathSegment(id ? id : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_update(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(document_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "PATCH";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : "") + "/" + bosbase::encodePathSegment(id ? id : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_delete(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            bosbase::SendOptions opts;
            opts.method = "DELETE";
            opts.query = query;
            opts.headers = headers;
            client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : "") + "/" + bosbase::encodePathSegment(id ? id : ""), std::move(opts));
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_list(
    bosbase_client* client,
    const char* collection,
    int page,
    int per_page,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        if (page > 0) query["page"] = page;
        if (per_page > 0) query["perPage"] = per_page;
        return wrap_json([&]() {
            bosbase::SendOptions opts;
            opts.query = query;
            opts.headers = headers;
            return client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_vector_search(
    bosbase_client* client,
    const char* collection,
    const char* options_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(options_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/vectors/" + bosbase::encodePathSegment(collection ? collection : "") + "/documents/search", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_list(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->caches->list(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_create(
    bosbase_client* client,
    const char* name,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<int> size_bytes;
        if (body.contains("sizeBytes") && body["sizeBytes"].is_number_integer()) {
            size_bytes = body["sizeBytes"].get<int>();
        }
        std::optional<int> default_ttl;
        if (body.contains("defaultTTLSeconds") && body["defaultTTLSeconds"].is_number_integer()) {
            default_ttl = body["defaultTTLSeconds"].get<int>();
        }
        std::optional<int> read_timeout;
        if (body.contains("readTimeoutMs") && body["readTimeoutMs"].is_number_integer()) {
            read_timeout = body["readTimeoutMs"].get<int>();
        }
        return wrap_json([&]() {
            return client->impl->caches->create(
                name ? name : "",
                size_bytes,
                default_ttl,
                read_timeout,
                body,
                query,
                headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_update(
    bosbase_client* client,
    const char* name,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->caches->update(name ? name : "", body, query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_delete(
    bosbase_client* client,
    const char* name,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            client->impl->caches->remove(name ? name : "", query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto value = parse_json_or(value_json, nlohmann::json::object());
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<int> ttl;
        if (ttl_seconds >= 0) ttl = ttl_seconds;
        return wrap_json([&]() {
            return client->impl->caches->setEntry(cache ? cache : "", key ? key : "", value, ttl, body, query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_get_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->caches->getEntry(cache ? cache : "", key ? key : "", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_renew_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    int ttl_seconds,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        std::optional<int> ttl;
        if (ttl_seconds >= 0) ttl = ttl_seconds;
        return wrap_json([&]() {
            return client->impl->caches->renewEntry(cache ? cache : "", key ? key : "", ttl, body, query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cache_delete_entry(
    bosbase_client* client,
    const char* cache,
    const char* key,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            client->impl->caches->deleteEntry(cache ? cache : "", key ? key : "", query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_backup_get_full_list(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->backups->getFullList(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_backup_create(
    bosbase_client* client,
    const char* basename,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->backups->create(basename ? basename : "", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_backup_upload(
    bosbase_client* client,
    const bosbase_file_attachment* file,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto attachments = convert_files(file, file ? 1 : 0);
        if (attachments.empty()) {
            throw std::runtime_error("file is required for backup upload");
        }
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->backups->upload(attachments.front(), query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_backup_delete(
    bosbase_client* client,
    const char* key,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_void([&]() {
            client->impl->backups->remove(key ? key : "", query, headers);
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_backup_restore(
    bosbase_client* client,
    const char* key,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->backups->restore(key ? key : "", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_backup_get_download_url(
    bosbase_client* client,
    const char* token,
    const char* key,
    char** out_url,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto url = client->impl->backups->getDownloadURL(token ? token : "", key ? key : "");
        if (out_url) *out_url = dup_string(url);
        return 0;
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cron_get_full_list(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->crons->getFullList(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_cron_run(
    bosbase_client* client,
    const char* job_id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->crons->run(job_id ? job_id : "", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_logs_get_list(
    bosbase_client* client,
    int page,
    int per_page,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->logs->getList(page, per_page, query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_logs_get_one(
    bosbase_client* client,
    const char* id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->logs->getOne(id ? id : "", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_logs_get_stats(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->logs->getStats(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_settings_get_all(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->settings->getAll(query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_settings_update(
    bosbase_client* client,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->settings->update(body, query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_settings_test_s3(
    bosbase_client* client,
    const char* filesystem,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->settings->testS3(filesystem && *filesystem ? filesystem : "storage", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_settings_test_email(
    bosbase_client* client,
    const char* collection,
    const char* to_email,
    const char* template_name,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->settings->testEmail(collection ? collection : "", to_email ? to_email : "", template_name ? template_name : "", query, headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

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
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return client->impl->settings->generateAppleClientSecret(
                client_id ? client_id : "",
                team_id ? team_id : "",
                key_id ? key_id : "",
                private_key ? private_key : "",
                duration,
                query,
                headers);
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_langchaingo_completions(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(payload_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/langchaingo/completions", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_langchaingo_rag(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(payload_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/langchaingo/rag", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_langchaingo_query_documents(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(payload_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/langchaingo/documents/query", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_langchaingo_sql(
    bosbase_client* client,
    const char* payload_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(payload_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/langchaingo/sql", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_list_collections(
    bosbase_client* client,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            bosbase::SendOptions opts;
            opts.query = query;
            opts.headers = headers;
            return client->impl->send("/api/llm-documents/collections", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_create_collection(
    bosbase_client* client,
    const char* name,
    const char* metadata_json,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        auto body = parse_json_or(metadata_json, nlohmann::json::object());
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body.is_null() ? nlohmann::json::object() : body;
        opts.query = query;
        opts.headers = headers;
        return wrap_void([&]() {
            client->impl->send("/api/llm-documents/collections/" + bosbase::encodePathSegment(name ? name : ""), std::move(opts));
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_delete_collection(
    bosbase_client* client,
    const char* name,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        return wrap_void([&]() {
            client->impl->send("/api/llm-documents/collections/" + bosbase::encodePathSegment(name ? name : ""), std::move(opts));
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_insert(
    bosbase_client* client,
    const char* collection,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(document_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/llm-documents/" + bosbase::encodePathSegment(collection ? collection : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_get(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            bosbase::SendOptions opts;
            opts.query = query;
            opts.headers = headers;
            return client->impl->send("/api/llm-documents/" + bosbase::encodePathSegment(collection ? collection : "") + "/" + bosbase::encodePathSegment(id ? id : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_update(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* document_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(document_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "PATCH";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/llm-documents/" + bosbase::encodePathSegment(collection ? collection : "") + "/" + bosbase::encodePathSegment(id ? id : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_delete(
    bosbase_client* client,
    const char* collection,
    const char* id,
    const char* query_json,
    const char* headers_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "DELETE";
        opts.query = query;
        opts.headers = headers;
        return wrap_void([&]() {
            client->impl->send("/api/llm-documents/" + bosbase::encodePathSegment(collection ? collection : "") + "/" + bosbase::encodePathSegment(id ? id : ""), std::move(opts));
        }, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_list(
    bosbase_client* client,
    const char* collection,
    int page,
    int per_page,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        if (page > 0) query["page"] = page;
        if (per_page > 0) query["perPage"] = per_page;
        bosbase::SendOptions opts;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/llm-documents/" + bosbase::encodePathSegment(collection ? collection : ""), std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_llm_query(
    bosbase_client* client,
    const char* collection,
    const char* options_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto body = parse_json_or(options_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        bosbase::SendOptions opts;
        opts.method = "POST";
        opts.body = body;
        opts.query = query;
        opts.headers = headers;
        return wrap_json([&]() {
            return client->impl->send("/api/llm-documents/" + bosbase::encodePathSegment(collection ? collection : "") + "/documents/query", std::move(opts));
        }, out_json, out_error);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
        return -1;
    }
}

int bosbase_batch_collection_create(
    bosbase_batch* batch,
    const char* collection,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    bosbase_error** out_error) {
    if (!batch || !batch->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto attachments = convert_files(files, files_len);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        batch->impl->collection(collection ? collection : "").create(body, query, attachments, e, fld);
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

int bosbase_batch_collection_upsert(
    bosbase_batch* batch,
    const char* collection,
    const char* body_json,
    const bosbase_file_attachment* files,
    size_t files_len,
    const char* expand,
    const char* fields,
    const char* query_json,
    bosbase_error** out_error) {
    if (!batch || !batch->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto attachments = convert_files(files, files_len);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        batch->impl->collection(collection ? collection : "").upsert(body, query, attachments, e, fld);
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

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
    bosbase_error** out_error) {
    if (!batch || !batch->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto attachments = convert_files(files, files_len);
        std::optional<std::string> e = expand && *expand ? std::optional<std::string>(expand) : std::nullopt;
        std::optional<std::string> fld = fields && *fields ? std::optional<std::string>(fields) : std::nullopt;
        batch->impl->collection(collection ? collection : "").update(record_id ? record_id : "", body, query, attachments, e, fld);
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

int bosbase_batch_collection_delete(
    bosbase_batch* batch,
    const char* collection,
    const char* record_id,
    const char* body_json,
    const char* query_json,
    bosbase_error** out_error) {
    if (!batch || !batch->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        batch->impl->collection(collection ? collection : "").remove(record_id ? record_id : "", body, query);
        return 0;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

int bosbase_batch_send(
    bosbase_batch* batch,
    const char* body_json,
    const char* query_json,
    const char* headers_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!batch || !batch->impl) return -1;
    try {
        auto body = parse_json_or(body_json, nlohmann::json::object());
        auto query = parse_query(query_json);
        auto headers = parse_headers(headers_json);
        return wrap_json([&]() {
            return batch->impl->send(body, query, headers);
        }, out_json, out_error);
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

int bosbase_pubsub_publish(
    bosbase_client* client,
    const char* topic,
    const char* data_json,
    char** out_json,
    bosbase_error** out_error) {
    if (!client || !client->impl) return -1;
    try {
        auto data = parse_json_or(data_json, nlohmann::json::object());
        return wrap_json([&]() {
            auto msg = client->impl->pubsub->publish(topic ? topic : "", data);
            nlohmann::json payload;
            payload["id"] = msg.id;
            payload["topic"] = msg.topic;
            payload["created"] = msg.created;
            payload["data"] = msg.data;
            return payload;
        }, out_json, out_error);
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return -1;
}

bosbase_subscription* bosbase_pubsub_subscribe(
    bosbase_client* client,
    const char* topic,
    bosbase_pubsub_cb callback,
    void* user_data,
    bosbase_error** out_error) {
    if (!client || !client->impl || !callback) return nullptr;
    try {
        auto holder = std::make_shared<SubscriptionHolder>();
        auto cb = [callback, user_data](const bosbase::PubSubMessage& msg) {
            auto data = dup_json(msg.data);
            callback(msg.topic.c_str(), msg.id.c_str(), msg.created.c_str(), data, user_data);
            bosbase_free_string(data);
        };
        holder->cancel = client->impl->pubsub->subscribe(topic ? topic : "", cb);
        auto* sub = new bosbase_subscription();
        sub->holder = holder;
        return sub;
    } catch (const bosbase::ClientResponseError& ex) {
        if (out_error) *out_error = make_error(ex);
    } catch (const std::exception& ex) {
        if (out_error) *out_error = make_error(ex.what());
    }
    return nullptr;
}

void bosbase_pubsub_disconnect(bosbase_client* client) {
    if (!client || !client->impl) return;
    client->impl->pubsub->disconnect();
}
