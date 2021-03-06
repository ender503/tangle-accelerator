#include <served/methods.hpp>
#include <served/plugins.hpp>
#include <served/served.hpp>
#include "accelerator/apis.h"
#include "accelerator/config.h"
#include "accelerator/errors.h"
#include "cJSON.h"

void set_options_method_header(served::response& res) {
  res.set_header("Access-Control-Allow-Origin", "*");
  res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  res.set_header("Access-Control-Allow-Headers",
                 "Origin, Content-Type, Accept");
  res.set_header("Access-Control-Max-Age", "86400");
}

int main(int, char const**) {
  served::multiplexer mux;
  mux.use_after(served::plugin::access_log);

  iota_client_service_t service;
  service.http.path = "/";
  service.http.host = IRI_HOST;
  service.http.port = IRI_PORT;
  service.http.api_version = 1;
  service.serializer_type = SR_JSON;
  iota_client_core_init(&service);

  /**
   * @method {get} /tag/:tag Find transactions by tag
   *
   * @param {String} tag Must be 27 trytes long
   *
   * @return {String[]} hashes List of transaction hashes
   */
  mux.handle("/tag/{tag:[A-Z9]{1,27}}/hashes")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([&](served::response& res, const served::request& req) {
        char* json_result;

        api_find_transactions_by_tag(&service, req.params["tag"].c_str(),
                                     &json_result);
        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {get} /transaction/:tx Get transaction object
   *
   * @param {String} tx Must be 81 trytes long transaction hash
   *
   * @return {String[]} object Info of enitre transaction object
   */
  mux.handle("/transaction/{tx:[A-Z9]{81}}")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([&](served::response& res, const served::request& req) {
        char* json_result;

        api_get_transaction_object(&service, req.params["tx"].c_str(),
                                   &json_result);
        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {get} /tag/:tag Find transaction objects by tag
   *
   * @param {String} tag Must be 27 trytes long
   *
   * @return {String[]} transactions List of transaction objects
   */
  mux.handle("/tag/{tag:[A-Z9]{1,27}}")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([&](served::response& res, const served::request& req) {
        char* json_result;

        api_find_transactions_obj_by_tag(&service, req.params["tag"].c_str(),
                                         &json_result);
        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {get} /tips Fetch pair tips which base on GetTransactionToApprove
   *
   * @return {String[]} tips Pair of transaction hashes
   */
  mux.handle("/tips/pair")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([&](served::response& res, const served::request& req) {
        char* json_result;

        api_get_tips_pair(&service, &json_result);
        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {get} /tips Fetch all tips
   *
   * @return {String[]} tips List of transaction hashes
   */
  mux.handle("/tips")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([&](served::response& res, const served::request& req) {
        char* json_result;

        api_get_tips(&service, &json_result);
        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {get} /address Generate an unused address
   *
   * @return {String} hash of address hashes
   */
  mux.handle("/address")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([&](served::response& res, const served::request& req) {
        char* json_result;

        api_generate_address(&service, &json_result);
        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {post} /transaction send transfer
   *
   * @return {String} transaction object
   */
  mux.handle("/transaction")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .post([&](served::response& res, const served::request& req) {
        char* json_result;

        if (req.header("content-type").find("application/json") ==
            std::string::npos) {
          cJSON* json_obj = cJSON_CreateObject();
          cJSON_AddStringToObject(json_obj, "message",
                                  "Invalid request header");
          json_result = cJSON_PrintUnformatted(json_obj);

          res.set_status(SC_BAD_REQUEST);
          cJSON_Delete(json_obj);
        } else {
          api_send_transfer(&service, req.body().c_str(), &json_result);
        }

        res.set_header("content-type", "application/json");
        res << json_result;
      });

  /**
   * @method {get} {*} Client bad request
   * @method {options} {*} Get server information
   *
   * @return {String} message Error message
   */
  mux.handle("{*}")
      .method(served::method::OPTIONS,
              [&](served::response& res, const served::request& req) {
                set_options_method_header(res);
              })
      .get([](served::response& res, const served::request&) {
        cJSON* json_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(json_obj, "message", "Invalid path");
        const char* json = cJSON_PrintUnformatted(json_obj);

        res.set_status(SC_BAD_REQUEST);
        res.set_header("content-type", "application/json");
        res << json;

        cJSON_Delete(json_obj);
      });

  std::cout << "Starting..." << std::endl;
  served::net::server server(TA_HOST, TA_PORT, mux);
  server.run(TA_THREAD_COUNT);

  iota_client_core_destroy(&service);
  return 0;
}
