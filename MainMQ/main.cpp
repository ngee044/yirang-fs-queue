#include "ArgumentParser.h"
#include "Converter.h"
#include "File.h"
#include "SQLiteAdapter.h"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace
{
	auto read_text_file(const std::string& path) -> std::tuple<std::optional<std::string>, std::optional<std::string>>
	{
		Utilities::File source;
		auto [opened, open_message] = source.open(path, std::ios::in | std::ios::binary);
		if (!opened)
		{
			return { std::nullopt, open_message };
		}

		auto [data, read_message] = source.read_bytes();
		source.close();
		if (data == std::nullopt)
		{
			return { std::nullopt, read_message };
		}

		return { Utilities::Converter::to_string(data.value()), std::nullopt };
	}

	auto load_json(const std::string& path) -> std::tuple<std::optional<json>, std::optional<std::string>>
	{
		auto [text, message] = read_text_file(path);
		if (text == std::nullopt)
		{
			return { std::nullopt, message };
		}

		try
		{
			return { json::parse(text.value()), std::nullopt };
		}
		catch (const std::exception& e)
		{
			return { std::nullopt, std::string("config parse error: ") + e.what() };
		}
	}

	auto resolve_path(const std::filesystem::path& base, const std::string& input) -> std::string
	{
		if (input.empty())
		{
			return input;
		}

		std::filesystem::path target(input);
		if (target.is_relative())
		{
			return (base / target).lexically_normal().string();
		}

		return target.lexically_normal().string();
	}
}

int main(int argc, char* argv[])
{
	Utilities::ArgumentParser parser(argc, argv);
	const std::string config_path = parser.to_string("--config").value_or(parser.program_folder() + "main_mq_configuration.json");

	auto [config_json, config_message] = load_json(config_path);
	if (config_json == std::nullopt)
	{
		std::cerr << "Failed to load config: " << config_message.value_or("unknown error") << std::endl;
		return 1;
	}

	const auto config_dir = std::filesystem::path(config_path).parent_path();
	const auto& config = config_json.value();
	const std::string backend = config.value("backend", "sqlite");
	if (backend != "sqlite")
	{
		std::cerr << "Only sqlite backend is supported (current: " << backend << ")" << std::endl;
		return 1;
	}

	const auto sqlite_json = config.value("sqlite", json::object());
	std::string schema_path = sqlite_json.value("schemaPath", "");
	if (schema_path.empty())
	{
		schema_path = parser.program_folder() + "sqlite_schema.sql";
	}
	schema_path = resolve_path(config_dir, schema_path);

	YiRang::MQ::BackendConfig backend_config;
	backend_config.type = YiRang::MQ::BackendType::SQLite;
	backend_config.sqlite.db_path = resolve_path(config_dir, sqlite_json.value("dbPath", "./data/yirangmq.db"));
	backend_config.sqlite.kv_table = sqlite_json.value("kvTable", "kv");
	backend_config.sqlite.message_index_table = sqlite_json.value("messageIndexTable", "msg_index");
	backend_config.sqlite.busy_timeout_ms = sqlite_json.value("busyTimeoutMs", 5000);
	backend_config.sqlite.journal_mode = sqlite_json.value("journalMode", "WAL");
	backend_config.sqlite.synchronous = sqlite_json.value("synchronous", "NORMAL");

	YiRang::MQ::SQLiteAdapter adapter(schema_path);
	auto [opened, open_message] = adapter.open(backend_config);
	if (!opened)
	{
		std::cerr << "Failed to initialize sqlite backend: " << open_message.value_or("unknown error") << std::endl;
		return 1;
	}

	std::cout << "MainMQ initialized (SQLite schema applied)." << std::endl;
	return 0;
}
