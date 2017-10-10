#ifndef DRPC_HPP_
#define DRPC_HPP_

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <stdint.h>
#include "dbus.hpp"

namespace drpc {

template <typename T, typename ...Types, size_t ...Is>
constexpr std::initializer_list<T*> ToIterableHelper(std::tuple<Types...>* tuple, std::index_sequence<Is...>) {
  return { static_cast<T*>(&std::get<Is>(*tuple))... };
}

template <typename T, typename ...Types>
constexpr std::initializer_list<T*> ToIterable(std::tuple<Types...>* tuple) {
  return ToIterableHelper<T, Types...>(tuple, std::index_sequence_for<Types...>{});
}

template <typename T, typename ...Types, size_t ...Is>
constexpr std::initializer_list<const T&> ToIterableHelper(const std::tuple<Types...>& tuple,
                                                           std::index_sequence<Is...>) {
  return { static_cast<const T&>(std::get<Is>(tuple))... };
}

template <typename T, typename ...Types>
constexpr std::initializer_list<const T&> ToIterable(const std::tuple<Types...>& tuple) {
  return ToIterableHelper<T, Types...>(tuple, std::index_sequence_for<Types...>{});
}

template <typename ...Types, size_t ...Is>
constexpr std::tuple<Types*...> ToPointersHelper(std::tuple<Types...>* tuple, std::index_sequence<Is...>) {
  return std::make_tuple(&std::get<Is>(*tuple)...);
}

template <typename ...Types>
constexpr std::tuple<Types*...> ToPointers(std::tuple<Types...>* tuple) {
  return ToPointersHelper(tuple, std::index_sequence_for<Types...>{});
}

template <typename ...Types, size_t ...Is>
constexpr std::tuple<const Types&...> ToConstRefsHelper(const std::tuple<Types...>& tuple, std::index_sequence<Is...>) {
  return std::make_tuple(std::get<Is>(tuple)...);
}

template <typename ...Types>
constexpr std::tuple<const Types&...> ToConstRefs(const std::tuple<Types...>& tuple) {
  return ToConstRefsHelper(tuple, std::index_sequence_for<Types...>{});
}

template <typename R, typename ...Args, size_t ...Is>
constexpr R InvokeHelper(std::function<R(Args...)> func, std::tuple<Args...>* tuple, std::index_sequence<Is...>) {
  return func(std::get<Is>(*tuple)...);
}

template <typename R, typename ...Args>
constexpr R Invoke(std::function<R(Args...)> func, std::tuple<Args...>* tuple) {
  return InvokeHelper(func, tuple, std::index_sequence_for<Args...>{});
}

template <typename T>
constexpr char PrimitiveSignature() {}

template <>
constexpr char PrimitiveSignature<uint8_t>() { return 'y'; }

template <typename ...Types>
constexpr std::string CompositeSignature() {
  return { PrimitiveSignature<Types>()... };
}

class InputBase {
 public:
  virtual ~InputBase() {}

  virtual bool Parse(const DBusMessageOperations& ops, sd_bus_message* message) = 0;
};

template <typename T>
class SimpleInput : public InputBase {
 public:
  SimpleInput(T* input) : input_(input) {}

  bool Parse(const DBusMessageOperations& ops, sd_bus_message* message) override;

 private:
  T* input_;
};

template <typename ...Types>
class CompositeInput : public InputBase {
 public:
  CompositeInput(Types*... inputs) : inputs_(SimpleInput<Types>(inputs)...) {}

  bool Parse(const DBusMessageOperations& ops, sd_bus_message* message) override {
    for (InputBase* input : ToIterable<InputBase>(&inputs_)) {
      if (!input->Parse(ops, message)) {
        return false;
      }
    }
    return true;
  }

 private:
  std::tuple<SimpleInput<Types>...> inputs_;
};

class OutputBase {
 public:
  virtual ~OutputBase() {}

  virtual void Compose(const DBusMessageOperations& ops, sd_bus_message* message) const = 0;
};

template <typename T>
class SimpleOutput : public OutputBase {
 public:
  SimpleOutput(const T& output) : output_(output) {}

  void Compose(const DBusMessageOperations& ops, sd_bus_message* message) const override;

 private:
  const T& output_;
};

template <typename ...Types>
class CompositeOutput : public OutputBase {
 public:
  CompositeOutput(const Types&... outputs) : outputs_(SimpleOutput<Types>(outputs)...) {}

