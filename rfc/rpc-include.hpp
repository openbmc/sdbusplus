// This does not have header gaurds because it must be included multiple times
// for code generation.

#include "drpc.hpp"

#if defined(GENERATE_INTERFACE)
#undef DECLARE_RPC
#undef DECLARE_RPC_CONSTRUCTOR
#undef DECLARE_RPC_METHOD
#define DECLARE_RPC(NAME) DECLARE_INTERFACE(NAME)
#define DECLARE_RPC_CONSTRUCTOR(NAME)
#define DECLARE_RPC_METHOD(NAME, OUT_ARG, ...) DECLARE_INTERFACE_METHOD(NAME, OUT_ARG, ##__VA_ARGS__)

#elif defined(GENERATE_SERVER)
#undef DECLARE_RPC
#undef DECLARE_RPC_CONSTRUCTOR
#undef DECLARE_RPC_METHOD
#define DECLARE_RPC(NAME) DECLARE_SERVER(NAME)
#define DECLARE_RPC_CONSTRUCTOR(NAME) DECLARE_SERVER_CONSTRUCTOR(NAME)
#define DECLARE_RPC_METHOD(NAME, OUT_ARG, ...) DECLARE_SERVER_METHOD(NAME, OUT_ARG, ##__VA_ARGS__)

#elif defined(GENERATE_CLIENT)
#undef DECLARE_RPC
#undef DECLARE_RPC_CONSTRUCTOR
#undef DECLARE_RPC_METHOD
#define DECLARE_RPC(NAME) DECLARE_CLIENT(NAME)
#define DECLARE_RPC_CONSTRUCTOR(NAME) DECLARE_CLIENT_CONSTRUCTOR(NAME)
#define DECLARE_RPC_METHOD(NAME, OUT_ARG, ...) DECLARE_CLIENT_METHOD(NAME, OUT_ARG, ##__VA_ARGS__)

#else
#error either GENERATE_INTERFACE, GENERATE_SERVER, or GENERATE_CLIENT must be set!
#endif

