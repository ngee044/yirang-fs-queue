#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

namespace YiRang
{
	namespace MQ
	{
		enum class BackendType
		{
			FileSystem,
			SQLite
		};

		enum class MessageState
		{
			Ready,
			Inflight,
			Delayed,
			Dlq,
			Archived
		};

		struct RetryPolicy
		{
			int32_t limit = 0;
			std::string backoff;
			int32_t initial_delay_sec = 0;
			int32_t max_delay_sec = 0;
		};

		struct DlqPolicy
		{
			bool enabled = false;
			std::string queue;
			int32_t retention_days = 0;
		};

		struct QueuePolicy
		{
			int32_t visibility_timeout_sec = 0;
			RetryPolicy retry;
			DlqPolicy dlq;
		};

		struct FileSystemConfig
		{
			std::string root;
			std::string inbox_dir;
			std::string processing_dir;
			std::string archive_dir;
			std::string dlq_dir;
			std::string meta_dir;
		};

		struct SQLiteConfig
		{
			std::string db_path;
			std::string kv_table;
			std::string message_index_table;
			int32_t busy_timeout_ms = 0;
			std::string journal_mode;
			std::string synchronous;
		};

		struct BackendConfig
		{
			BackendType type = BackendType::SQLite;
			FileSystemConfig filesystem;
			SQLiteConfig sqlite;
		};

		struct MessageEnvelope
		{
			std::string key;
			std::string message_id;
			std::string queue;
			std::string payload_json;
			std::string attributes_json;
			int32_t priority = 0;
			int32_t attempt = 0;
			int64_t created_at_ms = 0;
			int64_t available_at_ms = 0;
		};

		struct LeaseToken
		{
			std::string lease_id;
			std::string message_key;
			std::string consumer_id;
			int64_t lease_until_ms = 0;
		};

		struct LeaseResult
		{
			bool leased = false;
			std::optional<MessageEnvelope> message;
			std::optional<LeaseToken> lease;
			std::optional<std::string> error;
		};

		struct QueueMetrics
		{
			uint64_t ready = 0;
			uint64_t inflight = 0;
			uint64_t delayed = 0;
			uint64_t dlq = 0;
		};

		class BackendAdapter
		{
		public:
			virtual ~BackendAdapter(void) = default;

			virtual auto open(const BackendConfig& config) -> std::tuple<bool, std::optional<std::string>> = 0;
			virtual auto close(void) -> void = 0;

			virtual auto enqueue(const MessageEnvelope& message) -> std::tuple<bool, std::optional<std::string>> = 0;
			virtual auto lease_next(const std::string& queue, const std::string& consumer_id, const int32_t& visibility_timeout_sec)
				-> LeaseResult = 0;
			virtual auto ack(const LeaseToken& lease) -> std::tuple<bool, std::optional<std::string>> = 0;
			virtual auto nack(const LeaseToken& lease, const std::string& reason, const bool& requeue)
				-> std::tuple<bool, std::optional<std::string>> = 0;
			virtual auto extend_lease(const LeaseToken& lease, const int32_t& visibility_timeout_sec)
				-> std::tuple<bool, std::optional<std::string>> = 0;

			virtual auto load_policy(const std::string& queue) -> std::tuple<std::optional<QueuePolicy>, std::optional<std::string>> = 0;
			virtual auto save_policy(const std::string& queue, const QueuePolicy& policy) -> std::tuple<bool, std::optional<std::string>> = 0;

			virtual auto metrics(const std::string& queue) -> std::tuple<QueueMetrics, std::optional<std::string>> = 0;
		};
	} // namespace MQ
} // namespace YiRang
