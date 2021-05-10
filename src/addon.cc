#include <cstdint>
#include <cmath>

#include <type_traits>
#include <unordered_map>
#include <optional>
#include <vector>
#include <memory>

#include <napi.h>

#include "counter_group.h"

using namespace node_perf_counters;

namespace
{
    struct active_group_t {
        /**
         * Maps a unique counter to its position in the input of a create() operation.
         * Used during subsequent write operations.
         */
        std::unordered_map<counter, uint32_t> order;

        /** The group of counters */
        counter_group group;
    };

    /** Map from id to active group */
    std::unordered_map<int32_t, std::unique_ptr<active_group_t>> active_groups;
}

/** Throws an exception in the JS environment */
void js_throw(Napi::Env& env, std::string&& err)
{
    Napi::Error::New(env, std::move(err)).ThrowAsJavaScriptException();
}

/** Returns the counter group associated with the given id */
std::optional<active_group_t*> get_counter_group(
    const Napi::CallbackInfo& info, std::size_t arg)
{
    if (info.Length() < arg) {
        Napi::Env env = info.Env();
        js_throw(env, "Expected at least " + std::to_string(arg + 1) + "arguments");
        return std::nullopt;
    }

    int fd = info[arg].As<Napi::Number>().Int32Value();

    if (active_groups.count(fd) == 0) {
        Napi::Env env = info.Env();
        js_throw(env, "id " + std::to_string(fd) + " is invalid");
        return std::nullopt;
    }

    return active_groups[fd].get();
}

/** Create a new counter group */
Napi::Value create(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (info.Length() < 1) {
        return env.Undefined();
    }

    Napi::Array config = info[0].As<Napi::Array>();

    try {
        std::unordered_map<counter, uint32_t> order;
        std::vector<counter> counters;

        for (uint32_t i = 0; i < config.Length(); i++) {
            Napi::Value val = config[i];
            counter c = static_cast<counter>(val.As<Napi::Number>().Int32Value());

            if (std::get<1>(order.try_emplace(c, i))) {
                counters.push_back(c);
            }
        }

        std::unique_ptr<active_group_t> entry{
            new active_group_t{ std::move(order), counter_group{ std::move(counters) } }
        };

        auto id = entry->group.id();

        active_groups.emplace(id, std::move(entry));

        return Napi::Number::New(env, id);
    }
    catch (const std::runtime_error& err) {
        js_throw(env, std::string("Failed to create event (") + err.what() + ")");
        return env.Undefined();
    }
}

/** Return the current counter value for a given Counter type */
Napi::Value read(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();
    Napi::Value result = env.Undefined();

    if (auto entry = get_counter_group(info, 0)) {
        if (info.Length() < 2) {
            js_throw(env, "Expected a counter type");
            return result;
        }

        try {
            counter c = static_cast<counter>(info[1].As<Napi::Number>().Int32Value());
            (*entry)->group.read([&](counter type, std::int64_t value) {
                if (type == c) result = Napi::BigInt::New(env, value);
            });
        } catch (const std::runtime_error& err) {
            js_throw(env, err.what());
            return result;
        }
    }

    return result;
}

/** Write current counter values of the given group to a BigInt64Array */
void readAll(const Napi::CallbackInfo& info)
{
    Napi::Env env = info.Env();

    if (auto entry = get_counter_group(info, 0)) {
        if (info.Length() < 2) {
            js_throw(env, "Expected an object to write to");
            return;
        }

        try {
            Napi::BigInt64Array report = info[1].As<Napi::BigInt64Array>();
            (*entry)->group.read([&](counter type, std::int64_t value) {
                report[(*entry)->order[type]] = value;
            });
        } catch (const std::runtime_error& err) {
            js_throw(env, err.what());
            return;
        }
    }
}

/** Resets a counter group with the associated id */
void reset(const Napi::CallbackInfo& info)
{
    if (auto entry = get_counter_group(info, 0)) {
        (*entry)->group.reset();
    }
}

/** Stops a given counter group with the associated id */
void stop(const Napi::CallbackInfo& info)
{
    if (auto entry = get_counter_group(info, 0)) {
        active_groups.erase((*entry)->group.id());
    }
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
    exports["create"] = Napi::Function::New(env, create);
    exports["reset"] = Napi::Function::New(env, reset);
    exports["read"] = Napi::Function::New(env, read);
    exports["readAll"] = Napi::Function::New(env, readAll);
    exports["stop"] = Napi::Function::New(env, stop);

    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)
