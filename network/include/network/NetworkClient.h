#pragma once

#include "game_event.pb.h"
#include "game_service.grpc.pb.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace grpc {
class Channel;
class CompletionQueue;
} // namespace grpc

namespace network {

// ── Connection state ────────────────────────────────────────────────────────

/// Possible states of the network connection.
enum class ConnectionState : std::uint8_t {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
};

// ── Action submission result ────────────────────────────────────────────────

/// Result of an asynchronous action submission.
struct SubmitResult {
    bool success = false;
    std::string errorMessage;
};

// ── Constants ───────────────────────────────────────────────────────────────

/// Default timeout for gRPC calls in milliseconds.
static constexpr int DEFAULT_RPC_TIMEOUT_MS = 5000;

/// Maximum number of events buffered before oldest are dropped.
static constexpr std::size_t MAX_EVENT_BUFFER_SIZE = 1024;

// ── NetworkClient ───────────────────────────────────────────────────────────

/// Client-side gRPC connection manager for the 4X game.
///
/// Provides a non-blocking interface for submitting game actions and
/// receiving server-pushed events. All gRPC I/O happens on a dedicated
/// background thread so the render loop is never blocked.
///
/// This is a pure networking abstraction — it contains no game logic.
class NetworkClient {
  public:
    NetworkClient();
    ~NetworkClient();

    // Non-copyable.
    NetworkClient(const NetworkClient &) = delete;
    NetworkClient &operator=(const NetworkClient &) = delete;

    // Non-movable (background thread holds `this`).
    NetworkClient(NetworkClient &&) = delete;
    NetworkClient &operator=(NetworkClient &&) = delete;

    // ── Connection lifecycle ────────────────────────────────────────────

    /// Establish a gRPC channel to the given host and port.
    /// Returns true if the connection attempt was initiated.
    /// The actual connection proceeds asynchronously.
    bool connect(const std::string &host, std::uint16_t port);

    /// Cleanly shut down the connection and stop the background thread.
    void disconnect();

    /// Query the current connection state.
    [[nodiscard]] ConnectionState connectionState() const;

    /// Convenience: returns true when connectionState() == Connected.
    [[nodiscard]] bool isConnected() const;

    // ── Action submission ───────────────────────────────────────────────

    /// Submit a game action to the server (non-blocking).
    /// The action is serialised and sent on the background thread.
    /// Returns false if not connected.
    bool submitAction(const std::string &gameId, const std::string &playerId, const game_proto::GameAction &action);

    // ── Event polling ───────────────────────────────────────────────────

    /// Retrieve all game events received since the last call to pollEvents.
    /// Returns an empty vector when there are no new events.
    /// This is safe to call from the render thread.
    [[nodiscard]] std::vector<game_proto::GameEvent> pollEvents();

    /// Return the number of buffered events not yet polled.
    [[nodiscard]] std::size_t pendingEventCount() const;

  private:
    /// Background thread entry point — drains the completion queue.
    void networkThreadFunc();

    /// Internal: update connection state (thread-safe).
    void setConnectionState(ConnectionState state);

    /// The gRPC channel to the server.
    std::shared_ptr<grpc::Channel> channel_;

    /// The generated stub for the GameService.
    std::unique_ptr<game_proto::GameService::Stub> stub_;

    /// Completion queue for async gRPC operations.
    std::unique_ptr<grpc::CompletionQueue> completionQueue_;

    /// Background thread that drives async gRPC I/O.
    std::thread networkThread_;

    /// Current connection state (atomic for lock-free reads).
    std::atomic<ConnectionState> state_{ConnectionState::Disconnected};

    /// Signals the background thread to stop.
    std::atomic<bool> shutdownRequested_{false};

    /// Mutex protecting the event buffer.
    mutable std::mutex eventMutex_;

    /// Buffered events received from the server.
    std::vector<game_proto::GameEvent> eventBuffer_;
};

} // namespace network
