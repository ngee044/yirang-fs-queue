#pragma once

#include "BackendAdapter.h"
#include "MailboxTypes.h"
#include "MessageValidator.h"
#include "ThreadPool.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>

class QueueManager;

class MailboxHandler
{
public:
	MailboxHandler(
		std::shared_ptr<BackendAdapter> backend,
		std::shared_ptr<QueueManager> queue_manager,
		const MailboxConfig& config
	);
	~MailboxHandler(void);

	auto start(void) -> std::tuple<bool, std::optional<std::string>>;
	auto stop(void) -> void;

	auto is_running(void) const -> bool { return running_.load(); }

private:
	// Directory management
	auto ensure_directories(void) -> std::tuple<bool, std::optional<std::string>>;
	auto build_path(const std::string& sub_dir, const std::string& filename = "") -> std::string;
	auto build_response_path(const std::string& client_id, const std::string& filename = "") -> std::string;

	// Request processing loop
	auto request_processing_worker(void) -> void;
	auto stale_cleanup_worker(void) -> void;

	// File operations (atomic write)
	auto read_request_file(const std::string& file_path) -> std::tuple<std::optional<MailboxRequest>, std::optional<std::string>>;
	auto write_response_file(const std::string& client_id, const MailboxResponse& response) -> std::tuple<bool, std::optional<std::string>>;
	auto atomic_write(const std::string& target_path, const std::string& content) -> std::tuple<bool, std::optional<std::string>>;
	auto move_to_processing(const std::string& request_file) -> std::tuple<std::string, std::optional<std::string>>;
	auto move_to_dead(const std::string& processing_file, const std::string& reason) -> std::tuple<bool, std::optional<std::string>>;
	auto delete_processed(const std::string& processing_file) -> std::tuple<bool, std::optional<std::string>>;

	// Request parsing
	auto parse_request(const std::string& json_content, const std::string& file_path) -> std::tuple<std::optional<MailboxRequest>, std::optional<std::string>>;
	auto parse_command(const std::string& command_str) -> MailboxCommand;

	// Command handlers
	auto handle_request(const MailboxRequest& request) -> MailboxResponse;
	auto handle_publish(const MailboxRequest& request) -> MailboxResponse;
	auto handle_consume_next(const MailboxRequest& request) -> MailboxResponse;
	auto handle_ack(const MailboxRequest& request) -> MailboxResponse;
	auto handle_nack(const MailboxRequest& request) -> MailboxResponse;
	auto handle_extend_lease(const MailboxRequest& request) -> MailboxResponse;
	auto handle_status(const MailboxRequest& request) -> MailboxResponse;
	auto handle_health(const MailboxRequest& request) -> MailboxResponse;
	auto handle_metrics(const MailboxRequest& request) -> MailboxResponse;
	auto handle_list_dlq(const MailboxRequest& request) -> MailboxResponse;
	auto handle_reprocess_dlq(const MailboxRequest& request) -> MailboxResponse;

	// Response building
	auto build_success_response(const std::string& request_id, const std::string& data_json = "{}") -> MailboxResponse;
	auto build_error_response(const std::string& request_id, const std::string& error_code, const std::string& error_message) -> MailboxResponse;

	// Stale request handling
	auto cleanup_stale_requests(void) -> void;
	auto cleanup_stale_responses(void) -> void;

	// Utilities
	auto current_time_ms(void) -> int64_t;
	auto generate_uuid(void) -> std::string;
	auto list_files(const std::string& dir_path) -> std::vector<std::string>;

	// Metrics helpers
	auto record_request_start(void) -> int64_t;
	auto record_request_end(MailboxCommand command, bool success, const std::string& error_code, int64_t start_time) -> void;

private:
	std::atomic<bool> running_;
	MailboxConfig config_;
	std::shared_ptr<BackendAdapter> backend_;
	std::shared_ptr<QueueManager> queue_manager_;
	std::shared_ptr<Thread::ThreadPool> thread_pool_;
	MessageValidator validator_;
	std::mutex processing_mutex_;
	mutable std::mutex metrics_mutex_;
	MailboxMetrics metrics_;
};
