#ifndef ZY_HTTP_TASK_HPP
#define ZY_HTTP_TASK_HPP

#include <coroutine>
#include <filesystem>
#include <optional>
#include <span>
#include <utility>
namespace zy_http {

template <typename ResultType>
class TaskPromise;

template <typename ResultType = void>
class Task {
  public:
    using promise_type = TaskPromise<ResultType>;

    explicit Task(std::coroutine_handle<promise_type> handle) noexcept : handle_(handle) {}

    Task(Task &) = delete;

    Task &operator=(Task &) = delete;

    Task(Task &&task) noexcept : handle_(std::exchange(task.handle_, {})) {}

    Task &operator=(Task &&other) noexcept = delete;

    ~Task() {
        if (handle_) handle_.destroy();
    }

    void *get_addr() { return handle_.address(); }

    auto detach() -> void { handle_ = nullptr; };

  private:
    std::coroutine_handle<promise_type> handle_;
};

template <typename T>
class TaskPromiseBase {
  public:
    auto initial_suspend() const noexcept -> std::suspend_always { return {}; }

    auto final_suspend() const noexcept -> std::suspend_never { return {}; }

    auto unhandled_exception() const noexcept -> void { std::terminate(); }
};

template <typename ResultType>
class TaskPromise final : public TaskPromiseBase<ResultType> {
  public:
    auto get_return_object() noexcept -> Task<ResultType> {
        return Task<ResultType>{
            std::coroutine_handle<TaskPromise<ResultType>>::from_promise(*this)};
    }

    void return_value(ResultType value) { result_ = std::move(value); };

  private:
    std::optional<ResultType> result_;
};

template <>
class TaskPromise<void> final : public TaskPromiseBase<void> {
  public:
    auto get_return_object() noexcept -> Task<void> {
        return Task<void>{std::coroutine_handle<TaskPromise>::from_promise(*this)};
    }

    void return_void(){};
};

}  // namespace zy_http
#endif  // ZY_HTTP_TASK_HPP