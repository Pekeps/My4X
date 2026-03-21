#include "GameServiceImpl.h"

namespace my4x::server {

grpc::Status GameServiceImpl::SubmitAction([[maybe_unused]] grpc::ServerContext *context,
                                           [[maybe_unused]] const game_proto::SubmitActionRequest *request,
                                           [[maybe_unused]] game_proto::SubmitActionResponse *response) {
    return {grpc::StatusCode::UNIMPLEMENTED, "SubmitAction not yet implemented"};
}

grpc::Status GameServiceImpl::GetGameState([[maybe_unused]] grpc::ServerContext *context,
                                           [[maybe_unused]] const game_proto::GetGameStateRequest *request,
                                           [[maybe_unused]] game_proto::GetGameStateResponse *response) {
    return {grpc::StatusCode::UNIMPLEMENTED, "GetGameState not yet implemented"};
}

grpc::Status GameServiceImpl::SubscribeEvents([[maybe_unused]] grpc::ServerContext *context,
                                              [[maybe_unused]] const game_proto::SubscribeEventsRequest *request,
                                              [[maybe_unused]] grpc::ServerWriter<game_proto::GameEvent> *writer) {
    return {grpc::StatusCode::UNIMPLEMENTED, "SubscribeEvents not yet implemented"};
}

} // namespace my4x::server
