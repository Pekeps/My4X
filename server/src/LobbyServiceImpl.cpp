#include "LobbyServiceImpl.h"

namespace my4x::server {

grpc::Status LobbyServiceImpl::CreateGame([[maybe_unused]] grpc::ServerContext *context,
                                          [[maybe_unused]] const game_proto::CreateGameRequest *request,
                                          [[maybe_unused]] game_proto::CreateGameResponse *response) {
    return {grpc::StatusCode::UNIMPLEMENTED, "CreateGame not yet implemented"};
}

grpc::Status LobbyServiceImpl::ListGames([[maybe_unused]] grpc::ServerContext *context,
                                         [[maybe_unused]] const game_proto::ListGamesRequest *request,
                                         [[maybe_unused]] game_proto::ListGamesResponse *response) {
    return {grpc::StatusCode::UNIMPLEMENTED, "ListGames not yet implemented"};
}

grpc::Status LobbyServiceImpl::JoinGame([[maybe_unused]] grpc::ServerContext *context,
                                        [[maybe_unused]] const game_proto::JoinGameRequest *request,
                                        [[maybe_unused]] game_proto::JoinGameResponse *response) {
    return {grpc::StatusCode::UNIMPLEMENTED, "JoinGame not yet implemented"};
}

grpc::Status LobbyServiceImpl::LeaveGame([[maybe_unused]] grpc::ServerContext *context,
                                         [[maybe_unused]] const game_proto::LeaveGameRequest *request,
                                         [[maybe_unused]] game_proto::LeaveGameResponse *response) {
    return {grpc::StatusCode::UNIMPLEMENTED, "LeaveGame not yet implemented"};
}

} // namespace my4x::server
