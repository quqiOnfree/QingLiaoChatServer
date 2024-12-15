#include "manager.h"

#include <memory_resource>
#include <system_error>
#include <Ini.h>

#include "user.h"
#include "dataPackage.h"
#include "qls_error.h"

extern qini::INIObject serverIni;

namespace qls
{

struct ManagerImpl
{
    DataManager             m_dataManager; ///< Data manager instance.
    VerificationManager     m_verificationManager; ///< Verification manager instance.

    // Group room map
    std::unordered_map<GroupID, std::shared_ptr<GroupRoom>>
                            m_baseRoom_map; ///< Map of group room IDs to group rooms.
    std::shared_mutex       m_baseRoom_map_mutex; ///< Mutex for group room map.
    std::pmr::synchronized_pool_resource
                            m_baseRoom_sync_pool;

    // Private room map
    std::unordered_map<GroupID, std::shared_ptr<PrivateRoom>>
                            m_basePrivateRoom_map; ///< Map of private room IDs to private rooms.
    std::shared_mutex       m_basePrivateRoom_map_mutex; ///< Mutex for private room map.
    std::pmr::synchronized_pool_resource
                            m_basePrivateRoom_sync_pool;

    // Map of user IDs to private room IDs
    std::unordered_map<PrivateRoomIDStruct, GroupID, PrivateRoomIDStructHasher>
                            m_userID_to_privateRoomID_map; ///< Map of user ID structs to private room IDs.
    std::shared_mutex       m_userID_to_privateRoomID_map_mutex; ///< Mutex for user to private room ID map.

    // User map
    std::unordered_map<UserID, std::shared_ptr<User>>
                            m_user_map; ///< Map of user IDs to users.
    std::shared_mutex       m_user_map_mutex; ///< Mutex for user map.
    std::pmr::synchronized_pool_resource
                            m_user_sync_pool;

    std::unordered_map<std::shared_ptr<Socket>, UserID>
                            m_socket_map; ///< Map of socket pointers to user IDs.
    std::shared_mutex       m_socket_map_mutex; ///< Mutex for socket map.

    // New user ID
    std::atomic<long long>  m_newUserId; ///< Atomic counter for new user IDs.
    // New private room ID
    std::atomic<long long>  m_newPrivateRoomId; ///< Atomic counter for new private room IDs.
    // New group room ID
    std::atomic<long long>  m_newGroupRoomId; ///< Atomic counter for new group room IDs.

    // SQL process manager
    SQLDBProcess            m_sqlProcess; ///< SQL process instance.
};

Manager::Manager():
    m_impl(std::make_unique<ManagerImpl>())
{}

Manager::~Manager() = default;

void Manager::init()
{
    // sql 初始化
    // m_sqlProcess.setSQLServerInfo(serverIni["mysql"]["username"],
    //     serverIni["mysql"]["password"],
    //     "mysql",
    //     serverIni["mysql"]["host"],
    //     unsigned short(std::stoi(serverIni["mysql"]["port"])));

    // m_sqlProcess.connectSQLServer();

    {
        m_impl->m_newUserId = 10000;
        m_impl->m_newPrivateRoomId = 10000;
        m_impl->m_newGroupRoomId = 10000;

        // sql更新初始化数据
        // ...
    }

    m_impl->m_dataManager.init();
    m_impl->m_verificationManager.init();
}

GroupID Manager::addPrivateRoom(UserID user1_id, UserID user2_id)
{
    std::unique_lock<std::shared_mutex> local_unique_lock1(m_impl->m_basePrivateRoom_map_mutex, std::defer_lock),
        local_unique_lock2(m_impl->m_userID_to_privateRoomID_map_mutex, std::defer_lock);
    std::lock(local_unique_lock1, local_unique_lock2);

    // 私聊房间id
    GroupID privateRoom_id(m_impl->m_newGroupRoomId++);
    {
        /*
        * 这里有申请sql 创建私聊房间等命令
        */
    }

    m_impl->m_basePrivateRoom_map[privateRoom_id] = std::allocate_shared<PrivateRoom>(
        std::pmr::polymorphic_allocator<PrivateRoom>(&m_impl->m_basePrivateRoom_sync_pool), user1_id, user2_id, true);
    m_impl->m_userID_to_privateRoomID_map[{user1_id, user2_id}] = privateRoom_id;

    return privateRoom_id;
}

GroupID Manager::getPrivateRoomId(UserID user1_id, UserID user2_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_userID_to_privateRoomID_map_mutex);
    if (m_impl->m_userID_to_privateRoomID_map.find(
        { user1_id , user2_id }) != m_impl->m_userID_to_privateRoomID_map.cend())
        return GroupID(m_impl->m_userID_to_privateRoomID_map.find({ user1_id , user2_id })->second);
    else if (m_impl->m_userID_to_privateRoomID_map.find(
        { user2_id , user1_id }) != m_impl->m_userID_to_privateRoomID_map.cend())
        return GroupID(m_impl->m_userID_to_privateRoomID_map.find({ user2_id , user1_id })->second);
    else throw std::system_error(make_error_code(qls_errc::private_room_not_existed));
}

