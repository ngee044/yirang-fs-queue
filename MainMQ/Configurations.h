#pragma once

#include "ArgumentParser.h"
#include "BackendAdapter.h"
#include "LogTypes.h"
#include "MailboxTypes.h"
#include "MessageValidator.h"

#include <optional>
#include <string>
#include <vector>

using namespace Utilities;

// Operation mode
enum class OperationMode
{
	MailboxSqlite,
	MailboxFileSystem,
	Hybrid
};

struct QueueConfig
{
	std::string name;
	QueuePolicy policy;
	std::optional<MessageSchema> message_schema;
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
			auto operation_mode() -> OperationMode;

			// IPC (Mailbox)
			auto mailbox_config() -> MailboxConfig;

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
			auto validate() -> void;

		private:
			auto load_retry_policy(const void* json_obj) -> RetryPolicy;
			auto load_dlq_policy(const void* json_obj) -> DlqPolicy;
			auto load_queue_policy(const void* json_obj) -> QueuePolicy;
			auto load_message_schema(const void* json_obj) -> std::optional<MessageSchema>;
			auto load_validation_rule(const void* json_obj) -> std::optional<ValidationRule>;
			auto validate_retry_policy(RetryPolicy& policy, const std::string& context) -> void;
			auto validate_queue_policy(QueuePolicy& policy, const std::string& context) -> void;

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
			OperationMode operation_mode_;

			// IPC (Mailbox)
			MailboxConfig mailbox_config_;

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