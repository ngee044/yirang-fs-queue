#include "Configurations.h"

#include "Converter.h"
#include "File.h"
#include "Logger.h"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <format>

using json = nlohmann::json;

namespace YiRang
{
	namespace MQ
	{
		Configurations::Configurations(ArgumentParser&& arguments)
			: write_file_(LogTypes::None)
			, write_console_(LogTypes::Information)
			, root_path_("")
			, data_root_("./data")
			, log_root_("./logs")
			, schema_version_("0.1")
			, node_id_("local-01")
			, backend_type_(BackendType::SQLite)
			, lease_visibility_timeout_sec_(30)
			, lease_sweep_interval_ms_(1000)
		{
			// SQLite defaults
			sqlite_config_.db_path = "./data/yirangmq.db";
			sqlite_config_.kv_table = "kv";
			sqlite_config_.message_index_table = "msg_index";
			sqlite_config_.busy_timeout_ms = 5000;
			sqlite_config_.journal_mode = "WAL";
			sqlite_config_.synchronous = "NORMAL";

			// FileSystem defaults
			filesystem_config_.root = "./data/fs";
			filesystem_config_.inbox_dir = "inbox";
			filesystem_config_.processing_dir = "processing";
			filesystem_config_.archive_dir = "archive";
			filesystem_config_.dlq_dir = "dlq";
			filesystem_config_.meta_dir = "meta";

			// Policy defaults
			policy_defaults_.visibility_timeout_sec = 30;
			policy_defaults_.retry.limit = 5;
			policy_defaults_.retry.backoff = "exponential";
			policy_defaults_.retry.initial_delay_sec = 1;
			policy_defaults_.retry.max_delay_sec = 60;
			policy_defaults_.dlq.enabled = true;
			policy_defaults_.dlq.queue = "";
			policy_defaults_.dlq.retention_days = 14;

			root_path_ = arguments.program_folder();

			load();
			parse(arguments);
		}

		Configurations::~Configurations(void) {}

		auto Configurations::write_file() -> LogTypes { return write_file_; }
		auto Configurations::write_console() -> LogTypes { return write_console_; }

		auto Configurations::root_path() -> std::string { return root_path_; }
		auto Configurations::data_root() -> std::string { return data_root_; }
		auto Configurations::log_root() -> std::string { return log_root_; }

		auto Configurations::schema_version() -> std::string { return schema_version_; }
		auto Configurations::node_id() -> std::string { return node_id_; }
		auto Configurations::backend_type() -> BackendType { return backend_type_; }

		auto Configurations::sqlite_config() -> SQLiteConfig { return sqlite_config_; }
		auto Configurations::sqlite_db_path() -> std::string { return sqlite_config_.db_path; }
		auto Configurations::sqlite_kv_table() -> std::string { return sqlite_config_.kv_table; }
		auto Configurations::sqlite_message_index_table() -> std::string { return sqlite_config_.message_index_table; }
		auto Configurations::sqlite_schema_path() -> std::string { return root_path_ + "sqlite_schema.sql"; }
		auto Configurations::sqlite_busy_timeout_ms() -> int32_t { return sqlite_config_.busy_timeout_ms; }
		auto Configurations::sqlite_journal_mode() -> std::string { return sqlite_config_.journal_mode; }
		auto Configurations::sqlite_synchronous() -> std::string { return sqlite_config_.synchronous; }

		auto Configurations::filesystem_config() -> FileSystemConfig { return filesystem_config_; }

		auto Configurations::lease_visibility_timeout_sec() -> int32_t { return lease_visibility_timeout_sec_; }
		auto Configurations::lease_sweep_interval_ms() -> int32_t { return lease_sweep_interval_ms_; }

		auto Configurations::policy_defaults() -> QueuePolicy { return policy_defaults_; }

		auto Configurations::queues() -> std::vector<QueueConfig>& { return queues_; }

		auto Configurations::backend_config() -> BackendConfig
		{
			BackendConfig config;
			config.type = backend_type_;
			config.sqlite = sqlite_config_;
			config.filesystem = filesystem_config_;
			return config;
		}

