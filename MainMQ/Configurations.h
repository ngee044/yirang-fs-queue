#pragma once

#include "ArgumentParser.h"
#include "BackendAdapter.h"
#include "LogTypes.h"

#include <string>
#include <vector>

using namespace Utilities;

namespace YiRang
{
	namespace MQ
	{
		struct QueueConfig
		{
			std::string name;
			QueuePolicy policy;
		};

		class Configurations
		{
		public:
			Configurations(ArgumentParser&& arguments);
			virtual ~Configurations(void);

			// Logging
			auto write_file() -> LogTypes;
			auto write_console() -> LogTypes;

			// Paths
			auto root_path() -> std::string;
			auto data_root() -> std::string;
			auto log_root() -> std::string;

			// General
			auto schema_version() -> std::string;
			auto node_id() -> std::string;
			auto backend_type() -> BackendType;

			// SQLite
			auto sqlite_config() -> SQLiteConfig;
			auto sqlite_db_path() -> std::string;
			auto sqlite_kv_table() -> std::string;
			auto sqlite_message_index_table() -> std::string;
			auto sqlite_schema_path() -> std::string;
			auto sqlite_busy_timeout_ms() -> int32_t;
			auto sqlite_journal_mode() -> std::string;
			auto sqlite_synchronous() -> std::string;

			// FileSystem
			auto filesystem_config() -> FileSystemConfig;

			// Lease
			auto lease_visibility_timeout_sec() -> int32_t;
			auto lease_sweep_interval_ms() -> int32_t;

			// Policy defaults
			auto policy_defaults() -> QueuePolicy;

			// Queues
			auto queues() -> std::vector<QueueConfig>&;

			// Backend config for adapter
			auto backend_config() -> BackendConfig;

		protected:
			auto load() -> void;
			auto parse(ArgumentParser& arguments) -> void;

		private:
			auto load_retry_policy(const void* json_obj) -> RetryPolicy;
			auto load_dlq_policy(const void* json_obj) -> DlqPolicy;
			auto load_queue_policy(const void* json_obj) -> QueuePolicy;

		private:
			// Logging
			LogTypes write_file_;
			LogTypes write_console_;

			// Paths
			std::string root_path_;
			std::string data_root_;
			std::string log_root_;

			// General
			std::string schema_version_;
			std::string node_id_;
			BackendType backend_type_;

			// SQLite
			SQLiteConfig sqlite_config_;

			// FileSystem
			FileSystemConfig filesystem_config_;

			// Lease
			int32_t lease_visibility_timeout_sec_;
			int32_t lease_sweep_interval_ms_;

			// Policy defaults
			QueuePolicy policy_defaults_;

			// Queues
			std::vector<QueueConfig> queues_;
		};
	} // namespace MQ
} // namespace YiRang
