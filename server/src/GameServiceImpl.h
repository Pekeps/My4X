#pragma once

#include "game_service.grpc.pb.h"
#include <grpcpp/grpcpp.h>

namespace my4x::server {

/// Stub implementation of the GameService gRPC service.
/// All RPCs return UNIMPLEMENTED — real logic will be added in later tasks.
class GameServiceImpl final : public game_proto::GameService::Service {
  public:
    grpc::Status SubmitAction(grpc::ServerContext *context, const game_proto::SubmitActionRequest *request,
                              game_proto::SubmitActionResponse *response) override;

    grpc::Status GetGameState(grpc::ServerContext *context, const game_proto::GetGameStateRequest *request,
                              game_proto::GetGameStateResponse *response) override;

    grpc::Status SubscribeEvents(grpc::ServerContext *context, const game_proto::SubscribeEventsRequest *request,
                                 grpc::ServerWriter<game_proto::GameEvent> *writer) override;
};

} // namespace my4x::server