		auto Configurations::load() -> void
		{
			std::filesystem::path path = root_path_ + "main_mq_configuration.json";
			if (!std::filesystem::exists(path))
			{
				Logger::handle().write(LogTypes::Error, std::format("Configuration file does not exist: {}", path.string()));
				return;
			}

			Utilities::File source;
			auto [opened, open_error] = source.open(path.string(), std::ios::in | std::ios::binary);
			if (!opened)
			{
				Logger::handle().write(LogTypes::Error, open_error.value_or("Failed to open configuration file"));
				return;
			}

			auto [source_data, read_error] = source.read_bytes();
			source.close();

			if (source_data == std::nullopt)
			{
				Logger::handle().write(LogTypes::Error, read_error.value_or("Failed to read configuration file"));
				return;
			}

			try
			{
				json config = json::parse(Utilities::Converter::to_string(source_data.value()));

				// Schema version
				if (config.contains("schemaVersion") && config["schemaVersion"].is_string())
				{
					schema_version_ = config["schemaVersion"].get<std::string>();
				}

				// Node ID
				if (config.contains("nodeId") && config["nodeId"].is_string())
				{
					node_id_ = config["nodeId"].get<std::string>();
				}

				// Backend type
				if (config.contains("backend") && config["backend"].is_string())
				{
					std::string backend = config["backend"].get<std::string>();
					if (backend == "sqlite")
					{
						backend_type_ = BackendType::SQLite;
					}
					else if (backend == "filesystem" || backend == "fs")
					{
						backend_type_ = BackendType::FileSystem;
					}
				}

				// Paths
				if (config.contains("paths") && config["paths"].is_object())
				{
					auto& paths = config["paths"];
					if (paths.contains("dataRoot") && paths["dataRoot"].is_string())
					{
						data_root_ = paths["dataRoot"].get<std::string>();
					}
					if (paths.contains("logRoot") && paths["logRoot"].is_string())
					{
						log_root_ = paths["logRoot"].get<std::string>();
					}
				}

				// SQLite config
				if (config.contains("sqlite") && config["sqlite"].is_object())
				{
					auto& sqlite = config["sqlite"];
					if (sqlite.contains("dbPath") && sqlite["dbPath"].is_string())
					{
						sqlite_config_.db_path = sqlite["dbPath"].get<std::string>();
					}
					if (sqlite.contains("kvTable") && sqlite["kvTable"].is_string())
					{
						sqlite_config_.kv_table = sqlite["kvTable"].get<std::string>();
					}
					if (sqlite.contains("messageIndexTable") && sqlite["messageIndexTable"].is_string())
					{
						sqlite_config_.message_index_table = sqlite["messageIndexTable"].get<std::string>();
					}
					if (sqlite.contains("busyTimeoutMs") && sqlite["busyTimeoutMs"].is_number())
					{
						sqlite_config_.busy_timeout_ms = sqlite["busyTimeoutMs"].get<int32_t>();
					}
					if (sqlite.contains("journalMode") && sqlite["journalMode"].is_string())
					{
						sqlite_config_.journal_mode = sqlite["journalMode"].get<std::string>();
					}
					if (sqlite.contains("synchronous") && sqlite["synchronous"].is_string())
					{
						sqlite_config_.synchronous = sqlite["synchronous"].get<std::string>();
					}
				}

				// FileSystem config
				if (config.contains("filesystem") && config["filesystem"].is_object())
				{
					auto& fs = config["filesystem"];
					if (fs.contains("root") && fs["root"].is_string())
					{
						filesystem_config_.root = fs["root"].get<std::string>();
					}
					if (fs.contains("inboxDir") && fs["inboxDir"].is_string())
					{
						filesystem_config_.inbox_dir = fs["inboxDir"].get<std::string>();
					}
					if (fs.contains("processingDir") && fs["processingDir"].is_string())
					{
						filesystem_config_.processing_dir = fs["processingDir"].get<std::string>();
					}
					if (fs.contains("archiveDir") && fs["archiveDir"].is_string())
					{
						filesystem_config_.archive_dir = fs["archiveDir"].get<std::string>();
					}
					if (fs.contains("dlqDir") && fs["dlqDir"].is_string())
					{
						filesystem_config_.dlq_dir = fs["dlqDir"].get<std::string>();
					}
					if (fs.contains("metaDir") && fs["metaDir"].is_string())
					{
						filesystem_config_.meta_dir = fs["metaDir"].get<std::string>();
					}
				}

				// Lease config
				if (config.contains("lease") && config["lease"].is_object())
				{
					auto& lease = config["lease"];
					if (lease.contains("visibilityTimeoutSec") && lease["visibilityTimeoutSec"].is_number())
					{
						lease_visibility_timeout_sec_ = lease["visibilityTimeoutSec"].get<int32_t>();
					}
					if (lease.contains("sweepIntervalMs") && lease["sweepIntervalMs"].is_number())
					{
						lease_sweep_interval_ms_ = lease["sweepIntervalMs"].get<int32_t>();
					}
				}

				// Policy defaults
				if (config.contains("policyDefaults") && config["policyDefaults"].is_object())
				{
					policy_defaults_ = load_queue_policy(&config["policyDefaults"]);
				}

				// Queues
				if (config.contains("queues") && config["queues"].is_array())
				{
					queues_.clear();
					for (auto& queue_json : config["queues"])
					{
						QueueConfig queue_config;
						if (queue_json.contains("name") && queue_json["name"].is_string())
						{
							queue_config.name = queue_json["name"].get<std::string>();
						}
						if (queue_json.contains("policy") && queue_json["policy"].is_object())
						{
							queue_config.policy = load_queue_policy(&queue_json["policy"]);
						}
						else
						{
							queue_config.policy = policy_defaults_;
						}
						queues_.push_back(queue_config);
					}
				}
			}
			catch (const json::exception& e)
			{
				Logger::handle().write(LogTypes::Error, std::format("JSON parse error: {}", e.what()));
			}
		}

