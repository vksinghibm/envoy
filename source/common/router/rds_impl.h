#pragma once

#include "envoy/json/json_object.h"
#include "envoy/local_info/local_info.h"
#include "envoy/router/rds.h"
#include "envoy/thread_local/thread_local.h"

#include "common/common/logger.h"
#include "common/http/rest_api_fetcher.h"

namespace Router {

/**
 * Route configuration provider utilities.
 */
class RouteConfigProviderUtil {
public:
  /**
   * @return RouteConfigProviderPtr a new route configuration provider based on the supplied JSON
   *         configuration.
   */
  static RouteConfigProviderPtr create(const Json::Object& config, Runtime::Loader& runtime,
                                       Upstream::ClusterManager& cm, Event::Dispatcher& dispatcher,
                                       Runtime::RandomGenerator& random,
                                       const LocalInfo::LocalInfo& local_info, Stats::Scope& scope,
                                       const std::string& stat_prefix, ThreadLocal::Instance& tls);
};

/**
 * Implementation of RouteConfigProvider that holds a static route configuration.
 */
class StaticRouteConfigProviderImpl : public RouteConfigProvider {
public:
  StaticRouteConfigProviderImpl(const Json::Object& config, Runtime::Loader& runtime,
                                Upstream::ClusterManager& cm);

  // Router::RouteConfigProvider
  Router::ConfigPtr config() override { return config_; }

private:
  ConfigPtr config_;
};

/**
 * All RDS stats. @see stats_macros.h
 */
// clang-format off
#define ALL_RDS_STATS(COUNTER)                                                                     \
  COUNTER(config_reload)                                                                           \
  COUNTER(update_attempt)                                                                          \
  COUNTER(update_success)                                                                          \
  COUNTER(update_failure)
// clang-format on

/**
 * Struct definition for all RDS stats. @see stats_macros.h
 */
struct RdsStats {
  ALL_RDS_STATS(GENERATE_COUNTER_STRUCT)
};

/**
 * Implementation of RouteConfigProvider that fetches the route configuration dynamically using
 * the RDS API.
 */
class RdsRouteConfigProviderImpl : public RouteConfigProvider,
                                   Http::RestApiFetcher,
                                   Logger::Loggable<Logger::Id::router> {
public:
  RdsRouteConfigProviderImpl(const Json::Object& config, Runtime::Loader& runtime,
                             Upstream::ClusterManager& cm, Event::Dispatcher& dispatcher,
                             Runtime::RandomGenerator& random,
                             const LocalInfo::LocalInfo& local_info, Stats::Scope& scope,
                             const std::string& stat_prefix, ThreadLocal::Instance& tls);

  void initialize() { RestApiFetcher::initialize(); }

  // Router::RouteConfigProvider
  Router::ConfigPtr config() override;

  // Http::RestApiFetcher
  void createRequest(Http::Message& request) override;
  void parseResponse(const Http::Message& response) override;
  void onFetchComplete() override {}
  void onFetchFailure(EnvoyException* e) override;

private:
  struct ThreadLocalConfig : public ThreadLocal::ThreadLocalObject {
    ThreadLocalConfig(ConfigPtr initial_config) : config_(initial_config) {}

    // ThreadLocal::ThreadLocalObject
    void shutdown() override {}

    ConfigPtr config_;
  };

  Runtime::Loader& runtime_;
  const LocalInfo::LocalInfo& local_info_;
  ThreadLocal::Instance& tls_;
  const uint32_t tls_slot_;
  const std::string route_config_name_;
  bool initialized_{};
  uint64_t last_config_hash_{};
  RdsStats stats_;
};

} // Router
