// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

#include <atomic>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

#include "GameServiceImpl.h"
#include "LobbyServiceImpl.h"

namespace {

/// Default port the server listens on when none is specified.
static constexpr int kDefaultPort = 50051;

/// Global pointer used by the signal handler to trigger graceful shutdown.
std::atomic<grpc::Server *> g_server_ptr{nullptr}; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

void signalHandler(int /*signal*/) {
    grpc::Server *server = g_server_ptr.load(std::memory_order_relaxed);
    if (server != nullptr) {
        server->Shutdown();
    }
}

void installSignalHandlers() {
    std::signal(SIGINT, signalHandler);  // NOLINT(cert-err33-c)
    std::signal(SIGTERM, signalHandler); // NOLINT(cert-err33-c)
}

} // namespace

int main(int argc, char *argv[]) {
    // Parse optional port from command-line arguments.
    int port = kDefaultPort;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]); // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } catch (const std::exception &ex) {
            std::cerr << "Invalid port argument: " << argv[1]
                      << "\n"; // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            return 1;
        }
    }

    const std::string server_address = "0.0.0.0:" + std::to_string(port);

    my4x::server::LobbyServiceImpl lobby_service;
    my4x::server::GameServiceImpl game_service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&lobby_service);
    builder.RegisterService(&game_service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "Failed to start server on " << server_address << "\n";
        return 1;
    }

    // Store pointer for signal-based shutdown.
    g_server_ptr.store(server.get(), std::memory_order_relaxed);
    installSignalHandlers();

    std::cout << "My4X server listening on " << server_address << "\n";
    server->Wait();
    std::cout << "Server shut down.\n";

    return 0;
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