		auto Configurations::parse(ArgumentParser& arguments) -> void
		{
			auto string_target = arguments.to_string("--backend");
			if (string_target != std::nullopt)
			{
				if (string_target.value() == "sqlite")
				{
					backend_type_ = BackendType::SQLite;
				}
				else if (string_target.value() == "filesystem" || string_target.value() == "fs")
				{
					backend_type_ = BackendType::FileSystem;
				}
			}

			string_target = arguments.to_string("--db-path");
			if (string_target != std::nullopt)
			{
				sqlite_config_.db_path = string_target.value();
			}

			string_target = arguments.to_string("--data-root");
			if (string_target != std::nullopt)
			{
				data_root_ = string_target.value();
			}

			string_target = arguments.to_string("--log-root");
			if (string_target != std::nullopt)
			{
				log_root_ = string_target.value();
			}

			string_target = arguments.to_string("--node-id");
			if (string_target != std::nullopt)
			{
				node_id_ = string_target.value();
			}

			auto int_target = arguments.to_int("--visibility-timeout");
			if (int_target != std::nullopt)
			{
				lease_visibility_timeout_sec_ = int_target.value();
			}

			int_target = arguments.to_int("--write-console-log");
			if (int_target != std::nullopt)
			{
				write_console_ = static_cast<LogTypes>(int_target.value());
			}

			int_target = arguments.to_int("--write-file-log");
			if (int_target != std::nullopt)
			{
				write_file_ = static_cast<LogTypes>(int_target.value());
			}
		}

		auto Configurations::load_retry_policy(const void* json_obj) -> RetryPolicy
		{
			RetryPolicy policy;
			policy.limit = policy_defaults_.retry.limit;
			policy.backoff = policy_defaults_.retry.backoff;
			policy.initial_delay_sec = policy_defaults_.retry.initial_delay_sec;
			policy.max_delay_sec = policy_defaults_.retry.max_delay_sec;

			const json& obj = *static_cast<const json*>(json_obj);

			if (obj.contains("limit") && obj["limit"].is_number())
			{
				policy.limit = obj["limit"].get<int32_t>();
			}
			if (obj.contains("backoff") && obj["backoff"].is_string())
			{
				policy.backoff = obj["backoff"].get<std::string>();
			}
			if (obj.contains("initialDelaySec") && obj["initialDelaySec"].is_number())
			{
				policy.initial_delay_sec = obj["initialDelaySec"].get<int32_t>();
			}
			if (obj.contains("maxDelaySec") && obj["maxDelaySec"].is_number())
			{
				policy.max_delay_sec = obj["maxDelaySec"].get<int32_t>();
			}

			return policy;
		}

		auto Configurations::load_dlq_policy(const void* json_obj) -> DlqPolicy
		{
			DlqPolicy policy;
			policy.enabled = policy_defaults_.dlq.enabled;
			policy.queue = policy_defaults_.dlq.queue;
			policy.retention_days = policy_defaults_.dlq.retention_days;

			const json& obj = *static_cast<const json*>(json_obj);

			if (obj.contains("enabled") && obj["enabled"].is_boolean())
			{
				policy.enabled = obj["enabled"].get<bool>();
			}
			if (obj.contains("queue") && obj["queue"].is_string())
			{
				policy.queue = obj["queue"].get<std::string>();
			}
			if (obj.contains("retentionDays") && obj["retentionDays"].is_number())
			{
				policy.retention_days = obj["retentionDays"].get<int32_t>();
			}

			return policy;
		}

		auto Configurations::load_queue_policy(const void* json_obj) -> QueuePolicy
		{
			QueuePolicy policy;
			policy.visibility_timeout_sec = policy_defaults_.visibility_timeout_sec;

			const json& obj = *static_cast<const json*>(json_obj);

			if (obj.contains("visibilityTimeoutSec") && obj["visibilityTimeoutSec"].is_number())
			{
				policy.visibility_timeout_sec = obj["visibilityTimeoutSec"].get<int32_t>();
			}
			if (obj.contains("retry") && obj["retry"].is_object())
			{
				policy.retry = load_retry_policy(&obj["retry"]);
			}
			else
			{
				policy.retry = policy_defaults_.retry;
			}
			if (obj.contains("dlq") && obj["dlq"].is_object())
			{
				policy.dlq = load_dlq_policy(&obj["dlq"]);
			}
			else
			{
				policy.dlq = policy_defaults_.dlq;
			}

			return policy;
		}
	} // namespace MQ
} // namespace YiRang
