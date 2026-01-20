#pragma once

#include "BackendAdapter.h"

#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

enum class ValidationRuleType
{
	Required,
	Type,
	MinLength,
	MaxLength,
	MinValue,
	MaxValue,
	Pattern,
	Enum,
	Custom
};

struct ValidationRule
{
	std::string field;
	ValidationRuleType type;
	std::string expected_type;
	int64_t min_length = 0;
	int64_t max_length = INT64_MAX;
	double min_value = 0;
	double max_value = 0;
	std::string pattern;
	std::vector<std::string> enum_values;
	std::function<bool(const std::string&)> custom_validator;
	std::string error_message;
};

struct ValidationResult
{
	bool valid = true;
	std::vector<std::string> errors;
};

struct MessageSchema
{
	std::string name;
	std::string description;
	std::vector<ValidationRule> rules;
};

class MessageValidator
{
public:
	MessageValidator(void) = default;
	~MessageValidator(void) = default;

	auto register_schema(const std::string& queue, const MessageSchema& schema) -> void;
	auto unregister_schema(const std::string& queue) -> void;
	auto has_schema(const std::string& queue) const -> bool;
	auto validate(const MessageEnvelope& message) -> ValidationResult;
	auto validate_json(const std::string& json_str, const std::string& queue) -> ValidationResult;
	auto get_schema(const std::string& queue) const -> std::optional<MessageSchema>;

	static auto required(const std::string& field, const std::string& error_msg = "") -> ValidationRule;
	static auto type_string(const std::string& field, const std::string& error_msg = "") -> ValidationRule;
	static auto type_number(const std::string& field, const std::string& error_msg = "") -> ValidationRule;
	static auto type_boolean(const std::string& field, const std::string& error_msg = "") -> ValidationRule;
	static auto type_array(const std::string& field, const std::string& error_msg = "") -> ValidationRule;
	static auto type_object(const std::string& field, const std::string& error_msg = "") -> ValidationRule;
	static auto min_length(const std::string& field, int64_t min, const std::string& error_msg = "") -> ValidationRule;
	static auto max_length(const std::string& field, int64_t max, const std::string& error_msg = "") -> ValidationRule;
	static auto min_value(const std::string& field, double min, const std::string& error_msg = "") -> ValidationRule;
	static auto max_value(const std::string& field, double max, const std::string& error_msg = "") -> ValidationRule;
	static auto enum_values(const std::string& field, const std::vector<std::string>& values, const std::string& error_msg = "") -> ValidationRule;

private:
	auto validate_rule(const void* json_obj, const ValidationRule& rule) -> std::optional<std::string>;

private:
	std::map<std::string, MessageSchema> schemas_;
};
