#pragma once

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <type_traits>

template <typename T> class SlotInner {
  static_assert(!std::is_void_v<T>);

private:
  std::mutex _guard;
  std::condition_variable _cv;
  std::optional<T> _result;

public:
  bool push(T value) {
    std::unique_lock _lock{_guard};
    if (_result.has_value()) {
      if (!_cv.wait_for(_lock, std::chrono::seconds(1),
                        [&]() { return !_result.has_value(); })) {
        return false;
      }
    }
    _result.emplace(std::move(value));
    _cv.notify_all();
    return true;
  }

  std::optional<T> pull() {
    std::unique_lock _lock{_guard};
    if (!_result.has_value()) {
      if (!_cv.wait_for(_lock, std::chrono::seconds(1),
                        [&]() { return _result.has_value(); })) {
        return std::nullopt;
      }
    }
    T ret = std::move(*_result);
    _result.reset();
    _cv.notify_all();
    return ret;
  };
};

template <> class SlotInner<void> {};

template <typename T> using Slot = std::shared_ptr<SlotInner<T>>;

class PipelineElementBase {
private:
  bool _running = true;

public:
  virtual void stop() { _running = false; }
  bool is_running() { return _running; }

  virtual std::thread start() = 0;
};

template <typename In, typename Out>
class PipelineElement : public PipelineElementBase {
private:
  Slot<In> _input;
  Slot<Out> _output = std::make_shared<SlotInner<Out>>();

public:
  PipelineElement(Slot<In> &input) : _input(input) {}

  virtual Out work(In &&) = 0;

  Slot<Out> &output() { return _output; }
  Slot<In> &input() { return _input; }

  void loop() {
    while (is_running()) {
      std::optional<In> item = _input->pull();
      if (!item)
        continue;

      Out result = work(std::move(*item));
      _output->push(std::move(result));
    }
  }

  virtual std::thread start() override {
    return std::thread(&PipelineElement::loop, this);
  }
};

template <typename Out>
class PipelineElement<void, Out> : public PipelineElementBase {
private:
  Slot<Out> _output = std::make_shared<SlotInner<Out>>();

public:
  virtual Out work() = 0;

  Slot<Out> &output() { return _output; }

  void loop() {
    while (is_running()) {
      Out result = work();
      _output->push(std::move(result));
    }
  }

  virtual std::thread start() override {
    return std::thread(&PipelineElement::loop, this);
  }
};

template <typename In>
class PipelineElement<In, void> : public PipelineElementBase {
private:
  Slot<In> _input;

public:
  PipelineElement(Slot<In> &input) : _input(input) {}

  virtual void work(In &&) = 0;

  Slot<In> &input() { return _input; }

  void loop() {
    while (is_running()) {
      std::optional<In> item = _input->pull();
      if (!item)
        continue;
      work(std::move(*item));
    }
  }

  virtual std::thread start() override {
    return std::thread(&PipelineElement::loop, this);
  }
};

template <typename T> class Tee : public PipelineElementBase {
private:
  Slot<T> _in;
  Slot<T> _out1 = std::make_shared<SlotInner<T>>(),
          _out2 = std::make_shared<SlotInner<T>>();

public:
  Tee(Slot<T> &in) : _in(in) {}

  void loop() {
    while (is_running()) {
      std::optional<T> item = _in->pull();
      if (!item)
        continue;
      _out1->push(*item);
      _out2->push(*item);
    }
  }

  Slot<T> &out1() { return _out1; }
  Slot<T> &out2() { return _out2; }

  virtual std::thread start() override { return std::thread(&Tee::loop, this); }
};