  void Compose(const DBusMessageOperations& ops, sd_bus_message* message) const override {
    for (const OutputBase& output : ToIterable<OutputBase>(outputs_)) {
      output.Compose(ops, message);
    }
  }

 private:
  std::tuple<SimpleOutput<Types>...> outputs_;
};

struct MethodContext {
  std::string method_name;
  std::string output_signature;
  std::string input_signature;
  DBusHandler* method;
};

class ServerBase {
 public:
  ServerBase(const std::string& path, const std::string& interface) : path_(path), interface_(interface) {}

  virtual ~ServerBase() {}

  virtual void RegisterMethod(const std::string& method_name,
                              const std::string& output_signature,
                              const std::string& input_signature,
                              DBusHandler* method) {
    MethodContext context;
    context.method_name = method_name;
    context.output_signature = output_signature;
    context.input_signature = input_signature;
    context.method = method;
    methods_.push_back(context);
  }

  virtual const std::string& path() const {
    return path_;
  }

  virtual const std::string& interface() const {
    return interface_;
  }

  virtual const std::vector<MethodContext>& methods() const {
    return methods_;
  }

 private:
  std::string path_;
  std::string interface_;
  std::vector<MethodContext> methods_;
};

template <typename Class, typename R, typename ...Args>
class MethodCallback {
 public:
  MethodCallback(Class* class_ptr, R(Class::*method)(Args...))
      : class_ptr_(class_ptr), method_(method) {}

  R operator()(Args... args) { return (class_ptr_->*method_)(args...); }

 private:
  Class* class_ptr_;
  R (Class::*method_)(Args...);
};

template <typename Class, typename R, typename ...Args>
constexpr std::function<R(Args...)> MakeCallback(Class* class_ptr, R(Class::*method)(Args...)) {
  return MethodCallback<Class, R, Args...>(class_ptr, method);
}

template <typename ...Args>
CompositeInput<Args...> MakeCompositeInput(Args*... args) {
  return CompositeInput<Args...>(args...);
}

template <typename R, typename ...Args>
class ServerMethod : public DBusHandler {
 public:
  ServerMethod(ServerBase* server,
               const std::string& method_name,
               std::function<int(R*, Args...)> callback) : callback_(callback) {
    server->RegisterMethod(method_name,
                           CompositeSignature<R>(),
                           CompositeSignature<Args...>(),
                           this);
  }

  virtual int HandleMessage(const DBusMessageOperations& ops, sd_bus_message* message) override {
    std::tuple<Args...> args;
    std::tuple<Args*...> args_ptrs = ToPointers(&args);
    CompositeInput<Args...> input = Invoke(
        static_cast<std::function<CompositeInput<Args...>(Args*...)>>(MakeCompositeInput<Args...>), &args_ptrs);
    if (!input.Parse(ops, message)) {
      return -1;
    }
    R out;
    std::tuple<R*, const Args&...> out_and_args = std::tuple_cat(std::make_tuple(&out), ToConstRefs(args));
    if (Invoke(callback_, &out_and_args) < 0) {
      return -1;
    }
    SimpleOutput<R> output(out);
    output.Compose(ops, message);
    return 0;
  }

 private:
  std::function<int(R*, const Args&...)> callback_;
};

class ClientBase {
 public:
  ClientBase(const std::string& path, const std::string& interface, DBus* dbus)
      : path_(path), interface_(interface), dbus_(dbus) {}

  virtual const std::string& destination() const;

  virtual const std::string& path() const {
    return path_;
  }

  virtual const std::string& interface() const {
    return interface_;
  }

  virtual DBus* mutable_dbus() {
    return dbus_;
  }

 private:
  std::string path_;
  std::string interface_;
  DBus* dbus_;
};

template <typename R, typename ...Args>
class ClientMethod {
 public:
  ClientMethod(ClientBase* client, const std::string& method_name)
      : client_(client), method_name_(method_name) {}

  virtual int Call(R* out, const Args&... args) {
    CompositeOutput<Args...> output(args...);
    SimpleInput<R> input(out);
    return client_->mutable_dbus()->CallMethod(GetMemberInfo(), &input, output);
  }

  virtual DBusMemberInfo GetMemberInfo() {
    return DBusMemberInfo(client_->destination(),
                          client_->path(),
                          client_->interface(),
                          method_name_);
  }

