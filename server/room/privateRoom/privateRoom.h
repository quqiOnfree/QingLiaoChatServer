#ifndef PRIVATE_ROOM_H
#define PRIVATE_ROOM_H

#include <chrono>
#include <string_view>
#include <memory_resource>
#include <memory>

#include "userid.hpp"
#include "room.h"

namespace qls
{

struct PrivateRoomImpl;
struct PrivateRoomImplDeleter
{
    void operator()(PrivateRoomImpl* pri) noexcept;
};
    
/*
* @brief 私聊房间基类
*/
class PrivateRoom: public TextDataRoom
{
public:
    PrivateRoom(UserID user_id_1, UserID user_id_2, bool is_create);
    PrivateRoom(const PrivateRoom&) = delete;
    PrivateRoom(PrivateRoom&&) = delete;

    ~PrivateRoom() noexcept = default;

    void sendMessage(std::string_view message, UserID sender_user_id);
    void sendTipMessage(std::string_view message, UserID sender_user_id);
    void getMessage(
        const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>& from,
        const std::chrono::time_point<std::chrono::system_clock, std::chrono::milliseconds>& to);
        
    std::pair<UserID, UserID> getUserID() const;
    bool hasMember(UserID user_id) const;

    void removeThisRoom();
    bool canBeUsed() const;

private:
    std::unique_ptr<PrivateRoomImpl, PrivateRoomImplDeleter> m_impl;
};

} // namespace qls

#endif // !PRIVATE_ROOM_H
