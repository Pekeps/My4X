#include "network/NetworkClient.h"

#include <grpcpp/grpcpp.h>

#include <chrono>
#include <string>
#include <utility>

namespace network {

// ── Constructor / Destructor ─────────────────────────────────────────────────

NetworkClient::NetworkClient() = default;

NetworkClient::~NetworkClient() { disconnect(); }

// ── Connection lifecycle ─────────────────────────────────────────────────────

bool NetworkClient::connect(const std::string &host, std::uint16_t port) {
    // Only allow connecting from the Disconnected state.
    ConnectionState expected = ConnectionState::Disconnected;
    if (!state_.compare_exchange_strong(expected, ConnectionState::Connecting)) {
        return false;
    }

    // Build the target string (e.g. "localhost:50051").
    std::string target = host + ":" + std::to_string(port);

    // Create an insecure channel (TLS can be added later).
    channel_ = grpc::CreateChannel(target, grpc::InsecureChannelCredentials());
    stub_ = game_proto::GameService::NewStub(channel_);
    completionQueue_ = std::make_unique<grpc::CompletionQueue>();

    shutdownRequested_.store(false);

    // Launch the background I/O thread.
    networkThread_ = std::thread(&NetworkClient::networkThreadFunc, this);

    setConnectionState(ConnectionState::Connected);
    return true;
}

void NetworkClient::disconnect() {
    // Only allow disconnecting when Connected or Connecting.
    ConnectionState current = state_.load();
    if (current == ConnectionState::Disconnected || current == ConnectionState::Disconnecting) {
        return;
    }

    setConnectionState(ConnectionState::Disconnecting);
    shutdownRequested_.store(true);

    // Shut down the completion queue so Next() returns false.
    if (completionQueue_) {
        completionQueue_->Shutdown();
    }

    // Wait for the background thread to finish.
    if (networkThread_.joinable()) {
        networkThread_.join();
    }

    // Release gRPC resources.
    stub_.reset();
    channel_.reset();
    completionQueue_.reset();

    setConnectionState(ConnectionState::Disconnected);
}

ConnectionState NetworkClient::connectionState() const { return state_.load(); }

bool NetworkClient::isConnected() const { return state_.load() == ConnectionState::Connected; }

// ── Action submission ────────────────────────────────────────────────────────

bool NetworkClient::submitAction(const std::string &gameId, const std::string &playerId,
                                 const game_proto::GameAction &action) {
    if (!isConnected()) {
        return false;
    }

    // Build the request.
    game_proto::SubmitActionRequest request;
    request.mutable_game_id()->set_id(gameId);
    request.mutable_player_id()->set_id(playerId);
    *request.mutable_action() = action;

    // Perform a synchronous-on-background-thread call using a detached thread
    // so the caller (render thread) is never blocked.
    // The stub and channel are thread-safe for concurrent RPCs.
    auto *localStub = stub_.get();
    auto reqCopy = std::make_shared<game_proto::SubmitActionRequest>(request);

    std::thread([localStub, reqCopy]() {
        grpc::ClientContext context;
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(DEFAULT_RPC_TIMEOUT_MS);
        context.set_deadline(deadline);

        game_proto::SubmitActionResponse response;
        localStub->SubmitAction(&context, *reqCopy, &response);
        // Response is intentionally ignored for now — fire-and-forget.
        // A callback/future mechanism will be added when the server is ready.
    }).detach();

    return true;
}

// ── Event polling ────────────────────────────────────────────────────────────

std::vector<game_proto::GameEvent> NetworkClient::pollEvents() {
    std::lock_guard<std::mutex> lock(eventMutex_);
    std::vector<game_proto::GameEvent> events;
    events.swap(eventBuffer_);
    return events;
}

std::size_t NetworkClient::pendingEventCount() const {
    std::lock_guard<std::mutex> lock(eventMutex_);
    return eventBuffer_.size();
}

// ── Background thread ────────────────────────────────────────────────────────

void NetworkClient::networkThreadFunc() {
    // For now this is a stub — the completion-queue drain loop will be
    // fleshed out once the server-side streaming for SubscribeEvents is ready.
    //
    // The thread simply waits until shutdown is requested.
    while (!shutdownRequested_.load()) {
        // Drain the completion queue with a short deadline so we can
        // periodically check the shutdown flag.
        void *tag = nullptr;
        bool ok = false;
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(DEFAULT_RPC_TIMEOUT_MS);
        grpc::CompletionQueue::NextStatus status = completionQueue_->AsyncNext(&tag, &ok, deadline);

        (void)ok; // Will be used when processing async RPC completions.

        if (status == grpc::CompletionQueue::SHUTDOWN) {
            break;
        }
        // GOT_EVENT would be processed here once async RPCs are enqueued.
    }
}

// ── Internal helpers ─────────────────────────────────────────────────────────

void NetworkClient::setConnectionState(ConnectionState state) { state_.store(state); }

} // namespace network
