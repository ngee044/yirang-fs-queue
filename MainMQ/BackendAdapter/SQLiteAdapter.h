#pragma once

#include "BackendAdapter.h"

#include "SQLite.h"

#include <string>

namespace YiRang
{
	namespace MQ
	{
		class SQLiteAdapter : public BackendAdapter
		{
		public:
			SQLiteAdapter(const std::string& schema_path);
			~SQLiteAdapter(void) override;

			auto open(const BackendConfig& config) -> std::tuple<bool, std::optional<std::string>> override;
			auto close(void) -> void override;

			auto enqueue(const MessageEnvelope& message) -> std::tuple<bool, std::optional<std::string>> override;
			auto lease_next(const std::string& queue, const std::string& consumer_id, const int32_t& visibility_timeout_sec)
				-> LeaseResult override;
			auto ack(const LeaseToken& lease) -> std::tuple<bool, std::optional<std::string>> override;
			auto nack(const LeaseToken& lease, const std::string& reason, const bool& requeue)
				-> std::tuple<bool, std::optional<std::string>> override;
			auto extend_lease(const LeaseToken& lease, const int32_t& visibility_timeout_sec)
				-> std::tuple<bool, std::optional<std::string>> override;

			auto load_policy(const std::string& queue) -> std::tuple<std::optional<QueuePolicy>, std::optional<std::string>> override;
			auto save_policy(const std::string& queue, const QueuePolicy& policy) -> std::tuple<bool, std::optional<std::string>> override;

			auto metrics(const std::string& queue) -> std::tuple<QueueMetrics, std::optional<std::string>> override;

		private:
			auto apply_pragmas(void) -> std::tuple<bool, std::optional<std::string>>;
			auto ensure_schema(void) -> std::tuple<bool, std::optional<std::string>>;
			auto load_schema_sql(void) -> std::tuple<std::optional<std::string>, std::optional<std::string>>;

		private:
			bool is_open_;
			std::string schema_path_;
			SQLiteConfig sqlite_config_;
			DataBase::SQLite db_;
		};
	} // namespace MQ
} // namespace YiRang