bool Manager::hasPrivateRoom(GroupID private_room_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_basePrivateRoom_map_mutex);
    return m_impl->m_basePrivateRoom_map.find(
        private_room_id) != m_impl->m_basePrivateRoom_map.cend();
}

bool Manager::hasPrivateRoom(UserID user1_id, UserID user2_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_userID_to_privateRoomID_map_mutex);
    if (m_impl->m_userID_to_privateRoomID_map.find(
        { user1_id , user2_id }) != m_impl->m_userID_to_privateRoomID_map.cend())
        return true;
    else if (m_impl->m_userID_to_privateRoomID_map.find(
        { user2_id , user1_id }) != m_impl->m_userID_to_privateRoomID_map.cend())
        return true;
    else return false;
}

std::shared_ptr<PrivateRoom> Manager::getPrivateRoom(GroupID private_room_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_basePrivateRoom_map_mutex);
    auto itor = m_impl->m_basePrivateRoom_map.find(private_room_id);
    if (itor == m_impl->m_basePrivateRoom_map.cend())
        throw std::system_error(make_error_code(qls_errc::private_room_not_existed));
    return itor->second;
}

void Manager::removePrivateRoom(GroupID private_room_id)
{
    std::unique_lock<std::shared_mutex> local_unique_lock1(m_impl->m_basePrivateRoom_map_mutex, std::defer_lock),
        local_unique_lock2(m_impl->m_userID_to_privateRoomID_map_mutex, std::defer_lock);
    std::lock(local_unique_lock1, local_unique_lock2);

    auto itor = m_impl->m_basePrivateRoom_map.find(private_room_id);
    if (itor == m_impl->m_basePrivateRoom_map.cend())
        throw std::system_error(make_error_code(qls_errc::private_room_not_existed));

    {
        /*
        * 这里有申请sql 删除私聊房间等命令
        */
    }
    auto [user1_id, user2_id] = itor->second->getUserID();

    if (m_impl->m_userID_to_privateRoomID_map.find(
        { user1_id , user2_id }) != m_impl->m_userID_to_privateRoomID_map.cend())
        m_impl->m_userID_to_privateRoomID_map.erase({ user1_id , user2_id });
    else if (m_impl->m_userID_to_privateRoomID_map.find(
        { user2_id , user1_id }) != m_impl->m_userID_to_privateRoomID_map.cend())
        m_impl->m_userID_to_privateRoomID_map.erase({ user2_id , user1_id });

    m_impl->m_basePrivateRoom_map.erase(itor);
}

GroupID Manager::addGroupRoom(UserID opreator_user_id)
{
    std::unique_lock<std::shared_mutex> lock(m_impl->m_baseRoom_map_mutex);
    // 新群聊id
    GroupID group_room_id(m_impl->m_newPrivateRoomId++);
    {
        /*
        * sql 创建群聊获取群聊id
        */
    }

    m_impl->m_baseRoom_map[group_room_id] = std::allocate_shared<GroupRoom>(
        std::pmr::polymorphic_allocator<GroupRoom>(&m_impl->m_baseRoom_sync_pool),
        group_room_id, opreator_user_id, true);

    return group_room_id;
}

bool Manager::hasGroupRoom(GroupID group_room_id) const
{
    std::shared_lock<std::shared_mutex> lock(m_impl->m_baseRoom_map_mutex);
    return m_impl->m_baseRoom_map.find(group_room_id) !=
        m_impl->m_baseRoom_map.cend();
}

std::shared_ptr<GroupRoom> Manager::getGroupRoom(GroupID group_room_id) const
{
    std::shared_lock<std::shared_mutex> lock(m_impl->m_baseRoom_map_mutex);
    auto itor = m_impl->m_baseRoom_map.find(group_room_id);
    if (itor == m_impl->m_baseRoom_map.cend())
        throw std::system_error(make_error_code(qls_errc::group_room_not_existed));
    return itor->second;
}

void Manager::removeGroupRoom(GroupID group_room_id)
{
    std::unique_lock<std::shared_mutex> lock(m_impl->m_baseRoom_map_mutex);
    auto itor = m_impl->m_baseRoom_map.find(group_room_id);
    if (itor == m_impl->m_baseRoom_map.cend())
        throw std::system_error(make_error_code(qls_errc::group_room_not_existed));

    {
        /*
        * sql删除群聊
        */
    }

    m_impl->m_baseRoom_map.erase(group_room_id);
}

