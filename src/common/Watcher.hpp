#ifndef NEURO_SRC_COMMON_WATCHER_HPP_
#define NEURO_SRC_COMMON_WATCHER_HPP_

#include <atomic>
#include <condition_variable>
#include <optional>

namespace neuro {

template <class T>
class Watcher {
 private:
  std::optional<T> _last_element = std::nullopt;
  std::condition_variable _has_new_element;
  mutable std::mutex _cv_mutex;
  std::atomic_bool _has_watcher = false;

 public:
  using value_type = T;
  using Callback = std::function<bool(std::optional<T>&)>;

  void handle_new_element(const T& new_element) {
    if (_has_watcher) {
      {
        std::unique_lock cv_lock(_cv_mutex);
        _last_element = new_element;
      }
      _has_new_element.notify_all();
    }
  }

  void watch(const Callback& callback) {
    if (_has_watcher) {
      return;
    }
    _has_watcher = true;
    while (_has_watcher) {
      std::unique_lock cv_lock(_cv_mutex);
      _has_new_element.wait(cv_lock, [this]() { return _last_element; });
      _has_watcher = callback(_last_element);
      _last_element = std::nullopt;
    }
  }
};

}  // namespace neuro

#endif  // NEURO_SRC_COMMON_WATCHER_HPP_
