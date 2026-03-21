#pragma once

#include "lobby_service.grpc.pb.h"
#include <grpcpp/grpcpp.h>

namespace my4x::server {

/// Stub implementation of the LobbyService gRPC service.
/// All RPCs return UNIMPLEMENTED — real logic will be added in later tasks.
class LobbyServiceImpl final : public game_proto::LobbyService::Service {
  public:
    grpc::Status CreateGame(grpc::ServerContext *context, const game_proto::CreateGameRequest *request,
                            game_proto::CreateGameResponse *response) override;

    grpc::Status ListGames(grpc::ServerContext *context, const game_proto::ListGamesRequest *request,
                           game_proto::ListGamesResponse *response) override;

    grpc::Status JoinGame(grpc::ServerContext *context, const game_proto::JoinGameRequest *request,
                          game_proto::JoinGameResponse *response) override;

    grpc::Status LeaveGame(grpc::ServerContext *context, const game_proto::LeaveGameRequest *request,
                           game_proto::LeaveGameResponse *response) override;
};

} // namespace my4x::server