std::shared_ptr<User> Manager::addNewUser()
{
    std::unique_lock<std::shared_mutex> local_unique_lock(m_impl->m_user_map_mutex);
    UserID newUserId(m_impl->m_newUserId++);
    {
        // sql处理数据
    }

    m_impl->m_user_map[newUserId] = std::allocate_shared<User>(
        std::pmr::polymorphic_allocator<User>(&m_impl->m_user_sync_pool), newUserId, true);

    return m_impl->m_user_map[newUserId];
}

bool Manager::hasUser(UserID user_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_user_map_mutex);
    return m_impl->m_user_map.find(user_id) != m_impl->m_user_map.cend();
}

std::shared_ptr<User> Manager::getUser(UserID user_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_user_map_mutex);
    auto itor = m_impl->m_user_map.find(user_id);
    if (itor == m_impl->m_user_map.cend())
        throw std::system_error(make_error_code(qls_errc::user_not_existed));
    
    return itor->second;
}

std::unordered_map<UserID, std::shared_ptr<User>> Manager::getUserList() const
{
    std::shared_lock lock(m_impl->m_user_map_mutex);
    return m_impl->m_user_map;
}

void Manager::registerSocket(const std::shared_ptr<Socket> &socket_ptr)
{
    std::unique_lock<std::shared_mutex> local_unique_lock(m_impl->m_socket_map_mutex);
    if (m_impl->m_socket_map.find(socket_ptr) != m_impl->m_socket_map.cend())
        throw std::system_error(make_error_code(qls_errc::socket_pointer_existed));
    m_impl->m_socket_map.emplace(socket_ptr, -1ll);
}

bool Manager::hasSocket(const std::shared_ptr<Socket> &socket_ptr) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_socket_map_mutex);
    return m_impl->m_socket_map.find(socket_ptr) != m_impl->m_socket_map.cend();
}

bool Manager::matchUserOfSocket(const std::shared_ptr<Socket> &socket_ptr, UserID user_id) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_socket_map_mutex);
    auto iter = m_impl->m_socket_map.find(socket_ptr);
    if (iter == m_impl->m_socket_map.cend()) return false;
    return iter->second == user_id;
}

UserID Manager::getUserIDOfSocket(const std::shared_ptr<Socket> &socket_ptr) const
{
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_socket_map_mutex);
    auto iter = m_impl->m_socket_map.find(socket_ptr);
    if (iter == m_impl->m_socket_map.cend())
        throw std::system_error(make_error_code(qls_errc::socket_pointer_not_existed));
    return iter->second;
}

void Manager::modifyUserOfSocket(const std::shared_ptr<Socket> &socket_ptr, UserID user_id, DeviceType type)
{
    std::unique_lock<std::shared_mutex> local_unique_lock(m_impl->m_socket_map_mutex, std::defer_lock);
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_user_map_mutex, std::defer_lock);
    std::lock(local_unique_lock, local_shared_lock);

    if (m_impl->m_user_map.find(user_id) == m_impl->m_user_map.cend())
        throw std::system_error(make_error_code(qls_errc::user_not_existed));

    auto iter = m_impl->m_socket_map.find(socket_ptr);
    if (iter == m_impl->m_socket_map.cend())
        throw std::system_error(make_error_code(qls_errc::socket_pointer_not_existed));

    if (iter->second != -1ll)
        m_impl->m_user_map.find(iter->second)->second->removeSocket(socket_ptr);
    m_impl->m_user_map.find(user_id)->second->addSocket(socket_ptr, type);
    iter->second = user_id;
}

void Manager::removeSocket(const std::shared_ptr<Socket> &socket_ptr)
{
    std::unique_lock<std::shared_mutex> local_unique_lock(m_impl->m_socket_map_mutex, std::defer_lock);
    std::shared_lock<std::shared_mutex> local_shared_lock(m_impl->m_user_map_mutex, std::defer_lock);
    std::lock(local_unique_lock, local_shared_lock);

    auto iter = m_impl->m_socket_map.find(socket_ptr);
    if (iter == m_impl->m_socket_map.cend())
        throw std::system_error(make_error_code(qls_errc::socket_pointer_not_existed));

    if (iter->second != -1l)
        m_impl->m_user_map.find(iter->second)->second->removeSocket(socket_ptr);
    
    m_impl->m_socket_map.erase(iter);
}

SQLDBProcess &Manager::getServerSqlProcess()
{
    return m_impl->m_sqlProcess;
}

DataManager &Manager::getServerDataManager()
{
    return m_impl->m_dataManager;
}

VerificationManager &Manager::getServerVerificationManager()
{
    return m_impl->m_verificationManager;
}

} // namespace qls