 private:
  ClientBase* client_;
  std::string method_name_;
};

#define NUM_VA_ARGS_IMPL(__dummy, \
                         __1, \
                         __2, \
                         __3, \
                         __4, \
                         __5, \
                         __6, \
                         __7, \
                         __8, \
                         __9, \
                         __10, \
                         __11, \
                         __12, \
                         __13, \
                         __14, \
                         __15, \
                         __16, \
                         __nargs, ...) __nargs

#define NUM_VA_ARGS(...) NUM_VA_ARGS_IMPL(__dummy, ##__VA_ARGS__, \
                                          16, \
                                          15, \
                                          14, \
                                          13, \
                                          12, \
                                          11, \
                                          10, \
                                          9, \
                                          8, \
                                          7, \
                                          6, \
                                          5, \
                                          4, \
                                          3, \
                                          2, \
                                          1, \
                                          0)

#define TO_PARAM_LIST_N(__nargs, ...) TO_PARAM_LIST_##__nargs(__VA_ARGS__)
#define TYPE_LIST_TO_PARAM_LISTN(__nargs, ...) TO_PARAM_LIST_N(__nargs, __VA_ARGS__)
#define TYPE_LIST_TO_PARAM_LIST(...) TYPE_LIST_TO_PARAM_LISTN(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__)

#define TO_PARAM_LIST_0(__type, ...)
#define TO_PARAM_LIST_1(__type, ...) __type __arg0
#define TO_PARAM_LIST_2(__type, ...) __type __arg1, TO_PARAM_LIST_1(__VA_ARGS__)
#define TO_PARAM_LIST_3(__type, ...) __type __arg2, TO_PARAM_LIST_2(__VA_ARGS__)
#define TO_PARAM_LIST_4(__type, ...) __type __arg3, TO_PARAM_LIST_3(__VA_ARGS__)
#define TO_PARAM_LIST_5(__type, ...) __type __arg4, TO_PARAM_LIST_4(__VA_ARGS__)
#define TO_PARAM_LIST_6(__type, ...) __type __arg5, TO_PARAM_LIST_5(__VA_ARGS__)
#define TO_PARAM_LIST_7(__type, ...) __type __arg6, TO_PARAM_LIST_6(__VA_ARGS__)
#define TO_PARAM_LIST_8(__type, ...) __type __arg7, TO_PARAM_LIST_7(__VA_ARGS__)
#define TO_PARAM_LIST_9(__type, ...) __type __arg8, TO_PARAM_LIST_8(__VA_ARGS__)
#define TO_PARAM_LIST_10(__type, ...) __type __arg9, TO_PARAM_LIST_9(__VA_ARGS__)
#define TO_PARAM_LIST_11(__type, ...) __type __arg10, TO_PARAM_LIST_10(__VA_ARGS__)
#define TO_PARAM_LIST_12(__type, ...) __type __arg11, TO_PARAM_LIST_11(__VA_ARGS__)
#define TO_PARAM_LIST_13(__type, ...) __type __arg12, TO_PARAM_LIST_12(__VA_ARGS__)
#define TO_PARAM_LIST_14(__type, ...) __type __arg13, TO_PARAM_LIST_13(__VA_ARGS__)
#define TO_PARAM_LIST_15(__type, ...) __type __arg14, TO_PARAM_LIST_14(__VA_ARGS__)
#define TO_PARAM_LIST_16(__type, ...) __type __arg15, TO_PARAM_LIST_15(__VA_ARGS__)
#define TO_PARAM_LIST_17(__type, ...) ERROR: Too many arguments, to fix this add more TO_PARAM_LIST_XX definitions.

#define GET_ARG_NAMES_N(__nargs, ...) GET_ARG_NAMES_##__nargs(__VA_ARGS__)
#define TYPE_LIST_TO_ARG_NAMESN(__nargs, ...) GET_ARG_NAMES_N(__nargs, __VA_ARGS__)
#define TYPE_LIST_TO_ARG_NAMES(...) TYPE_LIST_TO_ARG_NAMESN(NUM_VA_ARGS(__VA_ARGS__), __VA_ARGS__)

