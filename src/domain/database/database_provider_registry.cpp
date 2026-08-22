#include "domain/database/database_provider.h"

namespace datalab::domain {

DatabaseResult<bool> DatabaseProviderRegistry::register_factory(
    DatabaseProviderDescriptor descriptor,
    DatabaseProviderFactory factory)
{
    if (descriptor.id.empty()) {
        return DatabaseResult<bool>::failure(
            "empty_provider_id", "Provider ID 不能为空。");
    }
    if (!factory && descriptor.available) {
        return DatabaseResult<bool>::failure(
            "empty_factory", "可用 Provider 的 factory 不能为空。");
    }
    for (const Entry& entry : entries_) {
        if (entry.descriptor.id == descriptor.id) {
            return DatabaseResult<bool>::failure(
                "duplicate_provider_id",
                "重复的 Provider ID: " + descriptor.id);
        }
    }
    entries_.push_back(Entry{std::move(descriptor), std::move(factory)});
    return DatabaseResult<bool>::success(true);
}

std::vector<DatabaseProviderDescriptor> DatabaseProviderRegistry::descriptors() const
{
    std::vector<DatabaseProviderDescriptor> result;
    result.reserve(entries_.size());
    for (const Entry& entry : entries_) {
        result.push_back(entry.descriptor);
    }
    return result;
}

const DatabaseProviderDescriptor* DatabaseProviderRegistry::find(
    std::string_view provider_id) const
{
    for (const Entry& entry : entries_) {
        if (entry.descriptor.id == provider_id) {
            return &entry.descriptor;
        }
    }
    return nullptr;
}

DatabaseResult<std::unique_ptr<IDatabaseProvider>> DatabaseProviderRegistry::create(
    std::string_view provider_id) const
{
    for (const Entry& entry : entries_) {
        if (entry.descriptor.id != provider_id) {
            continue;
        }
        if (!entry.descriptor.available) {
            return DatabaseResult<std::unique_ptr<IDatabaseProvider>>::failure(
                "driver_missing",
                entry.descriptor.unavailable_reason.empty()
                    ? ("Provider 不可用: " + entry.descriptor.id)
                    : entry.descriptor.unavailable_reason);
        }
        if (!entry.factory) {
            return DatabaseResult<std::unique_ptr<IDatabaseProvider>>::failure(
                "empty_factory",
                "Provider factory 为空: " + entry.descriptor.id);
        }
        std::unique_ptr<IDatabaseProvider> provider = entry.factory();
        if (!provider) {
            return DatabaseResult<std::unique_ptr<IDatabaseProvider>>::failure(
                "factory_failed",
                "Provider factory 返回空对象: " + entry.descriptor.id);
        }
        return DatabaseResult<std::unique_ptr<IDatabaseProvider>>::success(
            std::move(provider));
    }
    return DatabaseResult<std::unique_ptr<IDatabaseProvider>>::failure(
        "unknown_provider",
        "未注册的 Provider ID: " + std::string(provider_id));
}

DatabaseProviderRegistry& default_database_provider_registry()
{
    static DatabaseProviderRegistry registry;
    return registry;
}

}  // namespace datalab::domain
