// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
#include "network/NetworkClient.h"

#include <gtest/gtest.h>

#include <vector>

using network::ConnectionState;
using network::NetworkClient;

// ── Initial state ────────────────────────────────────────────────────────────

TEST(NetworkClientTest, InitialState_IsDisconnected) {
    NetworkClient client;
    EXPECT_EQ(client.connectionState(), ConnectionState::Disconnected);
    EXPECT_FALSE(client.isConnected());
}

TEST(NetworkClientTest, InitialState_NoPendingEvents) {
    NetworkClient client;
    EXPECT_EQ(client.pendingEventCount(), 0);
}

TEST(NetworkClientTest, InitialState_PollEvents_ReturnsEmpty) {
    NetworkClient client;
    auto events = client.pollEvents();
    EXPECT_TRUE(events.empty());
}

// ── Connection state machine ─────────────────────────────────────────────────

TEST(NetworkClientTest, Connect_TransitionsToConnected) {
    NetworkClient client;
    // Connect to a non-existent server — the gRPC channel is created lazily,
    // so connect() itself succeeds and sets the state to Connected.
    bool result = client.connect("localhost", 50051);
    EXPECT_TRUE(result);
    EXPECT_EQ(client.connectionState(), ConnectionState::Connected);
    EXPECT_TRUE(client.isConnected());
    client.disconnect();
}

TEST(NetworkClientTest, Disconnect_TransitionsToDisconnected) {
    NetworkClient client;
    client.connect("localhost", 50051);
    client.disconnect();
    EXPECT_EQ(client.connectionState(), ConnectionState::Disconnected);
    EXPECT_FALSE(client.isConnected());
}

TEST(NetworkClientTest, DoubleConnect_ReturnsFalse) {
    NetworkClient client;
    client.connect("localhost", 50051);
    // Second connect while already connected should fail.
    bool result = client.connect("localhost", 50052);
    EXPECT_FALSE(result);
    client.disconnect();
}

TEST(NetworkClientTest, DoubleDisconnect_IsHarmless) {
    NetworkClient client;
    client.connect("localhost", 50051);
    client.disconnect();
    // Second disconnect should be a no-op.
    client.disconnect();
    EXPECT_EQ(client.connectionState(), ConnectionState::Disconnected);
}

TEST(NetworkClientTest, DisconnectWithoutConnect_IsHarmless) {
    NetworkClient client;
    client.disconnect();
    EXPECT_EQ(client.connectionState(), ConnectionState::Disconnected);
}

// ── Action submission ────────────────────────────────────────────────────────

TEST(NetworkClientTest, SubmitAction_WhenDisconnected_ReturnsFalse) {
    NetworkClient client;
    game_proto::GameAction action;
    bool result = client.submitAction("game-1", "player-1", action);
    EXPECT_FALSE(result);
}

TEST(NetworkClientTest, SubmitAction_WhenConnected_ReturnsTrue) {
    NetworkClient client;
    client.connect("localhost", 50051);
    game_proto::GameAction action;
    action.mutable_end_turn(); // Simple action with no extra fields.
    bool result = client.submitAction("game-1", "player-1", action);
    EXPECT_TRUE(result);
    client.disconnect();
}

// ── Event polling after connect/disconnect cycle ─────────────────────────────

TEST(NetworkClientTest, PollEvents_AfterDisconnect_ReturnsEmpty) {
    NetworkClient client;
    client.connect("localhost", 50051);
    client.disconnect();
    auto events = client.pollEvents();
    EXPECT_TRUE(events.empty());
}

// ── Reconnection ─────────────────────────────────────────────────────────────

TEST(NetworkClientTest, Reconnect_AfterDisconnect_Succeeds) {
    NetworkClient client;
    client.connect("localhost", 50051);
    client.disconnect();
    bool result = client.connect("localhost", 50052);
    EXPECT_TRUE(result);
    EXPECT_TRUE(client.isConnected());
    client.disconnect();
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