#define GET_ARG_NAMES_0(__type, ...)
#define GET_ARG_NAMES_1(__type, ...) __arg0
#define GET_ARG_NAMES_2(__type, ...) __arg1, GET_ARG_NAMES_1(__VA_ARGS__)
#define GET_ARG_NAMES_3(__type, ...) __arg2, GET_ARG_NAMES_2(__VA_ARGS__)
#define GET_ARG_NAMES_4(__type, ...) __arg3, GET_ARG_NAMES_3(__VA_ARGS__)
#define GET_ARG_NAMES_5(__type, ...) __arg4, GET_ARG_NAMES_4(__VA_ARGS__)
#define GET_ARG_NAMES_6(__type, ...) __arg5, GET_ARG_NAMES_5(__VA_ARGS__)
#define GET_ARG_NAMES_7(__type, ...) __arg6, GET_ARG_NAMES_6(__VA_ARGS__)
#define GET_ARG_NAMES_8(__type, ...) __arg7, GET_ARG_NAMES_7(__VA_ARGS__)
#define GET_ARG_NAMES_9(__type, ...) __arg8, GET_ARG_NAMES_8(__VA_ARGS__)
#define GET_ARG_NAMES_10(__type, ...) __arg9, GET_ARG_NAMES_9(__VA_ARGS__)
#define GET_ARG_NAMES_11(__type, ...) __arg10, GET_ARG_NAMES_10(__VA_ARGS__)
#define GET_ARG_NAMES_12(__type, ...) __arg11, GET_ARG_NAMES_11(__VA_ARGS__)
#define GET_ARG_NAMES_13(__type, ...) __arg12, GET_ARG_NAMES_12(__VA_ARGS__)
#define GET_ARG_NAMES_14(__type, ...) __arg13, GET_ARG_NAMES_13(__VA_ARGS__)
#define GET_ARG_NAMES_15(__type, ...) __arg14, GET_ARG_NAMES_14(__VA_ARGS__)
#define GET_ARG_NAMES_16(__type, ...) __arg15, GET_ARG_NAMES_15(__VA_ARGS__)
#define GET_ARG_NAMES_17(__type, ...) ERROR: Too many arguments, to fix this add more GET_ARG_NAMES_XX definitions.

#define DECLARE_INTERFACE(INTERFACE_NAME) \
    class INTERFACE_NAME##Interface

#define DECLARE_INTERFACE_METHOD(METHOD_NAME, OUT_ARG, ...) \
    public: \
     virtual int METHOD_NAME(OUT_ARG*, ##__VA_ARGS__) = 0;

#define DECLARE_SERVER(SERVER_NAME) \
    class SERVER_NAME##Server : public SERVER_NAME##Interface, public ::drpc::ServerBase

#define DECLARE_SERVER_CONSTRUCTOR(SERVER_NAME) \
    typedef SERVER_NAME##Server Server; \
    public: \
     SERVER_NAME##Server(const std::string& path) : ServerBase(path, #SERVER_NAME) {}

#define DECLARE_SERVER_METHOD(METHOD_NAME, OUT_ARG, ...) \
    public: \
     virtual int METHOD_NAME(OUT_ARG*, ##__VA_ARGS__) override = 0; \
    private: \
     ::drpc::ServerMethod<OUT_ARG, __VA_ARGS__> METHOD_NAME##_method_ \
         {this, #METHOD_NAME, ::drpc::MakeCallback(this, &Server::METHOD_NAME)};

#define DECLARE_CLIENT(CLIENT_NAME) \
    class CLIENT_NAME##Client : public CLIENT_NAME##Interface, public ::drpc::ClientBase

#define DECLARE_CLIENT_CONSTRUCTOR(CLIENT_NAME) \
    public: \
     CLIENT_NAME##Client(const std::string& path, ::drpc::DBus* dbus) : ::drpc::ClientBase(path, #CLIENT_NAME, dbus) {}

#define DECLARE_CLIENT_METHOD(METHOD_NAME, OUT_ARG, ...) \
    public: \
     virtual int METHOD_NAME(OUT_ARG* out_arg, TYPE_LIST_TO_PARAM_LIST(__VA_ARGS__)) {\
       METHOD_NAME##_method_->Call(out_arg, TYPE_LIST_TO_ARG_NAMES(__VA_ARGS__)); \
     }\
    private: \
     ::drpc::ClientMethod<OUT_ARG(__VA_ARGS__)> METHOD_NAME##_method_(this, #METHOD_NAME);

} // namespace drpc
#endif // DRPC_HPP_
